/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_queue.h"

#include "log.h"
#include "mem.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

//private definitions
static int unlock_mutex(pthread_mutex_t *mutex);
static void set_wait_time(int timeout, struct timespec *max_wait);

//public functions

struct t_mympd_queue *mympd_queue_create(const char *name) {
    struct t_mympd_queue *queue = malloc_assert(sizeof(struct t_mympd_queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->length = 0;
    queue->name = name;

    queue->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    queue->wakeup = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    return queue;
}

void mympd_queue_free(struct t_mympd_queue *queue) {
    struct t_mympd_msg *current = queue->head;
    struct t_mympd_msg *tmp = NULL;
    while (current != NULL) {
        FREE_PTR(current->data);
        tmp = current;
        current = current->next;
        FREE_PTR(tmp);
    }
    FREE_PTR(queue);
}

int mympd_queue_push(struct t_mympd_queue *queue, void *data, long id) {
    int rc = pthread_mutex_lock(&queue->mutex);
    if (rc != 0) {
        MYMPD_LOG_ERROR("Error in pthread_mutex_lock: %d", rc);
        return 0;
    }
    struct t_mympd_msg* new_node = malloc_assert(sizeof(struct t_mympd_msg));
    new_node->data = data;
    new_node->id = id;
    new_node->timestamp = time(NULL);
    new_node->next = NULL;
    queue->length++;
    if (queue->head == NULL && queue->tail == NULL){
        queue->head = queue->tail = new_node;
    }
    else {
        queue->tail->next = new_node;
        queue->tail = new_node;
    }
    if (unlock_mutex(&queue->mutex) != 0) {
        return 0;
    }
    rc = pthread_cond_signal(&queue->wakeup);
    if (rc != 0) {
        MYMPD_LOG_ERROR("Error in pthread_cond_signal: %d", rc);
        return 0;
    }
    return 1;
}

long mympd_queue_length(struct t_mympd_queue *queue, int timeout) {
    int rc = pthread_mutex_lock(&queue->mutex);
    if (rc != 0) {
        MYMPD_LOG_ERROR("Error in pthread_mutex_lock: %d", rc);
        return 0;
    }
    if (timeout > 0 && queue->length == 0) {
        struct timespec max_wait = {0, 0};
        set_wait_time(timeout, &max_wait);
        errno = 0;
        rc = pthread_cond_timedwait(&queue->wakeup, &queue->mutex, &max_wait);
        if (rc != 0 && rc != ETIMEDOUT) {
            MYMPD_LOG_ERROR("Error in pthread_cond_timedwait: %d", rc);
            MYMPD_LOG_ERRNO(errno);
        }
    }
    long len = queue->length;
    unlock_mutex(&queue->mutex);
    return len;
}

void *mympd_queue_shift(struct t_mympd_queue *queue, int timeout, long id) {
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
            if (id == 0 || id == current->id) {
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

void *mympd_queue_expire(struct t_mympd_queue *queue, time_t max_age) {
    int rc = pthread_mutex_lock(&queue->mutex);
    if (rc != 0) {
        MYMPD_LOG_ERROR("Error in pthread_mutex_lock: %d", rc);
        return 0;
    }
    //queue has entry
    if (queue->head != NULL) {
        struct t_mympd_msg *current = NULL;
        struct t_mympd_msg *previous = NULL;

        time_t expire_time = time(NULL) - max_age;

        for (current = queue->head; current != NULL; previous = current, current = current->next) {
            if (max_age == 0 || current->timestamp < expire_time) {
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
                MYMPD_LOG_WARN("Found expired entry in queue");
                return data;
            }
        }
    }

    unlock_mutex(&queue->mutex);
    return NULL;
}

//private functions

static int unlock_mutex(pthread_mutex_t *mutex) {
    int rc = pthread_mutex_unlock(mutex);
    if (rc != 0) {
        MYMPD_LOG_ERROR("Error in pthread_mutex_unlock: %d", rc);
    }
    return rc;
}

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
