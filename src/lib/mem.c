/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Memory allocation related functions
 */

#include "compile_time.h"
#include "src/lib/mem.h"

#include <string.h>

/**
 * Duplicate a string and enforces NULL-termination
 * Aborts on malloc failure
 * @param str String to copy
 * @param len Length of string to copy
 * @return NULL-terminated copy of str
 */
char *my_strdup(const char *str, size_t len) { 
    char *p = malloc_assert(len + 1);
    memcpy(p, str, len);
    p[len] = '\0';
    return p;
}
