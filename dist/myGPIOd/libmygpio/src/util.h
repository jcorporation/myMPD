/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myGPIOd (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef LIBMYGPIO_SRC_UTIL_H
#define LIBMYGPIO_SRC_UTIL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef MYGPIOD_DEBUG
    #define LIBMYGPIO_LOG(...) libmygpio_log_log(__FILE__, __LINE__, __VA_ARGS__)
    void libmygpio_log_log(const char *file, int line, const char *fmt, ...);
#else
    #define LIBMYGPIO_LOG(...)
#endif

#endif
