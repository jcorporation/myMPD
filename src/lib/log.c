/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/log.h"

#include "src/lib/sds_extras.h"

#include <pthread.h>
#include <string.h>

//global variables
_Thread_local sds thread_logname;
_Atomic int loglevel;
bool log_to_syslog;
bool log_on_tty;

/**
 * Maps loglevels to names
 */
static const char *loglevel_names[8] = {
    [LOG_EMERG] = "EMERG",
    [LOG_ALERT] = "ALERT",
    [LOG_CRIT] = "CRITICAL",
    [LOG_ERR] = "ERROR",
    [LOG_WARNING] = "WARN",
    [LOG_NOTICE] = "NOTICE",
    [LOG_INFO] = "INFO",
    [LOG_DEBUG] = "DEBUG"
};

/**
 * Maps loglevels to terminal colors
 */
static const char *loglevel_colors[8] = {
    [LOG_EMERG] = "\033[0;31m",
    [LOG_ALERT] = "\033[0;31m",
    [LOG_CRIT] = "\033[0;31m",
    [LOG_ERR] = "\033[0;31m",
    [LOG_WARNING] = "\033[0;33m",
    [LOG_NOTICE] = "",
    [LOG_INFO] = "",
    [LOG_DEBUG] = "\033[0;34m"
};

/**
 * Sets the loglevel
 * @param level loglevel to set
 */
void set_loglevel(int level) {
    if (level > LOGLEVEL_MAX) {
        level = 7;
    }
    else if (level < LOGLEVEL_MIN) {
        level = 0;
    }
    MYMPD_LOG_NOTICE("Setting loglevel to %s", loglevel_names[level]);
    loglevel = level;
}

/**
 * Logs the errno string
 * This function should be called by the suitable macro
 * @param file filename for debug logging
 * @param line linenumber for debug logging
 * @param errnum errno
 */
void mympd_log_errno(const char *file, int line, int errnum) {
    char err_text[256];
    int rc = strerror_r(errnum, err_text, 256);
    const char *err_str = rc == 0 ? err_text : "Unknown error";
    mympd_log(LOG_ERR, file, line, "%s", err_str);
}

/**
 * Logs the errno string
 * This function should be called by the suitable macro
 * @param level loglevel of the message
 * @param file filename for debug logging
 * @param line linenumber for debug logging
 * @param fmt format string to print
 * @param ... arguments for the format string
 */
void mympd_log(int level, const char *file, int line, const char *fmt, ...) {
    if (level > loglevel) {
        return;
    }

    if (log_to_syslog == true) {
        va_list args;
        va_start(args, fmt);
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wformat-nonliteral"
            vsyslog(level, fmt, args); // NOLINT(clang-diagnostic-format-nonliteral)
        #pragma GCC diagnostic pop
        va_end(args);
        return;
    }

    sds logline = sdsempty();
    //preallocate some space for the logline to avoid continuous reallocations
    logline = sdsMakeRoomFor(logline, 512);
    if (log_on_tty == true) {
        logline = sdscat(logline, loglevel_colors[level]);
        time_t now = time(NULL);
        struct tm timeinfo;
        if (localtime_r(&now, &timeinfo) != NULL) {
            logline = sdscatprintf(logline, "%02d:%02d:%02d ", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
    }
    logline = sdscatprintf(logline, "%-8s %-10s", loglevel_names[level], thread_logname);
    #ifdef DEBUG
        logline = sdscatfmt(logline, "%s:%i: ", file, line);
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
        logline = sdscatlen(logline, "...", 3);
    }

    if (log_on_tty == true) {
        logline = sdscat(logline, "\033[0m\n");
    }
    else {
        logline = sdscatlen(logline, "\n", 1);
    }

    (void) fputs(logline, stdout);
    FREE_SDS(logline);
}
