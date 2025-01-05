/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Date and time functions
 */

#ifndef MYMPD_DATETIME_H
#define MYMPD_DATETIME_H

#include <time.h>

time_t parse_date(const char *str);
void readable_time(char *buf, time_t timestamp);

#endif
