/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __LOG_H__
#define __LOG_H__

#include <stdbool.h>
#include <syslog.h>

#ifdef DEBUG
    #define MYMPD_LOG_EMERG(...) mympd_log(LOG_EMERG, __FILE__, __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_ALERT(...) mympd_log(LOG_ALERT, __FILE__, __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_CRIT(...) mympd_log(LOG_CRIT, __FILE__, __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_ERROR(...) mympd_log(LOG_ERR, __FILE__, __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_WARN(...) mympd_log(LOG_WARNING,  __FILE__, __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_NOTICE(...) mympd_log(LOG_NOTICE, __FILE__, __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_INFO(...) mympd_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_DEBUG(...) mympd_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_ERRNO(ERRNUM) mympd_log_errno(__FILE__, __LINE__, ERRNUM)
#else
    //release build should have no references to build dir
    #define MYMPD_LOG_EMERG(...) mympd_log(LOG_EMERG, "", __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_ALERT(...) mympd_log(LOG_ALERT, "", __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_CRIT(...) mympd_log(LOG_CRIT, "", __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_ERROR(...) mympd_log(LOG_ERR, "", __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_WARN(...) mympd_log(LOG_WARNING, "", __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_NOTICE(...) mympd_log(LOG_NOTICE, "", __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_INFO(...) mympd_log(LOG_INFO, "", __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_DEBUG(...) mympd_log(LOG_DEBUG, "", __LINE__, __VA_ARGS__)
    #define MYMPD_LOG_ERRNO(ERRNUM) mympd_log_errno("", __LINE__, ERRNUM)
#endif

extern int loglevel;
extern bool log_on_tty;

void mympd_log_errno(const char *file, int line, int errnum);
void mympd_log(int level, const char *file, int line, const char *fmt, ...);
void set_loglevel(int level);

#endif
