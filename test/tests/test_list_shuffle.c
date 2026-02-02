/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/list/list.h"
#include "src/lib/list/shuffle.h"


UTEST(list_shuffle, test_list_large_shuffle) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true, 25000);

    unsigned eq = 0;
    unsigned pos = 0;
    struct t_list_node *current = test_list->head;
    while (current != NULL) {
        if (current->value_i == pos) {
            // Node is on original position
            eq++;
        }
        current = current->next;
        pos++;
    }
    ASSERT_LT(eq, 10U); // Max 10 nodes with the same position
    ASSERT_EQ(check_list_integrity(test_list, expected_len), true);

    list_free(test_list);
}
