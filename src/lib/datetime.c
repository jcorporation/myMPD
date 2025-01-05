/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Date and time functions
 */

#define _XOPEN_SOURCE

#include "compile_time.h"
#include "src/lib/datetime.h"
#include "src/lib/log.h"

#include <stdio.h>
#include <string.h>

/**
 * Parses a YYYY-MM-DD string to a unix timestamp
 * @param str string to parse
 * @return time_t unix timestamp
 */
time_t parse_date(const char *str) {
    struct tm tm;
    memset(&tm, '\0', sizeof (tm));
    if (strptime(str, "%Y-%m-%d", &tm) == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can not parse date string: \"%s\"", str);
        return 0;
    }
    return mktime(&tm);
}

/**
 * Appends a formated time to the buffer
 * @param buf already allocated sds string
 * @param timestamp timestamp to display
 */
void readable_time(char *buf, time_t timestamp) {
    if (timestamp == 0) {
        buf[0] = '0';
        buf[1] = '\0';
    }
    else {
        struct tm *tmp = localtime(&timestamp);
        (void)strftime(buf, 32, "%Y-%m-%dT%H:%M:%SZ", tmp);
    }
}
