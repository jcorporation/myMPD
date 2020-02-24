/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../dist/src/sds/sds.h"
#include "log.h"

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
    logline = sdscatprintf(logline, "%-8s ", loglevel_names[level]);

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
