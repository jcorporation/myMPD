/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_RANDOM_H
#define MYMPD_RANDOM_H

#include "dist/tinymt/tinymt32.h"

extern tinymt32_t tinymt;

long randrange(long lower, long upper);
#endif
