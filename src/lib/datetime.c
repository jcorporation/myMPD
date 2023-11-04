/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _XOPEN_SOURCE

#include "compile_time.h"
#include "src/lib/datetime.h"
#include "src/lib/log.h"

#include <string.h>

/**
 * Parses a YYYY-MM-DD string to a unix timestamp
 * @param str string to parse
 * @return time_t unix timestamp
 */
time_t parse_date(const char *str) {
    struct tm tm;
    memset(&tm, '\0', sizeof (tm));
    if (strptime (str, "%Y-%m-%d", &tm) == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can not parse date string");
        return 0;
    }
    return mktime(&tm);
}
