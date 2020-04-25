/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <limits.h>

#include "random.h"

int randrange(int lower, int upper) {
    unsigned rand = tinymt32_generate_uint32(&tinymt);
    
    return lower + rand / (UINT_MAX / (upper - lower + 1) + 1);
    
//    return (rand % (upper - lower + 1)) + lower;
}
