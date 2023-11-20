/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/convert.h"

#include "src/lib/log.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdlib.h>

/**
 * Convert string s to int out.
 * The format is the same as strtol, except that the following are inconvertible:
 * NULL, empty string, leading whitespace, any trailing characters
 * @param out pointer to integer
 * @param s string to convert
 * @return Indicates if the operation succeeded, or why it failed.
 */
enum str2int_errno str2int(int *out, const char *s) {
    if (s == NULL ||
        s[0] == '\0' ||
        isspace(s[0]))
    {
        MYMPD_LOG_ERROR(NULL, "Inconvertible string");
        return STR2INT_INCONVERTIBLE;
    }
    char *end;
    errno = 0;
    long l = strtoimax(s, &end, 10);
    /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
    if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX)) {
        MYMPD_LOG_ERROR(NULL, "Integer overflow");
        return STR2INT_OVERFLOW;
    }
    if (l < INT_MIN ||
        (errno == ERANGE && l == LONG_MIN))
    {
        MYMPD_LOG_ERROR(NULL, "Integer underflow");
        return STR2INT_UNDERFLOW;
    }
    if (*end != '\0') {
        MYMPD_LOG_ERROR(NULL, "Inconvertible string");
        return STR2INT_INCONVERTIBLE;
    }
    *out = (int)l;
    return STR2INT_SUCCESS;
}

/**
 * Convert string s to unsigned out.
 * The format is the same as strtol, except that the following are inconvertible:
 * NULL, empty string, leading whitespace, any trailing characters
 * @param out pointer to integer
 * @param s string to convert
 * @return Indicates if the operation succeeded, or why it failed.
 */
enum str2int_errno str2uint(unsigned *out, const char *s) {
    if (s == NULL ||
        s[0] == '\0' ||
        isspace(s[0]))
    {
        MYMPD_LOG_ERROR(NULL, "Inconvertible string");
        return STR2INT_INCONVERTIBLE;
    }
    if (s[0] == '-') {
        MYMPD_LOG_ERROR(NULL, "Integer underflow");
        return STR2INT_UNDERFLOW;
    }
    errno = 0;
    char *end;
    unsigned long l = strtoumax(s, &end, 10);
    /* Both checks are needed because UINT_MAX == ULONG_MAX is possible. */
    if (l > UINT_MAX ||
        (errno == ERANGE && l == ULONG_MAX))
    {
        MYMPD_LOG_ERROR(NULL, "Integer overflow");
        return STR2INT_OVERFLOW;
    }
    if (errno == ERANGE) {
        MYMPD_LOG_ERROR(NULL, "Integer underflow");
        return STR2INT_UNDERFLOW;
    }
    if (*end != '\0') {
        MYMPD_LOG_ERROR(NULL, "Inconvertible string");
        return STR2INT_INCONVERTIBLE;
    }
    *out = (unsigned)l;
    return STR2INT_SUCCESS;
}

/**
 * Convert string s to int64_t out.
 * The format is the same as strtoll, except that the following are inconvertible:
 * NULL, empty string, leading whitespace, any trailing characters
 * @param out pointer to integer
 * @param s string to convert
 * @return Indicates if the operation succeeded, or why it failed.
 */
enum str2int_errno str2int64(int64_t *out, const char *s) {
    if (s == NULL ||
        s[0] == '\0' ||
        isspace(s[0]))
    {
        MYMPD_LOG_ERROR(NULL, "Inconvertible string");
        return STR2INT_INCONVERTIBLE;
    }
    errno = 0;
    char *end;
    long long l = strtoll(s, &end, 10);
    /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
    if (l > INT64_MAX || (errno == ERANGE && l == LLONG_MAX)) {
        MYMPD_LOG_ERROR(NULL, "Integer overflow");
        return STR2INT_OVERFLOW;
    }
    if (l < INT64_MIN ||
        (errno == ERANGE && l == LLONG_MIN))
    {
        MYMPD_LOG_ERROR(NULL, "Integer underflow");
        return STR2INT_UNDERFLOW;
    }
    if (*end != '\0') {
        MYMPD_LOG_ERROR(NULL, "Inconvertible string");
        return STR2INT_INCONVERTIBLE;
    }
    *out = (int)l;
    return STR2INT_SUCCESS;
}

/**
 * Convert string s to float out.
 * The format is the same as strtof, except that the following are inconvertible:
 * NULL, empty string, leading whitespace, any trailing characters
 * @param out pointer to integer
 * @param s string to convert
 * @return Indicates if the operation succeeded, or why it failed.
 */
enum str2int_errno str2float(float *out, const char *s) {
    if (s == NULL ||
        s[0] == '\0' ||
        isspace(s[0]))
    {
        MYMPD_LOG_ERROR(NULL, "Inconvertible string");
        return STR2INT_INCONVERTIBLE;
    }
    errno = 0;
    char *end;
    double l = strtod(s, &end);
    if (l > FLT_MAX) {
        MYMPD_LOG_ERROR(NULL, "Float overflow");
        return STR2INT_OVERFLOW;
    }
    if (l < -FLT_MAX) {
        MYMPD_LOG_ERROR(NULL, "Float underflow");
        return STR2INT_UNDERFLOW;
    }
    if (*end != '\0') {
        MYMPD_LOG_ERROR(NULL, "Inconvertible string");
        return STR2INT_INCONVERTIBLE;
    }
    *out = (float)l;
    return STR2INT_SUCCESS;
}
