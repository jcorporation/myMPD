/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Array sort
 */

#ifndef MYMPD_ARRAY_SORT_H
#define MYMPD_ARRAY_SORT_H

#include <stdbool.h>

struct t_array;
struct t_array_node;

/**
 * Sort direction
 */
enum array_sort_direction {
    ARRAY_SORT_ASC = 0,
    ARRAY_SORT_DESC = 1,
};

/**
 * Definition of sort callback
 */
typedef bool (*array_sort_callback) (struct t_array_node *current, struct t_array_node *next,
        enum array_sort_direction direction);

bool array_sort_by_callback(struct t_array *l, enum array_sort_direction direction, array_sort_callback sort_cb);
bool array_sort_by_value_i(struct t_array *l, enum array_sort_direction direction);
bool array_sort_by_value_p(struct t_array *l, enum array_sort_direction direction);
bool array_sort_by_key(struct t_array *l, enum array_sort_direction direction);
bool array_sort_by_value_i_key_value_p(struct t_array *l, enum array_sort_direction direction);
bool array_sort_by_key_value_p_value_i(struct t_array *l, enum array_sort_direction direction);

#endif
