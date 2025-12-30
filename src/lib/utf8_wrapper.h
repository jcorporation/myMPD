/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief UTF8 wrapper functions
 */

#ifndef MYMPD_LIB_UTF8_WRAPPER_H
#define MYMPD_LIB_UTF8_WRAPPER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool utf8_wrap_validate(const char *str, size_t len);
char *utf8_wrap_casefold(const char *str, size_t len);
char *utf8_wrap_normalize(const char *str, size_t len);
int utf8_wrap_casecmp(const char *str1, size_t str1_len, const char *str2, size_t str2_len);

#endif
