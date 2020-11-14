/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <limits.h>

#include "random.h"

tinymt32_t tinymt;

unsigned randrange(unsigned lower, unsigned upper) {
    unsigned r = tinymt32_generate_uint32(&tinymt);
    unsigned rand = lower + r / (UINT_MAX / (upper - lower + 1) + 1);
    return rand;
}
