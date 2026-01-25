/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Fuzzy search implementation
 */

#include "compile_time.h"
#include "src/lib/search/search_fuzzy.h"

#include <stdlib.h>
#include <string.h>

/**
 * Private definitions
 */

 static size_t levenshtein(const char *a, size_t a_len, const char *b, size_t b_len,
        size_t *cache, size_t max_distance);

/**
 * Public functions
 */

/**
 * Fuzzy substring matching using the levenshtein distance
 * @param haystack Haystack
 * @param haystack_len Haystack length
 * @param needle Needle
 * @param needle_len Needle length
 * @return true on match, else false
 */
bool mympd_search_fuzzy_match(const char *haystack, size_t haystack_len,
        const char *needle, size_t needle_len)
{
    if (needle_len <= 1) {
        return true;
    }
    if (needle_len > haystack_len) {
        return false;
    }
    if (strstr(haystack, needle) != NULL) {
        return true;
    }
    const size_t max_distance = needle_len < 10
        ? 1
        : (needle_len / 10) + 1;
    size_t *cache = calloc(needle_len + 1, sizeof(size_t));
    const char *p = haystack;
    while (*p != '\0' &&
           haystack_len >= needle_len)
    {
        if (levenshtein(p, needle_len, needle, needle_len, cache, max_distance) <= max_distance) {
            free(cache);
            return true;
        }
        p++;
        haystack_len--;
    }
    free(cache);
    return false;
}

/**
 * Private functions
 */

/**
 * Return the minimum of 3 integers
 */
#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

/**
 * Calculate the levenshtein distance
 * https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C
 * @param a String 1
 * @param a_len Length of a
 * @param b String 2
 * @param b_len Length of b
 * @param cache Matrix cache
 * @param max_distance Return as soon as the calculated distance is smaller than this value
 * @return Calculated distance
 */
static size_t levenshtein(const char *a, size_t a_len, const char *b, size_t b_len,
        size_t *cache, size_t max_distance)
{
    size_t a_idx;
    size_t b_idx;
    size_t lastdiag;
    size_t olddiag;
    for (a_idx = 1; a_idx <= a_len; a_idx++) {
        cache[a_idx] = a_idx;
    }
    for (b_idx = 1; b_idx <= b_len; b_idx++) {
        cache[0] = b_idx;
        for (a_idx = 1, lastdiag = b_idx - 1; a_idx <= a_len; a_idx++) {
            olddiag = cache[a_idx];
            cache[a_idx] = MIN3(cache[a_idx] + 1, cache[a_idx - 1] + 1, lastdiag + (a[a_idx - 1] == b[b_idx - 1] ? 0 : 1));
            lastdiag = olddiag;
            // Check if distance is too high to become lower equal the max distance value in the remaining iterations
            if (cache[a_idx] > a_len - a_idx + max_distance) {
                break;
            }
        }
        // We do not require the minimum distance, a distance lower equal the max distance is good enough
        if (cache[a_len] <= max_distance) {
            //MYMPD_LOG_DEBUG(NULL, "levenshtein return early %lu/%lu: %.*s - %.*s", x, b_len, (int)a_len, a, (int)b_len, b);
            return cache[a_len];
        }
    }
    return cache[a_len];
}
