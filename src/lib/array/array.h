/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Dynamic array implementation with same semantic as the linked list implementation
 *        but with better performance for random access and sorting.
 */

#ifndef MYMPD_ARRAY_H
#define MYMPD_ARRAY_H

#include "dist/sds/sds.h"

#include <stdbool.h>

/**
 * Array node struct holding array key/value and additional data
 */
struct t_array_node {
    sds key;                   //!< key string
    sds value_p;               //!< string value
    int64_t value_i;           //!< int64 value
    void *user_data;           //!< custom data pointer
};

/**
 * Struct representing a dynamic array of sds strings
 */
struct t_array {
    unsigned length;              //!< Current number of elements in the array
    unsigned capacity;            //!< Maximum number of elements the array can hold before resizing
    struct t_array_node *nodes;   //!< Packed array of nodes
};

enum {
    ARRAY_START_CAPACITY = 16
};

/**
 * Definition of user data callback
 */
typedef void (*array_user_data_callback) (struct t_array_node *node);

struct t_array *array_new(void);
struct t_array *array_new_capacity(unsigned capacity);
void array_init(struct t_array *array, unsigned capacity);
void array_clear(struct t_array *array, array_user_data_callback free_cb);
void array_free(struct t_array *array, array_user_data_callback free_cb);
bool array_push(struct t_array *array, const char *key, const char *value_p, int64_t value_i, void *user_data);

struct t_array_node *array_get_node(const struct t_array *array, const char *key);
struct t_array_node *array_node_at(const struct t_array *array, unsigned idx);

#endif
