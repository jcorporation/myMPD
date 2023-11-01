/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/thread.h"

#include "src/lib/log.h"

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

#include <string.h>

/**
 * Ignore the thread name setting
 * @param threadname name of thread
 */
void set_threadname(__attribute__((__unused__)) const char *threadname) {
    MYMPD_LOG_DEBUG(NULL, "Setting the thread name is not supported");
}

#endif


