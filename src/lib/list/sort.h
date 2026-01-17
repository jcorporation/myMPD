/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Linked list merge sort implementation
 */

#ifndef MYMPD_LIST_SORT_H
#define MYMPD_LIST_SORT_H

#include <stdbool.h>

struct t_list_node;
struct t_list;

/**
 * Sort direction
 */
enum list_sort_direction {
    LIST_SORT_ASC = 0,
    LIST_SORT_DESC = 1
};

/**
 * Definition of sort callback
 */
typedef bool (*list_sort_callback) (struct t_list_node *current, struct t_list_node *next, enum list_sort_direction direction);

bool list_sort_by_callback(struct t_list *l, enum list_sort_direction direction, list_sort_callback sort_cb);
bool list_sort_by_value_i(struct t_list *l, enum list_sort_direction direction);
bool list_sort_by_value_p(struct t_list *l, enum list_sort_direction direction);
bool list_sort_by_key(struct t_list *l, enum list_sort_direction direction);

#endif
