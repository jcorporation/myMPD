/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_QUEUE_H
#define MYMPD_QUEUE_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

struct t_mympd_msg {
    void *data;
    long id;
    time_t timestamp;
    struct t_mympd_msg *next;
};

struct t_mympd_queue {
    int length;
    struct t_mympd_msg *head;
    struct t_mympd_msg *tail;
    pthread_mutex_t mutex;
    pthread_cond_t wakeup;
    const char *name;
};

struct t_mympd_queue *mympd_queue_create(const char *name);
void *mympd_queue_free(struct t_mympd_queue *queue);
bool mympd_queue_push(struct t_mympd_queue *queue, void *data, long id);
void *mympd_queue_shift(struct t_mympd_queue *queue, int timeout, long id);
long mympd_queue_length(struct t_mympd_queue *queue, int timeout);
int expire_response_queue(struct t_mympd_queue *queue, time_t age);
int expire_request_queue(struct t_mympd_queue *queue, time_t age);
#endif
