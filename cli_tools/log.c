/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "../dist/sds/sds.h"
#include "log.h"

#include <string.h>
#include <stdio.h>
#include <time.h>

int loglevel;
bool log_on_tty;

static const char *loglevel_colors[] = {
    "\033[0;31m", "\033[0;31m", "\033[0;31m", "\033[0;31m", "\033[0;33m", "", "", "\033[0;34m"
};

void set_loglevel(int level) {
    if (level > 7) {
        level = 7;
    }
    else if (level < 0) {
        level = 0;
    }
    MYMPD_LOG_NOTICE("Setting loglevel to %d", level);
    loglevel = level;
}

void mympd_log_errno(const char *file, int line, int errnum) {
    char err_text[256];
    int rc = strerror_r(errnum, err_text, 256);
    const char *err_str = rc == 0 ? err_text : "Unknown error";
    mympd_log(LOG_ERR, file, line, "%s", err_str);
}

void mympd_log(int level, const char *file, int line, const char *fmt, ...) {
    if (level > loglevel) {
        return;
    }

    sds logline = sdsempty();
    if (log_on_tty == true) {
        logline = sdscat(logline, loglevel_colors[level]);
    }

    #ifdef MYMPD_DEBUG
        logline = sdscatprintf(logline, "%s:%d: ", file, line);
    #else
        (void)file;
        (void)line;
    #endif

    va_list args;
    va_start(args, fmt);
    logline = sdscatvprintf(logline, fmt, args);
    va_end(args);

    if (sdslen(logline) > 1023) {
        sdsrange(logline, 0, 1020);
        logline = sdscatlen(logline, "...\n", 4);
    }
    else {
        logline = sdscatlen(logline, "\n", 1);
    }
    if (log_on_tty == true) {
        logline = sdscat(logline, "\033[0m");
    }

    fputs(logline, stdout);
    fflush(stdout);

    sdsfree(logline);
}
