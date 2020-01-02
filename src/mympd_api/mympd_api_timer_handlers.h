/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_TIMER_HANDLERS_H
#define MYMPD_API_TIMER_HANDLERS_H
void timer_handler_covercache(void *user_data); //timer_id 1
void timer_handler_smartpls_update(void *user_data); //timer_id 2
void timer_handler_select(void *user_data);
#endif
