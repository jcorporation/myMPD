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
#include <openssl/rand.h>
#include <stdint.h>

#include "src/lib/random_xoshiro256.h"

/**
 * Generates an unsigned type random number in range (inclusive lower and exclusive upper bound)
 * using openSSL cryptographic random number generator.
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

    return lower + (unsigned)((uint64_t)buf % range);
}

/**
 * Generates an unsigned type random number in range (inclusive lower and exclusive upper bound)
 * using a thread-local non-cryptographic PRNG.
 * Sets errno to EINVAL if lower >= upper
 * @param lower lower boundary
 * @param upper upper boundary
 * @return random number
 */
unsigned randrange_fast(unsigned lower, unsigned upper) {
    if (lower >= upper) {
        errno = EINVAL;
        return 0;
    }

    const uint64_t range = (uint64_t)upper - lower;
    const uint64_t threshold = (UINT64_C(0) - range) % range;
    uint64_t sample;

    do {
        sample = fast_random64();
    } while (sample < threshold);

    return lower + (unsigned)(sample % range);
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
 * Returns a random ascii char using the thread-local non-cryptographic PRNG.
 * @return random char
 */
char randchar(void) {
    return dict[randrange_fast(0, dict_len)];
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
    unsigned char random_bytes[256];
    size_t written = 0;

    while (written < max) {
        size_t requested = max - written;
        if (requested > sizeof(random_bytes)) {
            requested = sizeof(random_bytes);
        }

        if (RAND_bytes(random_bytes, (int)requested) != 1) {
            MYMPD_LOG_ERROR(NULL, "Error generating random string");
            buffer[written] = '\0';
            buffer[max] = '\0';
            errno = EINVAL;
            return;
        }

        for (size_t i = 0; i < requested && written < max; i++) {
            if (random_bytes[i] < 248) {
                buffer[written++] = dict[random_bytes[i] % dict_len];
            }
        }
    }

    buffer[max] = '\0';
}
