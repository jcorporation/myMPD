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

int signalfd_init(void);
void signalfd_close(int fd);
bool signalfd_handler(int fd);

#endif
