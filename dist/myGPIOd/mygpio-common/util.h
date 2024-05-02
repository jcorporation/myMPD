/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myGPIOd (c) 2020-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYGPIO_COMMON_UTIL_H
#define MYGPIO_COMMON_UTIL_H

#include <stdbool.h>
#include <stdint.h>

bool mygpio_parse_int(const char *str, int *result, char **rest, int min, int max);
bool mygpio_parse_uint(const char *str, unsigned *result, char **rest, unsigned min, unsigned max);
bool mygpio_parse_ulong(const char *str, unsigned long *result, char **rest, unsigned long min, unsigned long max);
bool mygpio_parse_uint64(const char *str, uint64_t *result, char **rest, uint64_t min, uint64_t max);
bool mygpio_parse_bool(const char *str);
const char *mygpio_bool_to_str(bool v);
#endif
