/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/utest/utest.h"
#include "../../src/lib/list.h"

static void populate_list(struct t_list *l) {
    list_init(l);
    list_push(l, "key1", 1, "value1", NULL);
    list_push(l, "key2", 2, "value2", NULL);
    list_push(l, "key3", 3, "value3", NULL);
    list_push(l, "key4", 4, "value4", NULL);
    list_push(l, "key5", 5, "value5", NULL);
    list_insert(l, "key0", 0, "value0", NULL);
}

UTEST(list, test_list_push) {
    struct t_list test_list;
    populate_list(&test_list);

    struct t_list_node *current;

    list_push(&test_list, "last", 6, "value6", NULL);
    ASSERT_STREQ("last", test_list.tail->key);
    ASSERT_EQ(7, test_list.length);
    current = list_node_at(&test_list, test_list.length - 1);
    ASSERT_STREQ(current->key, test_list.tail->key);

    list_clear(&test_list);
}

UTEST(list, test_list_insert) {
    struct t_list test_list;
    populate_list(&test_list);

    struct t_list_node *current;

    list_insert(&test_list, "first", -1, "value-1", NULL);
    ASSERT_STREQ("first", test_list.head->key);
    ASSERT_EQ(7, test_list.length);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ(current->key, test_list.head->key);

    list_clear(&test_list);
}

UTEST(list, test_list_shift) {
    struct t_list test_list;
    populate_list(&test_list);

    struct t_list_node *current;
    //remove middle item
    list_shift(&test_list, 3);
    current = list_node_at(&test_list, 4);
    ASSERT_STREQ("key5", current->key);
    ASSERT_EQ(5, test_list.length);

    //remove last item
    list_shift(&test_list, test_list.length - 1);
    ASSERT_STREQ("key4", test_list.tail->key);
    ASSERT_EQ(4, test_list.length);

    //check tail
    current = list_node_at(&test_list, test_list.length - 1);
    ASSERT_STREQ(current->key, test_list.tail->key);

    //remove first item
    list_shift(&test_list, 0);
    ASSERT_STREQ("key1", test_list.head->key);
    ASSERT_EQ(3, test_list.length);

    current = list_node_at(&test_list, 0);
    ASSERT_STREQ(current->key, test_list.head->key);

    list_clear(&test_list);
}

UTEST(list, test_list_replace) {
    struct t_list test_list;
    populate_list(&test_list);

    struct t_list_node *current;
    //replace middle item
    list_replace(&test_list, 4, "replace0", 0, NULL, NULL);
    current = list_node_at(&test_list, 4);
    ASSERT_STREQ("replace0", current->key);
    ASSERT_EQ(6, test_list.length);

    //replace last item
    list_replace(&test_list, test_list.length - 1, "replace1", 0, NULL, NULL);
    ASSERT_STREQ("replace1", test_list.tail->key);
    current = list_node_at(&test_list, 5);
    ASSERT_STREQ(current->key, test_list.tail->key);
    ASSERT_EQ(6, test_list.length);

    //replace first item
    list_replace(&test_list, 0, "replace2", 0, NULL, NULL);
    ASSERT_STREQ("replace2", test_list.head->key);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ(current->key, test_list.head->key);
    ASSERT_EQ(6, test_list.length);

    list_clear(&test_list);
}

