/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/random.h"

#include "src/lib/log.h"

#include <assert.h>
#include <limits.h>
#include <openssl/rand.h>

/**
 * Generates a positive random number in range (inclusive lower and upper bounds)
 * @param lower lower boundary
 * @param upper upper boundary
 * @return random number
 */
long randrange(long lower, long upper) {
    uint32_t buf;
    if (RAND_bytes((unsigned char *)&buf, sizeof(buf)) == 1) {
        return lower + buf / (UINT32_MAX / (upper - lower + 1) + 1);
    }

    MYMPD_LOG_ERROR(NULL, "Error generating random number in range %ld - %ld", lower, upper);
    assert(NULL);
    return 0;
}
