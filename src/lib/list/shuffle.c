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
#include "src/lib/mem.h"
#include "src/lib/random.h"

/**
 * Shuffles the list.
 * @param l list
 * @return true on success, else false
 */
bool list_shuffle(struct t_list *l) {
    if (l->length < 2) {
        return true;
    }
    // Convert linked list to array for faster shuffling
    struct t_list_node **node_array = (struct t_list_node **)malloc_assert(l->length * sizeof(struct t_list_node *));
    struct t_list_node *current = l->head;
    for (unsigned i = 0; i < l->length; i++) {
        node_array[i] = current;
        current = current->next;
    }

    // Fisher-Yates shuffle
    for (unsigned i = l->length - 1; i > 0; i--) {
        // Generate a random number between 0 and i (inclusive)
        unsigned j = randrange(0, i + 1);

        // Swap nodes
        struct t_list_node *temp = node_array[i];
        node_array[i] = node_array[j];
        node_array[j] = temp;
    }

    // Reconstruct the linked list
    for (unsigned i = 0; i < l->length - 1; i++) {
        node_array[i]->next = node_array[i + 1];
    }

    // Update head and tail
    l->head = node_array[0];
    l->tail = node_array[l->length - 1];
    l->tail->next = NULL;

    // Free temporary array
    free(node_array);

    return true;
}
