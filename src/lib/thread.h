/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Posix thread helpers
 */

#ifndef MYMPD_THREAD_H
#define MYMPD_THREAD_H

extern _Atomic int mympd_worker_threads;
#ifdef MYMPD_ENABLE_LUA
    extern _Atomic int script_worker_threads;
#endif

void set_threadname(const char *threadname);

#endif
