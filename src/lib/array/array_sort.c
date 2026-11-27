/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Array sort
 */

#include "src/lib/array/array_sort.h"

#include "src/lib/array/array.h"
#include "src/lib/mem.h"
#include "src/lib/utf8_wrapper.h"

#include <string.h>

// Private definitions

static bool sort_cb_value_i(struct t_array_node *first, struct t_array_node *second,
        enum array_sort_direction direction);
static bool sort_cb_value_p(struct t_array_node *first, struct t_array_node *second,
        enum array_sort_direction direction);
static bool sort_cb_key(struct t_array_node *first, struct t_array_node *second,
        enum array_sort_direction direction);
static bool sort_cb_value_i_key_value_p(struct t_array_node *first, struct t_array_node *second,
        enum array_sort_direction direction);
static bool sort_cb_key_value_p_value_i(struct t_array_node *first, struct t_array_node *second,
        enum array_sort_direction direction);

// Public functions

/**
 * Sorts the array with a stable bottom-up merge sort.
 * The callback returns true when the first node should follow the second.
 * @param array Pointer to the array
 * @param direction Sort direction
 * @param sort_cb Comparison callback
 * @return true on success, false for invalid arguments
 */
bool array_sort_by_callback(struct t_array *array, enum array_sort_direction direction, array_sort_callback sort_cb) {
    if (array == NULL ||
        (direction != ARRAY_SORT_ASC && direction != ARRAY_SORT_DESC) ||
        (array->length > 1 && sort_cb == NULL))
    {
        return false;
    }
    if (array->length < 2) {
        return true;
    }

    size_t length = array->length;
    struct t_array_node *buffer = malloc_assert_count(length, sizeof(*buffer));
    for (size_t width = 1; width < length;) {
        size_t left = 0;
        while (left < length) {
            size_t remaining = length - left;
            size_t left_count = remaining < width ? remaining : width;
            size_t middle = left + left_count;
            remaining = length - middle;
            size_t right_count = remaining < width ? remaining : width;
            size_t right = middle;
            size_t end = middle + right_count;
            size_t output = left;

            while (left < middle && right < end) {
                if (sort_cb(&array->nodes[left], &array->nodes[right], direction)) {
                    buffer[output++] = array->nodes[right++];
                }
                else {
                    buffer[output++] = array->nodes[left++];
                }
            }
            while (left < middle) {
                buffer[output++] = array->nodes[left++];
            }
            while (right < end) {
                buffer[output++] = array->nodes[right++];
            }
            left = end;
        }
        memcpy(array->nodes, buffer, length * sizeof(*buffer));
        if (width > length / 2) {
            break;
        }
        width *= 2;
    }
    free(buffer);
    return true;
}

/**
 * Sorts the array by integer value.
 * @param array Pointer to the array
 * @param direction Sort direction
 * @return true on success, false for invalid arguments
 */
bool array_sort_by_value_i(struct t_array *array, enum array_sort_direction direction) {
    return array_sort_by_callback(array, direction, sort_cb_value_i);
}

/**
 * Sorts the array by string value, using a UTF-8-aware case-insensitive comparison.
 * @param array Pointer to the array
 * @param direction Sort direction
 * @return true on success, false for invalid arguments
 */
bool array_sort_by_value_p(struct t_array *array, enum array_sort_direction direction) {
    return array_sort_by_callback(array, direction, sort_cb_value_p);
}

/**
 * Sorts the array by key, using a UTF-8-aware case-insensitive comparison.
 * @param array Pointer to the array
 * @param direction Sort direction
 * @return true on success, false for invalid arguments
 */
bool array_sort_by_key(struct t_array *array, enum array_sort_direction direction) {
    return array_sort_by_callback(array, direction, sort_cb_key);
}

/**
 * Sorts the array by value and key, using a UTF-8-aware case-insensitive comparison.
 * @param array Pointer to the array
 * @param direction Sort direction
 * @return true on success, false for invalid arguments
 */
bool array_sort_by_value_i_key_value_p(struct t_array *array, enum array_sort_direction direction) {
    return array_sort_by_callback(array, direction, sort_cb_value_i_key_value_p);
}

/**
 * Sorts the array by key and value, using a UTF-8-aware case-insensitive comparison.
 * @param array Pointer to the array
 * @param direction Sort direction
 * @return true on success, false for invalid arguments
 */
