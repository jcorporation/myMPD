/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Posix thread helpers
 */

#include "compile_time.h"
#include "src/lib/thread.h"

_Atomic int mympd_worker_threads;  //!< Count of running worker threads
#ifdef MYMPD_ENABLE_LUA
    _Atomic int script_worker_threads;  //!< Count of running runscript threads
#endif

#ifdef __linux__

#include <sys/prctl.h>

/**
 * Sets the thread name for linux
 * @param threadname name of thread
 */
void set_threadname(const char *threadname) {
    prctl(PR_SET_NAME, threadname, 0, 0, 0);
}

#else

#include "src/lib/log.h"

#include <string.h>

/**
 * Ignore the thread name setting
 * @param threadname name of thread
 */
void set_threadname(const char *threadname) {
    (void) threadname;
    MYMPD_LOG_DEBUG(NULL, "Setting the thread name is not supported");
}

#endif


