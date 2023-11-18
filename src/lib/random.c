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
