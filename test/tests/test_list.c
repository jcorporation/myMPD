/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../dist/src/utest/utest.h"
#include "../../src/lib/list.h"

UTEST(list, manipulate) {
    struct t_list test_list;
    list_init(&test_list);
    struct t_list_node *current;

    list_push(&test_list, "key1", 1, "value1", NULL);
    list_push(&test_list, "key2", 2, "value2", NULL);
    list_push(&test_list, "key3", 3, "value3", NULL);
    list_push(&test_list, "key4", 4, "value4", NULL);
    list_push(&test_list, "key5", 5, "value5", NULL);
    list_insert(&test_list, "key0", 0, "value0", NULL);

    ASSERT_STREQ("key0", test_list.head->key);
    ASSERT_STREQ("key5", test_list.tail->key);

    //swap nodes
    list_swap_item_pos(&test_list, 3, 1);
    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("key3", current->key);
    current = list_node_at(&test_list, 3);
    ASSERT_STREQ("key1", current->key);
    
    //sort nodes
    list_sort_by_key(&test_list, LIST_SORT_ASC);
    ASSERT_STREQ("key0", test_list.head->key);
    ASSERT_STREQ("key5", test_list.tail->key);

    //move node
    list_move_item_pos(&test_list, 4, 2);
    current = list_node_at(&test_list, 2);
    ASSERT_STREQ("key4", current->key);
    current = list_node_at(&test_list, 4);
    ASSERT_STREQ("key3", current->key);

/*
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
*/
    list_clear(&test_list);
}

UTEST(list, insert_sorted_by_key) {
    struct t_list test_list;
    list_init(&test_list);
    struct t_list_node *current;
    
    list_insert_sorted_by_key(&test_list, "ddd", 1, "value1", NULL, LIST_SORT_ASC);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("ddd", current->key);

    list_insert_sorted_by_key(&test_list, "bbb", 1, "value1", NULL, LIST_SORT_ASC);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("bbb", current->key);
    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("ddd", current->key);

    list_insert_sorted_by_key(&test_list, "ccc", 1, "value1", NULL, LIST_SORT_ASC);
    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("ccc", current->key);

    list_insert_sorted_by_key(&test_list, "aaa", 1, "value1", NULL, LIST_SORT_ASC);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("aaa", current->key);

    list_insert_sorted_by_key(&test_list, "xxx", 1, "value1", NULL, LIST_SORT_ASC);
    current = list_node_at(&test_list, 4);
    ASSERT_STREQ("xxx", current->key);

    list_insert_sorted_by_key(&test_list, "ggg", 1, "value1", NULL, LIST_SORT_ASC);
    current = list_node_at(&test_list, 4);
    ASSERT_STREQ("ggg", current->key);

    ASSERT_STREQ("aaa", test_list.head->key);
    ASSERT_STREQ("xxx", test_list.tail->key);

    list_clear(&test_list);
}

UTEST(list, insert_sorted_by_value_i) {
    struct t_list test_list;
    list_init(&test_list);
    struct t_list_node *current;
    
    list_insert_sorted_by_value_i(&test_list, "ddd", 4, "value1", NULL, LIST_SORT_DESC);
    current = list_node_at(&test_list, 0);
    ASSERT_EQ(4, current->value_i);

    list_insert_sorted_by_value_i(&test_list, "bbb", 2, "value1", NULL, LIST_SORT_DESC);
    current = list_node_at(&test_list, 1);
    ASSERT_EQ(2, current->value_i);
    current = list_node_at(&test_list, 0);
    ASSERT_EQ(4, current->value_i);

    list_insert_sorted_by_value_i(&test_list, "ccc", 3, "value1", NULL, LIST_SORT_DESC);
    current = list_node_at(&test_list, 1);
    ASSERT_EQ(3, current->value_i);

    list_insert_sorted_by_value_i(&test_list, "aaa", 1, "value1", NULL, LIST_SORT_DESC);
    current = list_node_at(&test_list, 3);
    ASSERT_EQ(1, current->value_i);

    list_insert_sorted_by_value_i(&test_list, "xxx", 24, "value1", NULL, LIST_SORT_DESC);
    current = list_node_at(&test_list, 0);
    ASSERT_EQ(24, current->value_i);
    
    ASSERT_EQ(24, test_list.head->value_i);
    ASSERT_EQ(1, test_list.tail->value_i);

    list_clear(&test_list);
}