UTEST(list, test_list_insert_sorted_by_key) {
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

UTEST(list, test_list_insert_sorted_by_value_i) {
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

UTEST(list, test_list_shuffle) {
    struct t_list test_list;
    list_init(&test_list);
    struct t_list_node *current;

    list_shuffle(&test_list);
    bool shuffled = false;
    for (unsigned i = 0; i < test_list.length; i++) {
        current = list_node_at(&test_list, i);
        if (current->value_i != (int)i) {
            shuffled = true;
        }
    }
    ASSERT_FALSE(shuffled);
    list_clear(&test_list);
}

UTEST(list, list_sort_by_value_i) {
    struct t_list test_list;
    populate_list(&test_list);

    list_sort_by_value_i(&test_list, LIST_SORT_DESC);
    ASSERT_EQ(0, test_list.tail->value_i);
    ASSERT_EQ(5, test_list.head->value_i);

    list_sort_by_key(&test_list, LIST_SORT_ASC);
    ASSERT_EQ(0, test_list.head->value_i);
    ASSERT_EQ(5, test_list.tail->value_i);

    ASSERT_EQ(6, test_list.length);

    list_clear(&test_list);
}

UTEST(list, test_list_sort_by_value_p) {
    struct t_list test_list;
    populate_list(&test_list);

    list_sort_by_key(&test_list, LIST_SORT_DESC);
    ASSERT_STREQ("value0", test_list.tail->value_p);
    ASSERT_STREQ("value5", test_list.head->value_p);

    list_sort_by_key(&test_list, LIST_SORT_ASC);
    ASSERT_STREQ("value0", test_list.head->value_p);
    ASSERT_STREQ("value5", test_list.tail->value_p);

    ASSERT_EQ(6, test_list.length);

    list_clear(&test_list);
}

UTEST(list, test_list_sort_by_key) {
    struct t_list test_list;
    populate_list(&test_list);

    list_sort_by_key(&test_list, LIST_SORT_DESC);
    ASSERT_STREQ("key0", test_list.tail->key);
    ASSERT_STREQ("key5", test_list.head->key);

    list_sort_by_key(&test_list, LIST_SORT_ASC);
    ASSERT_STREQ("key0", test_list.head->key);
    ASSERT_STREQ("key5", test_list.tail->key);

    ASSERT_EQ(6, test_list.length);

    list_clear(&test_list);
}

UTEST(list, test_list_swap_item_pos) {
    struct t_list test_list;
    populate_list(&test_list);

    struct t_list_node *current;

    list_swap_item_pos(&test_list, 3, 1);
    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("key3", current->key);
    current = list_node_at(&test_list, 3);
    ASSERT_STREQ("key1", current->key);

    ASSERT_EQ(6, test_list.length);

    list_clear(&test_list);
}

UTEST(list, test_list_move_item_pos) {
    struct t_list test_list;
    populate_list(&test_list);

    struct t_list_node *current;

    list_move_item_pos(&test_list, 4, 2);
    current = list_node_at(&test_list, 2);
    ASSERT_STREQ("key4", current->key);
    current = list_node_at(&test_list, 4);
    ASSERT_STREQ("key3", current->key);

    ASSERT_EQ(6, test_list.length);

    list_clear(&test_list);
}

UTEST(list, test_list_insert_sorted_by_key_limit_asc) {
    struct t_list test_list;
    list_init(&test_list);
    struct t_list_node *current;

    list_insert_sorted_by_key_limit(&test_list, "ddd", 1, "value1", NULL, LIST_SORT_ASC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("ddd", current->key);

    list_insert_sorted_by_key_limit(&test_list, "bbb", 1, "value1", NULL, LIST_SORT_ASC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("bbb", current->key);
    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("ddd", current->key);

    list_insert_sorted_by_key_limit(&test_list, "ccc", 1, "value1", NULL, LIST_SORT_ASC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("ccc", current->key);

    list_insert_sorted_by_key_limit(&test_list, "aaa", 1, "value1", NULL, LIST_SORT_ASC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("aaa", current->key);
    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("bbb", current->key);
    current = list_node_at(&test_list, 2);
    ASSERT_STREQ("ccc", current->key);

    list_insert_sorted_by_key_limit(&test_list, "xxx", 1, "value1", NULL, LIST_SORT_ASC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 2);
    ASSERT_STREQ("ccc", current->key);

    list_insert_sorted_by_key_limit(&test_list, "000", 1, "value1", NULL, LIST_SORT_ASC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 2);
    ASSERT_STREQ("bbb", current->key);

    ASSERT_STREQ("000", test_list.head->key);
    ASSERT_STREQ("bbb", test_list.tail->key);

    list_clear(&test_list);
}

UTEST(list, test_list_insert_sorted_by_key_limit_desc) {
    struct t_list test_list;
    list_init(&test_list);
    struct t_list_node *current;

    list_insert_sorted_by_key_limit(&test_list, "ddd", 1, "value1", NULL, LIST_SORT_DESC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("ddd", current->key);

    list_insert_sorted_by_key_limit(&test_list, "bbb", 1, "value1", NULL, LIST_SORT_DESC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("bbb", current->key);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("ddd", current->key);

    list_insert_sorted_by_key_limit(&test_list, "ccc", 1, "value1", NULL, LIST_SORT_DESC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("ccc", current->key);

    list_insert_sorted_by_key_limit(&test_list, "aaa", 1, "value1", NULL, LIST_SORT_DESC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("ddd", current->key);
    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("ccc", current->key);
    current = list_node_at(&test_list, 2);
    ASSERT_STREQ("bbb", current->key);

    list_insert_sorted_by_key_limit(&test_list, "xxx", 1, "value1", NULL, LIST_SORT_DESC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("xxx", current->key);

    list_insert_sorted_by_key_limit(&test_list, "000", 1, "value1", NULL, LIST_SORT_DESC, 3, list_free_cb_ignore_user_data);
    current = list_node_at(&test_list, 2);
    ASSERT_STREQ("ccc", current->key);

    ASSERT_STREQ("xxx", test_list.head->key);
    ASSERT_STREQ("ccc", test_list.tail->key);

    list_clear(&test_list);
}
