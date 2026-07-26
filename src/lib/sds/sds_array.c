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
#include "src/lib/utf8_wrapper.h"

// Private definitions
static int sort_compare_asc(const void* a, const void* b);
static int sort_compare_desc(const void* a, const void* b);

// Public functions

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
 * @param array Pointer to the sds array to clear
 */
void sds_array_clear(struct t_sds_array *array) {
    for (unsigned i = 0; i < array->length; i++) {
        sdsfree(array->items[i]);
    }
    array->length = 0;
}

/**
 * Frees the sds array and all its contents
 * @param array Pointer to the sds array to free
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
 * @return bool true on success, false on failure
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

/**
 * Sorts the sds array utf8 aware and case insensitive using qsort
 * @param array Pointer to the sds array
 * @param desc If true, sorts in descending order; otherwise, sorts in ascending order
 * @return bool true on success, false on failure
 */
bool sds_array_sort(struct t_sds_array *array, bool desc) {
    if (array->length <= 1) {
        // Nothing to sort
        return true;
    }
    if (desc == false) {
        qsort((void *)array->items, array->length, sizeof(sds), sort_compare_asc);
    }
    else {
        qsort((void *)array->items, array->length, sizeof(sds), sort_compare_desc);
    }
    return true;
}

// Internal functions

/**
 * Compares two sds strings for sorting in ascending order
 * @param a Pointer to the first sds string
 * @param b Pointer to the second sds string
 * @return int negative if a < b, zero if a == b, positive if a > b
 */
static int sort_compare_asc(const void *a, const void *b) {
   sds *s1 = (sds *)a;
   sds *s2 = (sds *)b;
   return utf8_wrap_casecmp(*s1, sdslen(*s1), *s2, sdslen(*s2));
}

/**
 * Compares two sds strings for sorting in descending order
 * @param a Pointer to the first sds string
 * @param b Pointer to the second sds string
 * @return int positive if a < b, zero if a == b, negative if a > b
 */
static int sort_compare_desc(const void *a, const void *b) {
   sds *s1 = (sds *)a;
   sds *s2 = (sds *)b;
   return utf8_wrap_casecmp(*s2, sdslen(*s2), *s1, sdslen(*s1));
}
