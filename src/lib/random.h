/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Cryptographic and fast random number generator functions
 */

#ifndef MYMPD_RANDOM_H
#define MYMPD_RANDOM_H

#include <stddef.h>

unsigned randrange(unsigned lower, unsigned upper);
unsigned randrange_fast(unsigned lower, unsigned upper);
char randchar(void);
void randstring(char *buffer, size_t len);

#endif
