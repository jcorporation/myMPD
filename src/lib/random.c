/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "random.h"

#include <limits.h>

tinymt32_t tinymt;

unsigned randrange_u(unsigned lower, unsigned upper) {
    uint32_t r = tinymt32_generate_uint32(&tinymt);
    unsigned rand = lower + r / (UINT32_MAX / (upper - lower + 1) + 1);
    return rand;
}

long randrange(long lower, long upper) {
    uint32_t r = tinymt32_generate_uint32(&tinymt);
    unsigned rand = lower + r / (UINT32_MAX / (upper - lower + 1) + 1);
    return rand;
}
