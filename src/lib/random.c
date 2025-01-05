/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Random number generator functions based on OpenSSL
 */

#include "compile_time.h"
#include "src/lib/random.h"

#include "src/lib/log.h"

#include <assert.h>
#include <limits.h>
#include <openssl/rand.h>

/**
 * Generates an unsigned type random number in range (inclusive lower and exclusive upper bound)
 * @param lower lower boundary
 * @param upper upper boundary
 * @return random number
 */
unsigned randrange(unsigned lower, unsigned upper) {
    unsigned buf;
    if (RAND_bytes((unsigned char *)&buf, sizeof(buf)) == 1) {
        return lower + (unsigned) ((upper - lower) * (buf / (UINT_MAX + 1.0)));
    }

    MYMPD_LOG_ERROR(NULL, "Error generating random number in range");
    assert(NULL);
    return 0;
}

/**
 * Characterset to generate random strings from
 */
static const char *dict = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
/**
 * Charaterset length
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
 * @param buffer buffer to fill
 * @param len length of buffer
 */
void randstring(char *buffer, size_t len) {
    size_t max = len - 1;
    for (size_t i = 0; i < max; i++) {
        buffer[i] = randchar();
    }
    buffer[max] = '\0';
}
