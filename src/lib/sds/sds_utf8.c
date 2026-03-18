/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief UTF8 functions for sds strings
 */

#include "compile_time.h"
#include "src/lib/sds/sds_utf8.h"

#include "dist/sds/sds.h"
#include "src/lib/utf8_wrapper.h"

#include <stdlib.h>

/**
 * Casefolds the sds string
 * @param s sds string to modify in place
 * @return pointer to s
 */
sds sds_utf8_casefold(sds s) {
    size_t newlen;
    char *fold_str = utf8_wrap_casefold(s, sdslen(s), &newlen);
    sdsclear(s);
    s = sdscatlen(s, fold_str, newlen);
    free(fold_str);
    return s;
}

/**
 * Normalizes the sds string
 * @param s sds string to modify in place
 * @return pointer to s
 */
sds sds_utf8_normalize(sds s) {
    size_t newlen;
    char *fold_str = utf8_wrap_normalize(s, sdslen(s), &newlen);
    sdsclear(s);
    s = sdscatlen(s, fold_str, newlen);
    free(fold_str);
    return s;
}
