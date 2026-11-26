/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Cryptographic and fast random number generator functions
 */

#include "src/lib/random.h"

#include "src/lib/log.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <openssl/rand.h>

#include "src/lib/random_xoshiro256.h"

/**
 * Generates an unsigned type random number in range (inclusive lower and exclusive upper bound)
 * Sets errno to EINVAL if lower >= upper
 * @param lower lower boundary
 * @param upper upper boundary
 * @return random number
 */
unsigned randrange(unsigned lower, unsigned upper) {
    if (lower >= upper) {
        MYMPD_LOG_ERROR(NULL, "Invalid random range: lower=%u upper=%u", lower, upper);
        errno = EINVAL;
        return 0;
    }
    const uint64_t space = (uint64_t)UINT_MAX + 1;
    const uint64_t range = (uint64_t)upper - lower;
    const uint64_t limit = space - (space % range);

    uint32_t buf;
    do {
        if (RAND_bytes((unsigned char *)&buf, sizeof(buf)) != 1) {
            MYMPD_LOG_ERROR(NULL, "Error generating random number in range");
            errno = EINVAL;
            return 0;
        }
    } while ((uint64_t)buf >= limit);

    errno = 0;
    return lower + (unsigned)((uint64_t)buf % range);
}

/**
 * Generates a uniformly distributed unsigned random number in the
 * [lower, upper) range using a thread-local non-cryptographic PRNG.
 */
unsigned randrange_fast(unsigned lower, unsigned upper) {
    if (lower >= upper) {
        errno = EINVAL;
        return 0;
    }

    const uint64_t range = (uint64_t)upper - lower;
    const uint64_t threshold = (UINT64_MAX - range + 1) % range;
    uint64_t sample;
    uint64_t product;

    do {
        sample = fast_random64() >> 32;
        product = sample * range;
    } while (product < threshold);

    errno = 0;
    return lower + (unsigned)(product >> 32);
}

/**
 * Characterset to generate random strings from
 */
static const char *dict = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
/**
 * Characterset length
 */
static unsigned dict_len = 62;

/**
 * Returns a random ascii char
 * @return random char
 */
char randchar(void) {
    return dict[randrange(0, dict_len)];
}

/**
 * Fills the buffer with random ascii chars and NULL-terminates it.
 * Sets errno to EINVAL if buffer is NULL or len is 0.
 * @param buffer buffer to fill
 * @param len length of buffer including NULL-term
 */
void randstring(char *buffer, size_t len) {
    if (buffer == NULL || len == 0) {
        MYMPD_LOG_ERROR(NULL, "Invalid buffer or length for random string generation");
        errno = EINVAL;
        return;
    }
    size_t max = len - 1;
    for (size_t i = 0; i < max; i++) {
        buffer[i] = randchar();
        if (errno != 0) {
            buffer[i] = '\0';
            buffer[max] = '\0';
            return;
        }
    }
    buffer[max] = '\0';
    errno = 0;
}
