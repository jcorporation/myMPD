/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief UTF8 wrapper functions
 */

#include "compile_time.h"
#include "src/lib/utf8_wrapper.h"

#include <string.h>
#include <utf8proc.h>

/**
 Normalization flags to use
 */
static utf8proc_option_t utf8_wrap_normalize_flags = UTF8PROC_STABLE | UTF8PROC_COMPOSE | UTF8PROC_COMPAT | UTF8PROC_STRIPMARK | UTF8PROC_CASEFOLD | UTF8PROC_LUMP | UTF8PROC_STRIPNA;

/**
 * Checks if string is valid utf8
 * @param str String to validate
 * @param len String length
 * @return true if string is valid, else false
 */
bool utf8_wrap_validate(const char *str, size_t len) {
    utf8proc_uint8_t *fold_str;
    utf8proc_ssize_t rc = utf8proc_map((utf8proc_uint8_t *)str, (utf8proc_ssize_t)len, &fold_str, UTF8PROC_REJECTNA);
    free(fold_str);
    return rc >= 0;
}

/**
 * Casefolds a string
 * @param str String to normalize
 * @param len String length
 * @return Newly allocated char, caller must free it.
 */
char *utf8_wrap_casefold(const char *str, size_t len) {
    utf8proc_uint8_t *fold_str;
    utf8proc_map((utf8proc_uint8_t *)str, (utf8proc_ssize_t)len, &fold_str, UTF8PROC_CASEFOLD);
    return (char *)fold_str;
}

/**
 * Normalizes a string
 * @param str String to normalize
 * @param len String length
 * @return Newly allocated char, caller must free it.
 */
char *utf8_wrap_normalize(const char *str, size_t len) {
    utf8proc_uint8_t *fold_str;
    utf8proc_map((utf8proc_uint8_t *)str, (utf8proc_ssize_t)len, &fold_str, utf8_wrap_normalize_flags);
    return (char *)fold_str;
}

/**
 * Compares two strings that are normalized before.
 * @param str1 String1
 * @param str1_len String1 length
 * @param str2 String2
 * @param str2_len String2 length
 * @return Same as strcmp
 */
int utf8_wrap_casecmp(const char *str1, size_t str1_len, const char *str2, size_t str2_len) {
    utf8proc_uint8_t *fold_str1;
    utf8proc_map((utf8proc_uint8_t *)str1, (utf8proc_ssize_t)str1_len, &fold_str1, utf8_wrap_normalize_flags);

    utf8proc_uint8_t *fold_str2;
    utf8proc_map((utf8proc_uint8_t *)str2, (utf8proc_ssize_t)str2_len, &fold_str2, utf8_wrap_normalize_flags);

    int rc = strcmp((const char *)fold_str1, (const char *)fold_str2);

    free(fold_str1);
    free(fold_str2);
    return rc;
}
