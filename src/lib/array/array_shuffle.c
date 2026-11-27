/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Array shuffle
 */

#include "src/lib/array/array_shuffle.h"
#include "src/lib/array/array.h"
#include "src/lib/random.h"

#include <errno.h>

/**
 * Shuffles the sds array using the Fisher-Yates algorithm
 * @param array Pointer to the sds array
 * @return bool true on success, false on failure
 */
bool array_shuffle(struct t_array *array) {
    if (array == NULL) {
        return false;
    }
    if (array->length < 2) {
        // Nothing to shuffle
        return true;
    }
    // Fisher-Yates shuffle
    errno = 0;
    for (unsigned i = array->length - 1; i > 0; i--) {
        // Generate a random number between 0 and i (inclusive)
        unsigned j = randrange_fast(0, i + 1);
        if (errno != 0) {
            return false;
        }

        // Swap nodes
        struct t_array_node temp = array->nodes[i];
        array->nodes[i] = array->nodes[j];
        array->nodes[j] = temp;
    }
    return true;
}
