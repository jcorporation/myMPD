/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "random.h"

#include <limits.h>

tinymt32_t tinymt;

//generates random number in range (inclusive lower and upper bounds)
long randrange(long lower, long upper) {
    uint32_t r = tinymt32_generate_uint32(&tinymt);
    unsigned rand = lower + r / (UINT32_MAX / (upper - lower + 1) + 1);
    return rand;
}
