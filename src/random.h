/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __RANDOM_H__
#define __RANDOM_H__

#include "../dist/src/tinymt/tinymt32.h"

tinymt32_t tinymt;

int randrange(int lower, int upper);
#endif
