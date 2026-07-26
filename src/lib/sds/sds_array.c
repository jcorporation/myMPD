/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Array functions for sds strings
 */

#include "src/lib/sds/sds_array.h"
#include "src/lib/mem.h"

#include "dist/sds/sds.h"
#include "src/lib/random.h"

/**
 * Creates and initializes a new sds array
 * @return struct t_sds_array* 
 */
struct t_sds_array *sds_array_new(void) {
    struct t_sds_array *array = malloc_assert(sizeof(struct t_sds_array));
    sds_array_init(array);
    return array;
}

/**
 * Initializes an existing sds array
 * @param array Pointer to the sds array to initialize
 */
void sds_array_init(struct t_sds_array *array) {
    array->length = 0;
    array->capacity = SDS_ARRAY_START_CAPACITY;
    array->items = (sds *)malloc_assert(array->capacity * sizeof(sds));
}

/**
 * Clears the sds array, freeing all sds strings
 */
void sds_array_clear(struct t_sds_array *array) {
    for (unsigned i = 0; i < array->length; i++) {
        sdsfree(array->items[i]);
    }
    array->length = 0;
}

/**
 * Frees the sds array and all its contents
 */
void sds_array_free(struct t_sds_array *array) {
    sds_array_clear(array);
    free((sds)array->items);
    free(array);
}

/**
 * Pushes the sds string to the array
 * @param array Pointer to the sds array
 * @param s Pointer of sds string to push (this function does not copy it)
 * @return bool true on success, false on failure
 */
bool sds_array_push(struct t_sds_array *array, sds s) {
    if (array->length == array->capacity) {
        // Array is too small, resize
        array->capacity = 8 + ((array->capacity / 2) * 3);
        array->items = (sds *)realloc_assert((void *)array->items, array->capacity * sizeof(sds));
    }
    array->items[array->length++] = s;
    return true;
}

/**
 * Shuffles the sds array using the Fisher-Yates algorithm
 * @param array Pointer to the sds array
 */
bool sds_array_shuffle(struct t_sds_array *array) {
    if (array->length <= 2) {
        // Nothing to shuffle
        return true;
    }
    // Fisher-Yates shuffle
    for (unsigned i = array->length - 1; i > 0; i--) {
        // Generate a random number between 0 and i (inclusive)
        unsigned j = randrange(0, i + 1);

        // Swap nodes
        sds temp = array->items[i];
        array->items[i] = array->items[j];
        array->items[j] = temp;
    }
    return true;
}
