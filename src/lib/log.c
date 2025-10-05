/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Log implementation
 */

#include "compile_time.h"
#include "src/lib/log.h"

#include "src/lib/env.h"
#include "src/lib/sds_extras.h"

#include <pthread.h>
#include <string.h>
#include <unistd.h>

/**
 * Global variables
 */

/**
 * Thread name
 */
_Thread_local sds thread_logname;

/**
 * Loglevel
 */
_Atomic int loglevel;

/**
 * Type of logging system
 */
enum log_types log_type;

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
    [LOG_DEBUG] = "\033[0;32m"
};

/**
 * Returns the name of the loglevel
 * @param level 
 * @return the loglevel name or empty string if loglevel is invalid
 */
const char *get_loglevel_name(int level) {
    if (level > LOGLEVEL_MAX ||
        level < LOGLEVEL_MIN)
    {
        return "";
    }
    return loglevel_names[level];
}

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
    MYMPD_LOG_NOTICE(NULL, "Setting loglevel to %s", loglevel_names[level]);
    loglevel = level;
}

/**
 * Initializes the logging sub-system
 */
void log_init(void) {
    log_type = LOG_TO_STDOUT;
    if (isatty(fileno(stdout)) == true) {
        log_type = LOG_TO_TTY;
    }
    else if (getenv_check("INVOCATION_ID") != NULL) {
        log_type = LOG_TO_SYSTEMD;
    }
    #ifdef MYMPD_DEBUG
        set_loglevel(LOG_DEBUG);
    #else
        bool getenv_rc;
        set_loglevel(
            getenv_int("MYMPD_LOGLEVEL", CFG_MYMPD_LOGLEVEL, LOGLEVEL_MIN, LOGLEVEL_MAX, &getenv_rc)
        );
    #endif
}

/**
 * Logs the errno string
 * This function should be called by the suitable macro
 * @param file filename for debug logging
 * @param line linenumber for debug logging
 * @param partition mpd partition
 * @param errnum errno
 */
void mympd_log_errno(const char *file, int line, const char *partition, int errnum) {
    if (errnum == 0) {
        //do not log success
        return;
    }
    char err_text[256];
    int rc = strerror_r(errnum, err_text, 256);
    const char *err_str = rc == 0
        ? err_text
        : "Unknown error";
    mympd_log(LOG_ERR, file, line, partition, "%s", err_str);
}

/**
 * Logs the errno string
 * This function should be called by the suitable macro
 * @param level loglevel of the message
 * @param file filename for debug logging
 * @param line linenumber for debug logging
 * @param partition mpd partition
 * @param fmt format string to print
 * @param ... arguments for the format string
 */
void mympd_log(int level, const char *file, int line, const char *partition, const char *fmt, ...) {
    if (level > loglevel) {
        return;
    }

    if (log_type == LOG_TO_SYSLOG) {
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
    if (log_type == LOG_TO_TTY) {
        logline = sdscat(logline, loglevel_colors[level]);
        time_t now = time(NULL);
        struct tm timeinfo;
        if (localtime_r(&now, &timeinfo) != NULL) {
            logline = sdscatprintf(logline, "%02d:%02d:%02d ", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
    }
    else if (log_type == LOG_TO_SYSTEMD) {
        logline = sdscatfmt(logline, "<%d>", level);
    }
    logline = sdscatprintf(logline, "%-8s %-11s", loglevel_names[level], thread_logname);
    #ifdef MYMPD_DEBUG
        logline = sdscatfmt(logline, "%s:%i: ", file, line);
    #else
        (void)file;
        (void)line;
    #endif
    if (partition != NULL) {
        logline = sdscatfmt(logline, "\"%s\": ", partition);
    }

    va_list args;
    va_start(args, fmt);
    logline = sdscatvprintf(logline, fmt, args);
    va_end(args);

    if (sdslen(logline) > 1023) {
        sdsrange(logline, 0, 1020);
        logline = sdscatlen(logline, "...", 3);
    }

    if (log_type == LOG_TO_TTY) {
        logline = sdscat(logline, "\033[0m\n");
    }
    else {
        logline = sdscatlen(logline, "\n", 1);
    }

    (void) fputs(logline, stdout);
    FREE_SDS(logline);
}
