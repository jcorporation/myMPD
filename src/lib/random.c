/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Random number generator functions based on OpenSSL
 */

#include "src/lib/random.h"

#include "src/lib/log.h"

#include <errno.h>
#include <limits.h>
#include <openssl/rand.h>

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
    unsigned buf;
    if (RAND_bytes((unsigned char *)&buf, sizeof(buf)) == 1) {
        errno = 0;
        return lower + (unsigned) ((upper - lower) * (buf / (UINT_MAX + 1.0)));
    }

    MYMPD_LOG_ERROR(NULL, "Error generating random number in range");
    errno = EINVAL;
    return 0;
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
