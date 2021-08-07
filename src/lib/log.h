/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __LOG_H__
#define __LOG_H__

#include <stdbool.h>
#include <syslog.h>

#include "../../dist/src/sds/sds.h"

#define MYMPD_LOG_EMERG(...) mympd_log(LOG_EMERG, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_ALERT(...) mympd_log(LOG_ALERT, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_CRIT(...) mympd_log(LOG_CRIT, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_ERROR(...) mympd_log(LOG_ERR, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_WARN(...) mympd_log(LOG_WARNING,  __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_NOTICE(...) mympd_log(LOG_NOTICE, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_INFO(...) mympd_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_DEBUG(...) mympd_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

#define MYMPD_LOG_ERRNO(ERRNUM) mympd_log_errno(__FILE__, __LINE__, ERRNUM)

extern int loglevel;
extern bool log_on_tty;
extern bool log_to_syslog;
_Thread_local extern sds thread_logname;

void mympd_log_errno(const char *file, int line, int errnum);
void mympd_log(int level, const char *file, int line, const char *fmt, ...);
void set_loglevel(int level);

#endif
