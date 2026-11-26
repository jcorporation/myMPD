/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Thread-local xoshiro256** non-cryptographic random number generator
 */

#ifndef MYMPD_RANDOM__XOSHIRO256H
#define MYMPD_RANDOM__XOSHIRO256H

#include <inttypes.h>

/**
 * Returns the next 64-bit value from the thread-local xoshiro256** PRNG.
 * The PRNG is lazily seeded with OpenSSL randomness and is not suitable for
 * cryptographic purposes.
 * @return next pseudo-random 64-bit value
 */
uint64_t fast_random64(void);

#endif
