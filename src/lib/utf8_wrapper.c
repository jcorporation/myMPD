/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief UTF8 wrapper functions
 */

#include "compile_time.h"
#include "src/lib/mem.h"
#include "src/lib/utf8_wrapper.h"

#include <assert.h>
#include <string.h>

#ifdef MYMPD_ENABLE_UTF8
    #include <utf8proc.h>

    /**
    * Normalization flags to use
    * https://juliastrings.github.io/utf8proc/doc/utf8proc_8h.html#a0a18a541ba5bedeb5c3e150024063c2d
    */
    static utf8proc_option_t normalize_flags =
        UTF8PROC_STABLE |
        UTF8PROC_COMPAT |
        UTF8PROC_COMPOSE |
        UTF8PROC_IGNORE |
        UTF8PROC_STRIPCC |
        UTF8PROC_CASEFOLD |
        UTF8PROC_LUMP |
        UTF8PROC_STRIPMARK |
        UTF8PROC_STRIPNA;
#else
    #include <ctype.h>
#endif

/**
 * Checks if string is valid utf8
 * @param str String to validate
 * @param len String length
 * @return true if string is valid, else false
 */
bool utf8_wrap_validate(const char *str, size_t len) {
    assert(str);
    #ifdef MYMPD_ENABLE_UTF8
        utf8proc_uint8_t *fold_str;
        utf8proc_ssize_t rc = utf8proc_map((utf8proc_uint8_t *)str, (utf8proc_ssize_t)len, &fold_str, UTF8PROC_REJECTNA);
        FREE_PTR(fold_str);
        return rc >= 0;
    #else
        (void) str;
        (void) len;
        return true;
    #endif
}

/**
 * Casefolds a string
 * @param str String to normalize
 * @param len String length
 * @return Newly allocated char, caller must free it.
 */
char *utf8_wrap_casefold(const char *str, size_t len) {
    assert(str);
    #ifdef MYMPD_ENABLE_UTF8
        utf8proc_uint8_t *fold_str;
        utf8proc_map((utf8proc_uint8_t *)str, (utf8proc_ssize_t)len, &fold_str, UTF8PROC_CASEFOLD);
        assert(fold_str);
        return (char *)fold_str;
    #else
        char *lower = malloc_assert(len + 1);
        for (size_t i = 0; i < len; i++) {
            lower[i] = (char)tolower(str[i]);
        }
        lower[len] = '\0';
        return lower;
    #endif
}

/**
 * Normalizes a string
 * @param str String to normalize
 * @param len String length
 * @return Newly allocated char, caller must free it.
 */
char *utf8_wrap_normalize(const char *str, size_t len) {
    assert(str);
    #ifdef MYMPD_ENABLE_UTF8
        utf8proc_uint8_t *fold_str;
        utf8proc_map((utf8proc_uint8_t *)str, (utf8proc_ssize_t)len, &fold_str, normalize_flags);
        assert(fold_str);
        return (char *)fold_str;
    #else
        return utf8_wrap_casefold(str, len);
    #endif
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
    assert(str1);
    assert(str2);
    #ifdef MYMPD_ENABLE_UTF8
        utf8proc_uint8_t *fold_str1;
        utf8proc_map((utf8proc_uint8_t *)str1, (utf8proc_ssize_t)str1_len, &fold_str1, normalize_flags);
        assert(fold_str1);

        utf8proc_uint8_t *fold_str2;
        utf8proc_map((utf8proc_uint8_t *)str2, (utf8proc_ssize_t)str2_len, &fold_str2, normalize_flags);
        assert(fold_str2);

        int rc = strcmp((const char *)fold_str1, (const char *)fold_str2);

        FREE_PTR(fold_str1);
        FREE_PTR(fold_str2);
    #else
        (void) str1_len;
        (void) str2_len;
        int rc = strcasecmp(str1, str2);
    #endif
    return rc;
}
