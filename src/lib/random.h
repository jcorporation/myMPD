/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_RANDOM_H
#define MYMPD_RANDOM_H

#include <inttypes.h>

long randrange(long lower, long upper);
uint64_t randrange64(uint64_t lower, uint64_t upper);
#endif
