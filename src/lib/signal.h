/*
 SPDX-License-Identifier: GPL-3.0-or-later
 (c) 2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Search implementation
 */

#ifndef MYMPD_LIB_SIGNAL_H
#define MYMPD_LIB_SIGNAL_H

#include <signal.h>
#include <stdbool.h>

extern sig_atomic_t s_signal_received;

bool set_signal_handler(int sig_num);

#endif