bool array_sort_by_key_value_p_value_i(struct t_array *array, enum array_sort_direction direction) {
    return array_sort_by_callback(array, direction, sort_cb_key_value_p_value_i);
}

// Private functions

/**
 * Comparison callback for sorting by integer value.
 * @param first Pointer to the first array node
 * @param second Pointer to the second array node
 * @param direction Sort direction
 * @return true if the first node should follow the second, false otherwise
 */
static bool sort_cb_value_i(struct t_array_node *first, struct t_array_node *second,
        enum array_sort_direction direction)
{
    return direction == ARRAY_SORT_ASC
        ? first->value_i > second->value_i
        : first->value_i < second->value_i;
}

/**
 * Comparison callback for sorting by string value.
 * @param first Pointer to the first array node
 * @param second Pointer to the second array node
 * @param direction Sort direction
 * @return true if the first node should follow the second, false otherwise
 */
static bool sort_cb_value_p(struct t_array_node *first, struct t_array_node *second,
        enum array_sort_direction direction)
{
    if (first->value_p == NULL || second->value_p == NULL) {
        if (first->value_p == second->value_p) {
            return false; // both NULL
        }
        return direction == ARRAY_SORT_ASC
            ? first->value_p == NULL
            : second->value_p == NULL;
    }
    int result = utf8_wrap_casecmp(first->value_p, sdslen(first->value_p),
        second->value_p, sdslen(second->value_p));
    return direction == ARRAY_SORT_ASC
        ? result > 0
        : result < 0;
}

/**
 * Comparison callback for sorting by key.
 * @param first Pointer to the first array node
 * @param second Pointer to the second array node
 * @param direction Sort direction
 * @return true if the first node should follow the second, false otherwise
 */
static bool sort_cb_key(struct t_array_node *first, struct t_array_node *second,
        enum array_sort_direction direction)
{
    if (first->key == NULL || second->key == NULL) {
        if (first->key == second->key) {
            return false; // both NULL
        }
        return direction == ARRAY_SORT_ASC
            ? first->key == NULL
            : second->key == NULL;
    }
    int result = utf8_wrap_casecmp(first->key, sdslen(first->key),
        second->key, sdslen(second->key));
    return direction == ARRAY_SORT_ASC
        ? result > 0
        : result < 0;
}

/**
 * Comparison callback for sorting by integer value, then by key and then by value.
 * @param first Pointer to the first array node
 * @param second Pointer to the second array node
 * @param direction Sort direction
 * @return true if the first node should follow the second, false otherwise
 */
static bool sort_cb_value_i_key_value_p(struct t_array_node *first, struct t_array_node *second,
        enum array_sort_direction direction)
{
    int64_t result = (first->value_i > second->value_i) - (first->value_i < second->value_i);
    if (result == 0 && first->key != NULL && second->key != NULL) {
        result = utf8_wrap_casecmp(first->key, sdslen(first->key), second->key, sdslen(second->key));
        return direction == ARRAY_SORT_ASC
            ? result > 0
            : result < 0;
    }
    if (result == 0 && first->value_p != NULL && second->value_p != NULL) {
        result = utf8_wrap_casecmp(first->value_p, sdslen(first->value_p), second->value_p, sdslen(second->value_p));
        return direction == ARRAY_SORT_ASC
            ? result > 0
            : result < 0;
    }
    // Reverse the result for descending order, because we want the newest first by default
    return direction == ARRAY_SORT_DESC
        ? result > 0
        : result < 0;
}

/**
 * Comparison callback for sorting by key, then by value and then by integer value.
 * @param first Pointer to the first array node
 * @param second Pointer to the second array node
 * @param direction Sort direction
 * @return true if the first node should follow the second, false otherwise
 */
static bool sort_cb_key_value_p_value_i(struct t_array_node *first, struct t_array_node *second,
        enum array_sort_direction direction)
{
    int64_t result = utf8_wrap_casecmp(first->key, sdslen(first->key), second->key, sdslen(second->key));
    if (result == 0 && first->value_p != NULL && second->value_p != NULL) {
        result = utf8_wrap_casecmp(first->value_p, sdslen(first->value_p), second->value_p, sdslen(second->value_p));
    }
    if (result == 0) {
        result = (first->value_i > second->value_i) - (first->value_i < second->value_i);
        // Reverse the result for descending order, because we want the newest first by default
        return direction == ARRAY_SORT_DESC
            ? result > 0
            : result < 0;
    }
    return direction == ARRAY_SORT_ASC
        ? result > 0
        : result < 0;
}
