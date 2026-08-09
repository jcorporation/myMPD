/*
 SPDX-License-Identifier: GPL-3.0-or-later
 (c) 2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Signal handling
 */

#ifndef MYMPD_LIB_SIGNAL_H
#define MYMPD_LIB_SIGNAL_H

#include <signal.h>
#include <stdbool.h>

extern sig_atomic_t s_signal_received;

bool signal_eventfd_init(void);
int signal_eventfd_get(void);
void signal_eventfd_close(void);
bool signal_eventfd_handler(void);

#endif
