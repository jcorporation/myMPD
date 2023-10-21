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
 * Generates a long type positive random number in range (inclusive lower and upper bounds)
 * @param lower lower boundary
 * @param upper upper boundary
 * @return random number
 */
#if ULONG_MAX == 0xffffffffffffffff
long randrange(long lower, long upper) {
    uint64_t buf;
    uint64_t u_lower = (uint64_t)lower;
    uint64_t u_upper = (uint64_t)upper;
    if (RAND_bytes((unsigned char *)&buf, sizeof(buf)) == 1) {
        return (long)(u_lower + buf / (UINT64_MAX / (u_upper - u_lower + 1) + 1));
    }

    MYMPD_LOG_ERROR(NULL, "Error generating random number in range");
    assert(NULL);
    return 0;
}
#else
long randrange(long lower, long upper) {
    uint32_t buf;
    uint32_t u_lower = (uint32_t)lower;
    uint32_t u_upper = (uint32_t)upper;
    if (RAND_bytes((unsigned char *)&buf, sizeof(buf)) == 1) {
        return (long)(u_lower + buf / (UINT32_MAX / (u_upper - u_lower + 1) + 1));
    }

    MYMPD_LOG_ERROR(NULL, "Error generating random number in range");
    assert(NULL);
    return 0;
}
#endif
