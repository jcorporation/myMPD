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

enum mympd_queue_types {
    QUEUE_TYPE_REQUEST,
    QUEUE_TYPE_RESPONSE
};

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
    enum mympd_queue_types type;
};

struct t_mympd_queue *mympd_queue_create(const char *name, enum mympd_queue_types type);
void *mympd_queue_free(struct t_mympd_queue *queue);
bool mympd_queue_push(struct t_mympd_queue *queue, void *data, long id);
void *mympd_queue_shift(struct t_mympd_queue *queue, int timeout, long id);
int mympd_queue_expire(struct t_mympd_queue *queue, time_t max_age);
#endif
