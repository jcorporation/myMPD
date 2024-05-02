/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myGPIOd (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "libmygpio/src/util.h"

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef MYGPIOD_DEBUG
/**
 * Debug logging function. Do not call it directly, use the LIBMYGPIO_LOG macro.
 * @param file file of the log statement
 * @param line line number of the log statement
 * @param fmt format string
 * @param ... variadic arguments for format string
 */
void libmygpio_log_log(const char *file, int line, const char *fmt, ...) {
    printf("%s:%d: ", file, line);
    va_list args;
    va_start(args, fmt);
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    vprintf(fmt, args);
    va_end(args);
    #pragma GCC diagnostic pop
    printf("\n");
}
#endif
