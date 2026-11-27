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

#include <assert.h>
#include <limits.h>
#include <string.h>

// Public functions

/**
 * Creates and initializes a new array
 * @return struct t_array*
 */
struct t_array *array_new(void) {
    struct t_array *array = malloc_assert(sizeof(struct t_array));
    array_init(array);
    return array;
}

/**
 * Initializes an existing array
 * @param array Pointer to the array to initialize
 */
void array_init(struct t_array *array) {
    array->length = 0;
    array->capacity = ARRAY_START_CAPACITY;
    array->nodes = malloc_assert_count(array->capacity, sizeof(*array->nodes));
}

/**
 * Clears the array, freeing its nodes and values
 * @param array Pointer to the array to clear
 * @param free_cb Callback function to free user data
 */
void array_clear(struct t_array *array, array_user_data_callback free_cb) {
    for (unsigned i = 0; i < array->length; i++) {
        array_node_free(array->nodes[i], free_cb);
        array->nodes[i] = NULL;
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
 * @param key Key string for the array node
 * @param value_p String value for the array node
 * @param value_i Integer value for the array node
 * @param user_data Custom data pointer for the array node
 * @return bool true on success, false on failure
 */
bool array_push(struct t_array *array, sds key, sds value_p, int64_t value_i, void *user_data) {
    assert(array);
    assert(array->length <= array->capacity);
    if (array->length == array->capacity) {
        if (array->capacity > UINT_MAX - array->capacity / 2 - 8) {
            return false;
        }
        size_t new_capacity = (size_t)(array->capacity + array->capacity / 2 + 8);
        array->nodes = realloc_assert_count(array->nodes, new_capacity, sizeof(*array->nodes));
        array->capacity = (unsigned)new_capacity;
    }
    array->nodes[array->length++] = array_node_new(key, value_p, value_i, user_data);
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
        if (array->nodes[i] != NULL &&
            array->nodes[i]->key != NULL &&
            strcmp(array->nodes[i]->key, key) == 0) {
            return array->nodes[i];
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
    return array->nodes[idx];
}

/**
 * Creates a new array node with the given key, value, and user data
 * @param key Key string for the array node
 * @param value_p String value for the array node
 * @param value_i Integer value for the array node
 * @param user_data Custom data pointer for the array node
 * @return Pointer to the newly created array node
 */
struct t_array_node *array_node_new(sds key, sds value_p, int64_t value_i, void *user_data) {
    struct t_array_node *node = malloc_assert(sizeof(struct t_array_node));
    node->key = key;
    node->value_p = value_p;
    node->value_i = value_i;
    node->user_data = user_data;
    return node;
}

/**
 * Frees an array node, including its key and value strings, and calls the user data callback if provided
 * @param node Pointer to the array node to free
 * @param free_cb Callback function to free user data
 */
void array_node_free(struct t_array_node *node, array_user_data_callback free_cb) {
    if (node == NULL) {
        return;
    }
    sdsfree(node->key);
    sdsfree(node->value_p);
    if (free_cb != NULL &&
        node->user_data != NULL) {
        free_cb(node);
    }
    free(node);
}
