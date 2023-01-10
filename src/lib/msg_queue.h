/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_QUEUE_H
#define MYMPD_QUEUE_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

enum mympd_queue_types {
    QUEUE_TYPE_REQUEST,  //!< queue holds only t_work_request entries
    QUEUE_TYPE_RESPONSE  //!< queue holds only t_work_response entries
};

/**
 * A message in the queue
 */
struct t_mympd_msg {
    void *data;                //!< data t_work_request or t_work_response
    long id;                   //!< id of the message
    time_t timestamp;          //!< messages added timestamp
    struct t_mympd_msg *next;  //!< pointer to next message
};

/**
 * Struct for the thread save message queue
 */
struct t_mympd_queue {
    int length;                   //!< length of the queue
    struct t_mympd_msg *head;     //!< pointer to first message
    struct t_mympd_msg *tail;     //!< pointer to last message
    pthread_mutex_t mutex;        //!< the mutex
    pthread_cond_t wakeup;        //!< condition varibale for the mutex
    const char *name;             //!< descriptive name
    enum mympd_queue_types type;  //!< the queue type (request or response)
};

struct t_mympd_queue *mympd_queue_create(const char *name, enum mympd_queue_types type);
void *mympd_queue_free(struct t_mympd_queue *queue);
bool mympd_queue_push(struct t_mympd_queue *queue, void *data, long id);
void *mympd_queue_shift(struct t_mympd_queue *queue, int timeout, long id);
int mympd_queue_expire(struct t_mympd_queue *queue, time_t max_age);
#endif
