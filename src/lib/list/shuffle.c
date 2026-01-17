/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Linked list implementation
 */

#include "compile_time.h"
#include "src/lib/list/shuffle.h"

#include "src/lib/list/list.h"
#include "src/lib/random.h"

// Private definitions

static bool swap_nodes(struct t_list_node *n1, struct t_list_node *n2);

// Public functions

/**
 * Shuffles the list.
 * @param l list
 * @return true on success, else false
 */
bool list_shuffle(struct t_list *l) {
    if (l->length < 2) {
        return false;
    }
    struct t_list_node *current = l->head;
    while (current != NULL) {
        unsigned pos = randrange(0, l->length);
        swap_nodes(current, list_node_at(l, pos));
        current = current->next;
    }
    return true;
}


// Private functions

/**
 * Swaps two list nodes values.
 * @param n1 first node
 * @param n2 second node
 * @return true on success, else false
 */
static bool swap_nodes(struct t_list_node *n1, struct t_list_node *n2) {
    if (n1 == n2 ||
        n1 == NULL ||
        n2 == NULL)
    {
        return false;
    }

    sds key = n2->key;
    int64_t value_i = n2->value_i;
    sds value_p = n2->value_p;
    void *user_data = n2->user_data;

    n2->key = n1->key;
    n2->value_i = n1->value_i;
    n2->value_p = n1->value_p;
    n2->user_data = n1->user_data;

    n1->key = key;
    n1->value_i = value_i;
    n1->value_p = value_p;
    n1->user_data = user_data;

    return true;
}
