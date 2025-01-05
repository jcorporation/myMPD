/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Type conversion functions
 */

#ifndef MYMPD_CONVERT_H
#define MYMPD_CONVERT_H

#include <stdint.h>

/**
 * Return codes for conversion functions
 */
enum str2int_errno {
    STR2INT_SUCCESS,
    STR2INT_OVERFLOW,
    STR2INT_UNDERFLOW,
    STR2INT_INCONVERTIBLE
};

enum str2int_errno str2int(int *out, const char *s);
enum str2int_errno str2uint(unsigned *out, const char *s);
enum str2int_errno str2int64(int64_t *out, const char *s);
enum str2int_errno str2float(float *out, const char *s);

#endif
