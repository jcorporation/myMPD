/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Thread-local xoshiro256** random number generator
 */

#include "src/lib/random_xoshiro256.h"

#include "src/lib/log.h"

#include <errno.h>
#include <limits.h>
#include <openssl/rand.h>
#include <stdint.h>
#include <stdbool.h>

// Private definitions

/** Thread-local xoshiro256** state. */
static _Thread_local uint64_t fast_random_state[4];

/** True when fast_random_state has been initialized for this thread. */
static _Thread_local bool fast_random_initialized;

static uint64_t fast_random_splitmix64(uint64_t *state);
static uint64_t fast_random_rotl(uint64_t value, unsigned shift);
static void fast_random_seed(void);

// Public functions

/**
 * Returns the next pseudo-random value from the thread-local PRNG.
 * @return 64-bit pseudo-random value
 */
uint64_t fast_random64(void) {
    if (fast_random_initialized == false) {
        fast_random_seed();
    }

    const uint64_t result = fast_random_rotl(fast_random_state[1] * 5, 7) * 9;
    const uint64_t temp = fast_random_state[1] << 17;

    fast_random_state[2] ^= fast_random_state[0];
    fast_random_state[3] ^= fast_random_state[1];
    fast_random_state[1] ^= fast_random_state[2];
    fast_random_state[0] ^= fast_random_state[3];
    fast_random_state[2] ^= temp;
    fast_random_state[3] = fast_random_rotl(fast_random_state[3], 45);

    return result;
}

// Private functions

/**
 * Advances and mixes the provided SplitMix64 state.
 * @param state pointer to the current SplitMix64 state
 * @return mixed 64-bit output
 */
static uint64_t fast_random_splitmix64(uint64_t *state) {
    uint64_t z = (*state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/**
 * Performs a left rotation of a 64-bit unsigned value.
 * @param value value to rotate
 * @param shift rotation distance from zero through sixty-three
 * @return rotated 64-bit value
 */
static uint64_t fast_random_rotl(uint64_t value, unsigned shift) {
    return (value << shift) | (value >> (64 - shift));
}

/**
 * Lazily initializes the thread-local xoshiro256** state.
 * Uses OpenSSL for initial entropy and guarantees that the resulting state is
 * not entirely zero.
 */
static void fast_random_seed(void) {
    uint64_t seed[4];
    if (RAND_bytes((unsigned char *)seed, sizeof(seed)) != 1) {
        seed[0] = UINT64_C(0x9E3779B97F4A7C15);
        seed[1] = UINT64_C(0x3C6EF372FE94F82A);
        seed[2] = UINT64_C(0xA075B461C67C5C4D);
        seed[3] = UINT64_C(0xE65F4AAB391D1F0A);
    }

    uint64_t splitmix_state = seed[0];
    for (unsigned i = 0; i < 4; i++) {
        fast_random_state[i] = fast_random_splitmix64(&splitmix_state) ^ seed[i];
    }

    bool all_zero = true;
    for (unsigned i = 0; i < 4; i++) {
        if (fast_random_state[i] != 0) {
            all_zero = false;
            break;
        }
    }
    if (all_zero) {
        fast_random_state[0] = UINT64_C(1);
    }
    fast_random_initialized = true;
}
