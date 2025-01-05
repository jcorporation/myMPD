/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Random number generator functions based on OpenSSL
 */

#ifndef MYMPD_RANDOM_H
#define MYMPD_RANDOM_H

#include <inttypes.h>
#include <stddef.h>

unsigned randrange(unsigned lower, unsigned upper);
char randchar(void);
void randstring(char *buffer, size_t len);

#endif
