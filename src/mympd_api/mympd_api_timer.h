/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_TIMER_H
#define MYMPD_API_TIMER_H

typedef enum {
    TIMER_SINGLE_SHOT = 0, /*Periodic Timer*/
    TIMER_PERIODIC         /*Single Shot Timer*/
} t_timer;

typedef void (*time_handler)(void *user_data);

struct t_timer_node {
    int fd;
    time_handler callback;
    void *user_data;
    unsigned int interval;
    t_timer type;
    int timer_id;
    struct t_timer_node *next;
};

struct t_timer_list {
    int length;
    struct t_timer_node *list;
};


void init_timerlist(struct t_timer_list *l);
void truncate_timerlist(struct t_timer_list *l);
void check_timer(struct t_timer_list *l);
bool add_timer(struct t_timer_list *l, unsigned int interval, time_handler handler, t_timer type, int timer_id, void *user_data);
void remove_timer(struct t_timer_list *l, int timer_id);

#endif
