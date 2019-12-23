/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYTIMER_H
#define MYTIMER_H
#include <stdlib.h>
 
typedef enum {
    TIMER_SINGLE_SHOT = 0, /*Periodic Timer*/
    TIMER_PERIODIC         /*Single Shot Timer*/
} t_timer;
 
typedef void (*time_handler)(size_t timer_id, void *user_data);
 
void *timer_loop(void *data);
size_t start_timer(unsigned int interval, time_handler handler, t_timer type, void *user_data);
void stop_timer(size_t timer_id);
 
#endif
