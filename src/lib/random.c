/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/random.h"

#include <limits.h>

tinymt32_t tinymt;

/**
 * Generates random number in range (inclusive lower and upper bounds)
 * This functions uses Mersenne Twister to generate random numbers
 * @param lower lower boundary
 * @param upper upper boundary
 * @return random number
 */
long randrange(long lower, long upper) {
    uint32_t lower_u = (uint32_t)lower;
    uint32_t upper_u = (uint32_t)upper;
    uint32_t r = tinymt32_generate_uint32(&tinymt);
    uint32_t rand = lower_u + r / (UINT32_MAX / (upper_u - lower_u + 1) + 1);
    return (long)rand;
}
