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

#include <inttypes.h>
#include <stddef.h>

/**
 * Generates a cryptographically secure random number in the [lower, upper)
 * range.
 * @param lower lower boundary, inclusive
 * @param upper upper boundary, exclusive
 * @return random number, or zero and errno set to EINVAL on error
 */
unsigned randrange(unsigned lower, unsigned upper);

/**
 * Generates a uniformly distributed random number in the [lower, upper) range
 * using a thread-local non-cryptographic PRNG.
 * @param lower lower boundary, inclusive
 * @param upper upper boundary, exclusive
 * @return random number, or zero and errno set to EINVAL on error
 */
unsigned randrange_fast(unsigned lower, unsigned upper);

/**
 * Returns a cryptographically secure random ASCII alphanumeric character.
 * @return random character
 */
char randchar(void);

/**
 * Fills a buffer with cryptographically secure random ASCII alphanumeric
 * characters and terminates it with a null byte.
 * @param buffer destination buffer
 * @param len size of the destination buffer, including the null terminator
 */
void randstring(char *buffer, size_t len);

#endif
