/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_LIB_TIMER_H
#define MYMPD_LIB_TIMER_H

#include <stdbool.h>

int mympd_timer_create(int clock, int timeout, int interval);
bool mympd_timer_read(int fd);
bool mympd_timer_set(int timer_fd, int timeout, int interval);
void mympd_timer_log_next_expire(int timer_fd);
void mympd_timer_close(int fd);

#endif
