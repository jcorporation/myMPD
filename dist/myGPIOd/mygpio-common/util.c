/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myGPIOd (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "mygpio-common/util.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/**
 * Parses a string to an integer value and checks it against min and max.
 * If rest is specified as NULL the complete string must be a number.
 * @param str string to parse
 * @param result pointer for the result
 * @param rest pointer to first none numeric char
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @return bool true on success, else false
 */
bool mygpio_parse_int(const char *str, int *result, char **rest, int min, int max) {
    if (str == NULL ||
        str[0] == '\0' ||
        isspace(str[0]))
    {
        return false;
    }
    errno = 0;
    char *endptr;
    long v = strtol(str, &endptr, 10);
    if (v > INT_MAX ||
        v < INT_MIN)
    {
        return false;
    }
    if (errno == 0 &&     // no error returned
        endptr != str &&  // parsed some chars
        v >= min &&       // enforce limit
        v <= max)         // enforce limit
    {
        if (rest == NULL) {
            // strict mode
            if (*endptr != '\0') {
                return false;
            }
        }
        else {
            *rest = endptr;
        }
        *result = (int)v;
        return true;
    }
    return false;
}

/**
 * Parses a string to an unsigned value and checks it against min and max.
 * If rest is specified as NULL the complete string must be a number.
 * @param str string to parse
 * @param result pointer for the result
 * @param rest pointer to first none numeric char
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @return bool true on success, else false
 */
bool mygpio_parse_uint(const char *str, unsigned *result, char **rest, unsigned min, unsigned max) {
    if (str == NULL ||
        str[0] == '\0' ||
        isspace(str[0]))
    {
        return false;
    }
    if (str[0] == '-') {
        return false;
    }
    errno = 0;
    char *endptr;
    unsigned long v = strtoul(str, &endptr, 10);
    if (v > UINT_MAX) {
        return false;
    }
    if (errno == 0 &&     // no error returned
        endptr != str &&  // parsed some chars
        v >= min &&       // enforce limit
        v <= max)         // enforce limit
    {
        if (rest == NULL) {
            // strict mode
            if (*endptr != '\0') {
                return false;
            }
        }
        else {
            *rest = endptr;
        }
        *result = (unsigned)v;
        return true;
    }
    return false;
}

/**
 * Parses a string to an unsigned long value and checks it against min and max.
 * If rest is specified as NULL the complete string must be a number.
 * @param str string to parse
 * @param result pointer for the result
 * @param rest pointer to first none numeric char
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @return bool true on success, else false
 */
bool mygpio_parse_ulong(const char *str, unsigned long *result, char **rest, unsigned long min, unsigned long max) {
    if (str == NULL ||
        str[0] == '\0' ||
        isspace(str[0]))
    {
        return false;
    }
    if (str[0] == '-') {
        return false;
    }
    errno = 0;
    char *endptr;
    unsigned long v = strtoul(str, &endptr, 10);
    if (errno == 0 &&     // no error returned
        endptr != str &&  // parsed some chars
        v >= min &&       // enforce limit
        v <= max)         // enforce limit
    {
        if (rest == NULL) {
            // strict mode
            if (*endptr != '\0') {
                return false;
            }
        }
        else {
            *rest = endptr;
        }
        *result = (unsigned)v;
        return true;
    }
    return false;
}

/**
 * Parses the start of a string to an uint64_t value.
 * @param str string to parse
 * @param result pointer for the result
 * @param rest pointer to first none numeric char
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @return bool true on success, else false
 */
bool mygpio_parse_uint64(const char *str, uint64_t *result, char **rest, uint64_t min, uint64_t max) {
    if (str == NULL ||
        str[0] == '\0' ||
        isspace(str[0]))
    {
        return false;
    }
    if (str[0] == '-') {
        return false;
    }
    errno = 0;
    char *endptr;
    unsigned long long v = strtoull(str, &endptr, 10);
    if (errno == 0 &&     // no error returned
        endptr != str &&  // parsed some chars
        v >= min &&       // enforce limit
        v <= max)         // enforce limit
    {
        if (rest == NULL) {
            // strict mode
            if (*endptr != '\0') {
                return false;
            }
        }
        else {
            *rest = endptr;
        }
        *result = (uint64_t)v;
        return true;
    }
    return false;
}

/**
 * Parses a string to a boolean value.
 * Sets errno to EINVAL on parser error.
 * Returns false if string is not true.
 * @param str string to parse
 * @return parsed value
 */
bool mygpio_parse_bool(const char *str) {
    if (strcasecmp(str, "true") == 0 ||
        strcmp(str, "1") == 0)
    {
        return true;
    }
    if (strcasecmp(str, "false") == 0 ||
        strcmp(str, "0") == 0)
    {
        return false;
    }
    errno = EINVAL;
    return false;
}

/**
 * Prints a bool value as string
 * @param v the bool value
 * @return string
 */
const char *mygpio_bool_to_str(bool v) {
    return v == true
        ? "true"
        : "false";
}
