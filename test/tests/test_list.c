/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../src/lib/list.h"
#include "../../src/lib/sds_extras.h"

#include "test_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_list(void) {
    struct t_list *test_list = list_new();
    list_init(test_list);
    list_push(test_list, "key1", 1, "value1", NULL);
    list_push(test_list, "key2", 2, "value2", NULL);
    list_push(test_list, "key3", 3, "value3", NULL);
    list_push(test_list, "key4", 4, "value4", NULL);
    list_push(test_list, "key5", 5, "value5", NULL);
    list_insert(test_list, "key0", 0, "value0", NULL);
    //test1
    list_swap_item_pos(test_list, 3, 1);
    list_move_item_pos(test_list, 4, 2);
    //remove middle item
    list_shift(test_list,3);
    //remove last item
    list_shift(test_list, 5);
    //remove first item
    list_shift(test_list, 0);
    list_push(test_list, "key6", 6, "value6", NULL);
    list_insert(test_list, "key7", 7, "value7", NULL);
    int i = 0;
    struct t_list_node *current = test_list->head;
    while (current != NULL) {
        printf("%d: %s\n", i, current->key);
        current = current->next;
        i++;
    }
    list_clear(test_list);
    //sorted inserts by key
    list_insert_sorted_by_key(test_list, "ddd", 1, "value1", NULL, true);
    list_insert_sorted_by_key(test_list, "bbb", 1, "value1", NULL, true);
    list_insert_sorted_by_key(test_list, "ccc", 1, "value1", NULL, true);
    list_insert_sorted_by_key(test_list, "aaa", 1, "value1", NULL, true);
    list_insert_sorted_by_key(test_list, "xxx", 1, "value1", NULL, true);
    list_insert_sorted_by_key(test_list, "ggg", 1, "value1", NULL, true);
    list_insert_sorted_by_key(test_list, "yyy", 1, "value1", NULL, true);
    list_insert_sorted_by_key(test_list, "zzz", 1, "value1", NULL, true);
    list_push(test_list, "last", 1, "value1", NULL); //check if tail is correct
    i = 0;
    current = test_list->head;
    while (current != NULL) {
        printf("%d: %s\n", i, current->key);
        current = current->next;
        i++;
    }
    printf("Tail is: %s\n", test_list->tail->key);
    list_clear(test_list);
    //sorted inserts by value_i
    list_insert_sorted_by_value_i(test_list, "ddd", 4, "value1", NULL, true);
    list_insert_sorted_by_value_i(test_list, "bbb", 2, "value1", NULL, true);
    list_insert_sorted_by_value_i(test_list, "ccc", 3, "value1", NULL, true);
    list_insert_sorted_by_value_i(test_list, "aaa", 1, "value1", NULL, true);
    list_insert_sorted_by_value_i(test_list, "xxx", 24, "value1", NULL, true);
    list_insert_sorted_by_value_i(test_list, "ggg", 7, "value1", NULL, true);
    list_insert_sorted_by_value_i(test_list, "yyy", 25, "value1", NULL, true);
    list_insert_sorted_by_value_i(test_list, "zzz", 26, "value1", NULL, true);
    list_push(test_list, "last", 0, "value1", NULL); //check if tail is correct
    i = 0;
    current = test_list->head;
    while (current != NULL) {
        printf("%d: %s - %ld\n", i, current->key, current->value_i);
        current = current->next;
        i++;
    }
    printf("Tail is: %s\n", test_list->tail->key);

    list_replace(test_list, 0, "test", 0, NULL, NULL);

    list_clear(test_list);
    free(test_list);
}
