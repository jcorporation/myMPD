/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __LOG_H__
#define __LOG_H__

#include <stdbool.h>
#include <syslog.h>

#define MYMPD_LOG_EMERG(...) mympd_log(LOG_EMERG, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_ALERT(...) mympd_log(LOG_ALERT, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_CRIT(...) mympd_log(LOG_CRIT, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_ERROR(...) mympd_log(LOG_ERR, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_WARN(...) mympd_log(LOG_WARNING,  __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_NOTICE(...) mympd_log(LOG_NOTICE, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_INFO(...) mympd_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define MYMPD_LOG_DEBUG(...) mympd_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

extern int loglevel;
extern bool log_on_tty;

void mympd_log(int level, const char *file, int line, const char *fmt, ...);
void set_loglevel(int level);

#endif
