/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __LOG_H__
#define __LOG_H__

enum { LOGLEVEL_ERROR, LOGLEVEL_WARN, LOGLEVEL_INFO, LOGLEVEL_VERBOSE, LOGLEVEL_DEBUG };

#define LOG_ERROR(...) mympd_log(LOGLEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) mympd_log(LOGLEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) mympd_log(LOGLEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_VERBOSE(...) mympd_log(LOGLEVEL_VERBOSE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) mympd_log(LOGLEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

int loglevel;

void mympd_log(int level, const char *file, int line, const char *fmt, ...);
void set_loglevel(int level);

#endif
