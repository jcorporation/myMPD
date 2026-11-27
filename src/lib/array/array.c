/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Dynamic array implementation with same semantic as the linked list implementation
 *        but with better performance for random access and sorting.
 */

#include "src/lib/array/array.h"

#include "src/lib/mem.h"
#include "src/lib/sds/sds_extras.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

// Public functions

/**
 * Creates and initializes a new array
 * @return struct t_array*
 */
struct t_array *array_new(void) {
    return array_new_capacity(0);
}

/**
 * Creates and initializes a new array with a specified initial capacity
 * @param capacity Initial capacity of the array
 * @return struct t_array*
 */
struct t_array *array_new_capacity(unsigned capacity) {
    struct t_array *array = malloc_assert(sizeof(struct t_array));
    array->length = 0;
    array->capacity = capacity > 0 ? capacity : ARRAY_START_CAPACITY;
    array->nodes = malloc_assert_count(array->capacity, sizeof(*array->nodes));
    return array;
}

/**
 * Clears the array, freeing its nodes and values
 * @param array Pointer to the array to clear
 * @param free_cb Callback function to free user data, if NULL, user data will not be freed
 */
void array_clear(struct t_array *array, array_user_data_callback free_cb) {
    for (unsigned i = 0; i < array->length; i++) {
        struct t_array_node *node = &array->nodes[i];
        if (node->user_data != NULL &&
            free_cb != NULL)
        {
            free_cb(node);
            node->user_data = NULL;
        }
        FREE_SDS(node->key);
        FREE_SDS(node->value_p);
        node->value_i = 0;
    }
    array->length = 0;
}

/**
 * Frees the array and all its contents
 * @param array Pointer to the array to free
 * @param free_cb Callback function to free user data
 */
void array_free(struct t_array *array, array_user_data_callback free_cb) {
    if (array == NULL) {
        return;
    }
    array_clear(array, free_cb);
    free(array->nodes);
    free(array);
}

/**
 * Pushes the sds string to the array
 * @param array Pointer to the sds array
 * @param key Key string for the array node, can be NULL
 * @param value_p String value for the array node, can be NULL
 * @param value_i Integer value for the array node
 * @param user_data Custom data pointer for the array node, can be NULL
 * @return bool true on success, false on failure
 */
bool array_push(struct t_array *array, const char *key, const char *value_p, int64_t value_i, void *user_data) {
    assert(array);
    assert(array->length <= array->capacity);
    if (array->length == array->capacity) {
        if (array->capacity > UINT_MAX - (array->capacity / 2) - 8) {
            return false;
        }
        unsigned new_capacity = array->capacity + (array->capacity / 2) + 8;
        array->nodes = realloc_assert_count(array->nodes, new_capacity, sizeof(*array->nodes));
        array->capacity = new_capacity;
    }
    struct t_array_node *node = &array->nodes[array->length++];
    node->key = key == NULL ? NULL : sdsnew(key);
    node->value_p = value_p == NULL ? NULL : sdsnew(value_p);
    node->value_i = value_i;
    node->user_data = user_data;
    return true;
}

/**
 * Returns the array node with the specified key
 * @param array Pointer to the array
 * @param key Key of the node to return
 * @return Pointer to the array node with the specified key, or NULL if not found
 */
struct t_array_node *array_get_node(const struct t_array *array, const char *key) {
    assert(array);
    assert(key);
    for (unsigned i = 0; i < array->length; i++) {
        if (array->nodes[i].key != NULL &&
            strcmp(array->nodes[i].key, key) == 0) {
            return &array->nodes[i];
        }
    }
    return NULL;
}

/**
 * Returns the array node at the specified index
 * @param array Pointer to the array
 * @param idx Index of the node to return
 * @return Pointer to the array node at the specified index, or NULL if the index is out of bounds
 */
struct t_array_node *array_node_at(const struct t_array *array, unsigned idx) {
    assert(array);
    if (idx >= array->length) {
        return NULL;
    }
    return &array->nodes[idx];
}
