/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>

#include "../dist/src/sds/sds.h"
#include "log.h"

int loglevel;

static const char *loglevel_names[] = {
  "ERROR", "WARN", "INFO", "VERBOSE", "DEBUG"
};

static const char *loglevel_colors[] = {
  "\033[0;31m", "\033[0;33m", "", "", "\033[0;34m"
};

void set_loglevel(int level) {
    if (level > 4) {
        level = 4;
    }
    else if (level < 0) {
        level = 0;
    }
    LOG_INFO("Setting loglevel to %s", loglevel_names[level]);
    loglevel = level;
}

void mympd_log(int level, const char *file, int line, const char *fmt, ...) {
    if (level > loglevel) {
        return;
    }
    
    sds logline = sdsnew(loglevel_colors[level]);
    
    if (loglevel == 4) {
        time_t now = time(NULL);
        struct tm *timeinfo = localtime(&now);
        logline = sdscatprintf(logline, "%02d:%02d:%02d ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    }
    logline = sdscatprintf(logline, "%-8s %-10s", loglevel_names[level], thread_logname);
    if (loglevel == 4) {
        logline = sdscatprintf(logline, "%s:%d: ", file, line);
    }

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
    logline = sdscat(logline, "\033[0m");
    
    fputs(logline, stderr);
    fflush(stderr);
    sdsfree(logline);
}
