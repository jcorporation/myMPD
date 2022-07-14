/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_queue.h"

#include "api.h"
#include "log.h"
#include "lua_mympd_state.h"
#include "mem.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

/*
 Message queue implementation to transfer messages between threads asynchronously
*/

//private definitions
static void free_queue_node(struct t_mympd_msg *n, enum mympd_queue_types type);
static void free_queue_node_extra(void *extra, enum mympd_cmd_ids cmd_id);
static int unlock_mutex(pthread_mutex_t *mutex);
static void set_wait_time(int timeout, struct timespec *max_wait);

//public functions

/**
 * Creates a thread safe message queue
 * @param name description of the queue
 * @param type type of the queue QUEUE_TYPE_REQUEST or QUEUE_TYPE_RESPONSE
 * @return pointer to allocated and initialized queue struct
 */
struct t_mympd_queue *mympd_queue_create(const char *name, enum mympd_queue_types type) {
    struct t_mympd_queue *queue = malloc_assert(sizeof(struct t_mympd_queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->length = 0;
    queue->name = name;
    queue->type = type;
    queue->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    queue->wakeup = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    return queue;
}

/**
 * Frees all queue nodes and the queue itself
 * @param queue pointer to the queue
 */
void *mympd_queue_free(struct t_mympd_queue *queue) {
    mympd_queue_expire(queue, 0);
    FREE_PTR(queue);
    return NULL;
}

/**
 * Appends data to the queue
 * @param queue pointer to the queue
 * @param data struct t_work_request or t_work_response
 * @param id id of the queue entry
 * @return true on success else false
 */
bool mympd_queue_push(struct t_mympd_queue *queue, void *data, long id) {
    int rc = pthread_mutex_lock(&queue->mutex);
    if (rc != 0) {
        MYMPD_LOG_ERROR("Error in pthread_mutex_lock: %d", rc);
        return false;
    }
    struct t_mympd_msg* new_node = malloc_assert(sizeof(struct t_mympd_msg));
    new_node->data = data;
    new_node->id = id;
    new_node->timestamp = time(NULL);
    new_node->next = NULL;
    queue->length++;
    if (queue->head == NULL &&
        queue->tail == NULL)
    {
        queue->head = queue->tail = new_node;
    }
    else {
        queue->tail->next = new_node;
        queue->tail = new_node;
    }
    if (unlock_mutex(&queue->mutex) != 0) {
        return false;
    }
    rc = pthread_cond_signal(&queue->wakeup);
    if (rc != 0) {
        MYMPD_LOG_ERROR("Error in pthread_cond_signal: %d", rc);
        return 0;
    }
    return true;
}

/**
 * Gets the first entry or the entry with specific id
 * @param queue pointer to the queue
 * @param timeout timeout in ms to wait for a queue entry, 0 to wait infinite
 * @param id 0 for first entry or specific id
 * @return t_work_request or t_work_response
 */
void *mympd_queue_shift(struct t_mympd_queue *queue, int timeout, long id) {
    //lock the queue
    int rc = pthread_mutex_lock(&queue->mutex);
    if (rc != 0) {
        MYMPD_LOG_ERROR("Error in pthread_mutex_lock: %d", rc);
        assert(NULL);
    }
    if (queue->length == 0) {
        if (timeout > 0) {
            struct timespec max_wait = {0, 0};
            set_wait_time(timeout, &max_wait);
            errno = 0;
            rc = pthread_cond_timedwait(&queue->wakeup, &queue->mutex, &max_wait);
        }
        else {
            //wait infinite for a queue entry
            errno = 0;
            rc = pthread_cond_wait(&queue->wakeup, &queue->mutex);
        }

        if (rc != 0) {
            if (rc != ETIMEDOUT) {
                MYMPD_LOG_ERROR("Error in pthread_cond_timedwait: %d", rc);
                MYMPD_LOG_ERRNO(errno);
            }
            unlock_mutex(&queue->mutex);
            return NULL;
        }
    }
    //queue has entry
    if (queue->head != NULL) {
        struct t_mympd_msg *current = NULL;
        struct t_mympd_msg *previous = NULL;

        for (current = queue->head; current != NULL; previous = current, current = current->next) {
            if (id == 0 ||
                id == current->id)
            {
                void *data = current->data;

                if (previous == NULL) {
                    //Fix beginning pointer
                    queue->head = current->next;
                }
                else {
                    //Fix previous nodes next to skip over the removed node.
                    previous->next = current->next;
                }
                //Fix tail
                if (queue->tail == current) {
                    queue->tail = previous;
                }

                FREE_PTR(current);
                queue->length--;
                unlock_mutex(&queue->mutex);
                return data;
            }
            MYMPD_LOG_DEBUG("Skipping queue entry with id %ld", current->id);
        }
    }

    unlock_mutex(&queue->mutex);
    return NULL;
}

/**
 * Expire entries from the queue
 * @param queue pointer to the queue
 * @param max_age max age of nodes in seconds
 * @return number of expired nodes
 */
int mympd_queue_expire(struct t_mympd_queue *queue, time_t max_age) {
    int rc = pthread_mutex_lock(&queue->mutex);
    if (rc != 0) {
        MYMPD_LOG_ERROR("Error in pthread_mutex_lock: %d", rc);
        return 0;
    }
    //queue has entry
    int expired_count = 0;
    if (queue->head != NULL) {
        struct t_mympd_msg *current = NULL;
        struct t_mympd_msg *previous = NULL;

        time_t expire_time = time(NULL) - max_age;

        for (current = queue->head; current != NULL;) {
            if (max_age == 0 ||
                current->timestamp < expire_time)
            {
                struct t_mympd_msg *to_remove = current;
                if (queue->tail == current) {
                    //Fix tail
                    queue->tail = previous;
                }
                if (previous == NULL) {
                    //Fix beginning pointer
                    queue->head = current->next;
                    //Set current to queue head
                    current = queue->head;
                }
                else {
                    //Fix previous nodes next to skip over the removed node.
                    previous->next = current->next;
                    //Set current to previous
                    current = previous;
                }
                free_queue_node(to_remove, queue->type);
                queue->length--;
                expired_count++;
            }
            else {
                //skip this node
                previous = current;
                current = current->next;
            }
        }
    }

    unlock_mutex(&queue->mutex);
    return expired_count;
}

//privat functions

/**
 * Frees a queue node, node must be detached from queue
 * @param node to free
 * @param type type of the queue QUEUE_TYPE_REQUEST or QUEUE_TYPE_RESPONSE
 */
static void free_queue_node(struct t_mympd_msg *node, enum mympd_queue_types type) {
    //free data
    if (type == QUEUE_TYPE_REQUEST) {
        struct t_work_request *request = node->data;
        free_queue_node_extra(request->extra, request->cmd_id);
        free_request(request);
    }
    else {
        //QUEUE_TYPE_RESPONSE
        struct t_work_response *response = node->data;
        free_queue_node_extra(response->extra, response->cmd_id);
        free_response(response);
    }
    //free the node itself
    FREE_PTR(node);
}

/**
 * Frees the extra data from a t_work_request or t_work_response struct.
 * It respects the cmd_id
 * @param extra extra data from request or response
 * @param cmd_id method of request or response
 */
static void free_queue_node_extra(void *extra, enum mympd_cmd_ids cmd_id) {
    if (extra == NULL) {
        return;
    }
    if (cmd_id == INTERNAL_API_SCRIPT_INIT) {
        lua_mympd_state_free(extra);
    }
    else {
        FREE_PTR(extra);
    }
}

/**
 * Unlocks the queue mutex
 * @param mutex the mutex to unlock
 */
static int unlock_mutex(pthread_mutex_t *mutex) {
    int rc = pthread_mutex_unlock(mutex);
    if (rc != 0) {
        MYMPD_LOG_ERROR("Error in pthread_mutex_unlock: %d", rc);
    }
    return rc;
}

/**
 * Populates the timespec struct with time now + timeout
 * @param timeout timeout in ms
 * @param max_wait timespec struct to populate
 */
static void set_wait_time(int timeout, struct timespec *max_wait) {
    timeout = timeout * 1000;
    errno = 0;
    if (clock_gettime(CLOCK_REALTIME, max_wait) == -1) {
        MYMPD_LOG_ERROR("Error getting realtime");
        MYMPD_LOG_ERRNO(errno);
        assert(NULL);
    }
    //timeout in ms
    if (max_wait->tv_nsec <= (999999999 - timeout)) {
        max_wait->tv_nsec += timeout;
    }
    else {
        max_wait->tv_sec += 1;
        max_wait->tv_nsec = timeout - (999999999 - max_wait->tv_nsec);
    }
}
