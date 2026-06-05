/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief UTF8 wrapper functions
 */

#include "compile_time.h"
#include "src/lib/mem.h"
#include "src/lib/utf8_wrapper.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>


// Private definitions

#ifdef MYMPD_ENABLE_UTF8
    #include <errno.h>
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
    static utf8proc_option_t casefold_flags = UTF8PROC_CASEFOLD;
#else
    #include <ctype.h>

    typedef int utf8proc_option_t;
    static utf8proc_option_t normalize_flags = 0;
    static utf8proc_option_t casefold_flags = 0;
#endif

static char *normalize(const char *str, size_t len, size_t *newlen, utf8proc_option_t flags);

// Public functions

/**
 * Checks if string is valid utf8
 * @param str String to validate
 * @param str_len String length
 * @return true if string is valid, else false
 */
bool utf8_wrap_validate(const char *str, size_t str_len) {
    assert(str);
    assert(str_len < INT_MAX);
    #ifdef MYMPD_ENABLE_UTF8
        const utf8proc_uint8_t *utf8_str = (const utf8proc_uint8_t *)str;
        const utf8proc_ssize_t len = (utf8proc_ssize_t)str_len;
        utf8proc_ssize_t pos = 0;
        utf8proc_int32_t codepoint;
        while (pos < len) {
            // Decode codepoint from utf8_str
            pos += utf8proc_iterate(&utf8_str[pos], len - pos, &codepoint);
            if (codepoint < 0) {
                // Invalid UTF-8
                return false;
            }
        }
        return true;
    #else
        (void) str;
        (void) str_len;
        return true;
    #endif
}

/**
 * Casefolds a string
 * @param str String to normalize
 * @param len String length
 * @param newlen Pointer to size_t for the new string length
 * @return Newly allocated char, caller must free it.
 */
char *utf8_wrap_casefold(const char *str, size_t len, size_t *newlen) {
    return normalize(str, len, newlen, casefold_flags);
}

char *utf8_wrap_normalize(const char *str, size_t len, size_t *newlen) {
    return normalize(str, len, newlen, normalize_flags);
}

/**
 * Normalizes a string
 * @param str String to normalize
 * @param len String length
 * @param newlen Pointer to size_t for the new string length
 * @param flags Normalization flags
 * @return Newly allocated char, caller must free it.
 */
static char *normalize(const char *str, size_t len, size_t *newlen, utf8proc_option_t flags) {
    assert(str);
    #ifdef MYMPD_ENABLE_UTF8
        utf8proc_uint8_t *fold_str;
        ssize_t return_len = utf8proc_map((utf8proc_uint8_t *)str, (utf8proc_ssize_t)len, &fold_str, flags);
        if (fold_str == NULL ||
            return_len < 0)
        {
            MYMPD_LOG_WARN(NULL, "Failure in unicode processing of: \"%s\"", str);
            *newlen = len;
            FREE_PTR(fold_str);
            return my_strdup(str, len);
        }
        *newlen = (size_t)return_len;
        return (char *)fold_str;
    #else
        (void) flags;
        char *lower = malloc_assert(len + 1);
        for (size_t i = 0; i < len; i++) {
            lower[i] = (char)tolower(str[i]);
        }
        lower[len] = '\0';
        *newlen = len;
        return lower;
    #endif
}

/**
 * Compares two strings case insensitive.
 * Set's errno to EINVAL if one string is invalid utf8.
 * @param str1 String1
 * @param str1_len String1 length
 * @param str2 String2
 * @param str2_len String2 length
 * @return Same as strcasecmp
 */
int utf8_wrap_casecmp(const char *str1, size_t str1_len, const char *str2, size_t str2_len) {
    assert(str1);
    assert(str2);
    assert(str1_len < INT_MAX);
    assert(str2_len < INT_MAX);
    #ifdef MYMPD_ENABLE_UTF8
        const utf8proc_uint8_t *utf8_str1 = (const utf8proc_uint8_t *)str1;
        const utf8proc_uint8_t *utf8_str2 = (const utf8proc_uint8_t *)str2;
        const utf8proc_ssize_t len1 = (utf8proc_ssize_t)str1_len;
        const utf8proc_ssize_t len2 = (utf8proc_ssize_t)str2_len;

        utf8proc_ssize_t pos1 = 0;
        utf8proc_ssize_t pos2 = 0;

        utf8proc_int32_t codepoint1;
        utf8proc_int32_t codepoint2;

        while (pos1 < len1 && pos2 < len2) {
            pos1 += utf8proc_iterate(&utf8_str1[pos1], len1 - pos1, &codepoint1);
            if (codepoint1 < 0) {
                errno = EINVAL;
                return 0;
            }

            pos2 += utf8proc_iterate(&utf8_str2[pos2], len2 - pos2, &codepoint2);
            if (codepoint2 < 0) {
                errno = EINVAL;
                return 0;
            }

            codepoint1 = utf8proc_tolower(codepoint1);
            codepoint2 = utf8proc_tolower(codepoint2);
            if (codepoint1 != codepoint2) {
                return codepoint1 - codepoint2;
            }
        }

        if (pos1 < len1) {
            return 1;
        }
        if (pos2 < len2) {
            return -1;
        }
        return 0;
    #else
        (void) str1_len;
        (void) str2_len;
        return strcasecmp(str1, str2);
    #endif
}
