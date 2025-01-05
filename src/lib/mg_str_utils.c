/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Functions for mongoose strings
 */

#include "compile_time.h"
#include "src/lib/mg_str_utils.h"

#include "src/lib/convert.h"
#include "src/lib/sds_extras.h"

/**
 * Converts a mg_str to int
 * @param str pointer to struct mg_str
 * @return parsed integer
 */
int mg_str_to_int(const struct mg_str *str) {
    sds s = sdsnewlen(str->buf, str->len);
    int i;
    enum str2int_errno rc = str2int(&i, s);
    FREE_SDS(s);
    return rc == STR2INT_SUCCESS
        ? i
        : 0;
}

/**
 * Converts a mg_str to unsigned int
 * @param str pointer to struct mg_str
 * @return parsed integer
 */
unsigned mg_str_to_uint(const struct mg_str *str) {
    sds s = sdsnewlen(str->buf, str->len);
    unsigned i;
    enum str2int_errno rc = str2uint(&i, s);
    FREE_SDS(s);
    return rc == STR2INT_SUCCESS
        ? i
        : 0;
}
