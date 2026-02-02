/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Extra functions for sds strings
 */

#include "compile_time.h"
#include "src/lib/sds/sds_extras.h"

#include "dist/sds/sds.h"

#include <string.h>

/**
 * Splits a comma separated string and trims whitespaces from single values
 * @param p string to split
 * @param count pointer to int representing the count of values
 * @return array of sds strings
 */
sds *sds_split_comma_trim(const char *p, int *count) {
    *count = 0;
    sds *values = sdssplitlen(p, (ssize_t)strlen(p), ",", 1, count);
    for (int i = 0; i < *count; i++) {
        sdstrim(values[i], " ");
    }
    return values;
}

/**
 * Appends a char to sds string s, this is faster than using sdscatfmt
 * @param s sds string
 * @param c char to append
 * @return modified sds string
 */
sds sds_catchar(sds s, const char c) {
    // Make sure there is always space for at least 1 char.
    s = sdsMakeRoomFor(s, 1);
    if (s == NULL) {
        return NULL;
    }
    size_t i = sdslen(s);
    s[i++] = c;
    sdsinclen(s, 1);
    // Add null-term
    s[i] = '\0';
    return s;
}

/**
 * Replaces a sds string with a new value,
 * allocates the string if it is NULL
 * @param s sds string to replace
 * @param p replacement string
 * @param len replacement string length
 * @return modified sds string
 */
sds sds_replacelen(sds s, const char *p, size_t len) {
    if (s != NULL) {
        sdsclear(s);
    }
    else {
        s = sdsempty();
    }
    if (p != NULL) {
        s = sdscatlen(s, p, len);
    }
    return s;
}

/**
 * Replaces a sds string with a new value,
 * allocates the string if it is NULL
 * @param s sds string to replace
 * @param p replacement string
 * @return modified sds string
 */
sds sds_replace(sds s, const char *p) {
    return sds_replacelen(s, p, strlen(p));
}

/**
 * Converts a bool value to a sds string
 * @param s string to append the value
 * @param v bool value to convert
 * @return modified sds string
 */
sds sds_catbool(sds s, bool v) {
    return v == true
        ? sdscatlen(s, "true", 4)
        : sdscatlen(s, "false", 5);
}

/**
 * Prints a zero padded value
 * @param value mpd song struct
 * @param buffer already allocated sds string to append
 * @return pointer to buffer
 */
sds sds_pad_int(int64_t value, sds buffer) {
    return sdscatprintf(buffer, "%020" PRId64, value);
}

/**
 * Frees an sds string pointed by void pointer
 * @param p Void pointer to sds string
 */
void sds_free_void(void *p) {
    sdsfree(p);
}
