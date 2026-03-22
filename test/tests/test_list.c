/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/list/list.h"

UTEST(list, test_list_push) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    list_push(&test_list, "last", 6, "value6", NULL);
    expected_len++;
    ASSERT_STREQ("last", test_list.tail->key);
    current = list_node_at(&test_list, test_list.length - 1);
    ASSERT_STREQ(current->key, test_list.tail->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_insert) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    list_insert(&test_list, "first", -1, "value-1", NULL);
    expected_len++;
    ASSERT_STREQ("first", test_list.head->key);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ(current->key, test_list.head->key);

    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_insert_empty) {
    struct t_list test_list;
    list_init(&test_list);

    list_insert(&test_list, "first", -1, "value-1", NULL);
    unsigned expected_len = 1;
    ASSERT_STREQ("first", test_list.head->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_remove_node) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    struct t_list_node *current;
    //remove middle item
    list_remove_node(&test_list, 3);
    current = list_node_at(&test_list, 4);
    ASSERT_STREQ("key5", current->key);
    expected_len--;
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    //remove last item
    list_remove_node(&test_list, test_list.length - 1);
    ASSERT_STREQ("key4", test_list.tail->key);
    expected_len--;
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    //remove first item
    list_remove_node(&test_list, 0);
    ASSERT_STREQ("key1", test_list.head->key);
    expected_len--;
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_remove_nody_by_key) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    struct t_list_node *current;
    //remove middle item
    list_remove_node_by_key(&test_list, "key3");
    current = list_node_at(&test_list, 3);
    ASSERT_STREQ("key4", current->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len - 1), true);

    list_clear(&test_list);
}

UTEST(list, test_list_replace) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    //replace middle item
    list_replace(&test_list, 4, "replace0", 0, NULL, NULL);
    current = list_node_at(&test_list, 4);
    ASSERT_STREQ("replace0", current->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    //replace last item
    list_replace(&test_list, test_list.length - 1, "replace1", 0, NULL, NULL);
    ASSERT_STREQ("replace1", test_list.tail->key);
    current = list_node_at(&test_list, 5);
    ASSERT_STREQ(current->key, test_list.tail->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    //replace first item
    list_replace(&test_list, 0, "replace2", 0, NULL, NULL);
    ASSERT_STREQ("replace2", test_list.head->key);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ(current->key, test_list.head->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_move_item_pos_4_2) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    list_move_item_pos(&test_list, 4, 2);
    current = list_node_at(&test_list, 2);
    ASSERT_STREQ("key4", current->key);

    current = list_node_at(&test_list, 4);
    ASSERT_STREQ("key3", current->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_move_item_pos_4_3) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    list_move_item_pos(&test_list, 4, 3);
    current = list_node_at(&test_list, 3);
    ASSERT_STREQ("key4", current->key);

    current = list_node_at(&test_list, 4);
    ASSERT_STREQ("key3", current->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_move_item_pos_2_4) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    list_move_item_pos(&test_list, 2, 4);
    current = list_node_at(&test_list, 4);
    ASSERT_STREQ("key2", current->key);

    current = list_node_at(&test_list, 2);
    ASSERT_STREQ("key3", current->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_move_item_pos_2_3) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    list_move_item_pos(&test_list, 2, 3);
    current = list_node_at(&test_list, 3);
    ASSERT_STREQ("key2", current->key);

    current = list_node_at(&test_list, 2);
    ASSERT_STREQ("key3", current->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_move_item_pos_to_start) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    list_move_item_pos(&test_list, 1, 0);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("key1", current->key);
    ASSERT_STREQ("key1", test_list.head->key);

    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("key0", current->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_move_item_pos_to_end) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    const long last_idx = test_list.length - 1;
    list_move_item_pos(&test_list, 1, last_idx);
    current = list_node_at(&test_list, last_idx);
    ASSERT_STREQ("key1", current->key);
    ASSERT_STREQ("key1", test_list.tail->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_move_item_pos_from_start) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    list_move_item_pos(&test_list, 0, 2);
    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("key1", current->key);
    ASSERT_STREQ("key1", test_list.head->key);

    current = list_node_at(&test_list, 2);
    ASSERT_STREQ("key0", current->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_move_item_pos_from_end) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    const long last_idx = test_list.length - 1;
    list_move_item_pos(&test_list, last_idx, 1);
    current = list_node_at(&test_list, last_idx);
    ASSERT_STREQ("key4", current->key);
    ASSERT_STREQ("key4", test_list.tail->key);

    current = list_node_at(&test_list, 1);
    ASSERT_STREQ("key5", current->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_move_item_pos_from_start_to_end) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    const long last_idx = test_list.length - 1;
    list_move_item_pos(&test_list, 0, last_idx);
    current = list_node_at(&test_list, last_idx);
    ASSERT_STREQ("key0", current->key);
    ASSERT_STREQ("key0", test_list.tail->key);

    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("key1", current->key);
    ASSERT_STREQ("key1", test_list.head->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_move_item_pos_from_end_to_start) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    struct t_list_node *current;

    const long last_idx = test_list.length - 1;
    list_move_item_pos(&test_list, last_idx, 0);
    current = list_node_at(&test_list, last_idx);
    ASSERT_STREQ("key4", current->key);
    ASSERT_STREQ("key4", test_list.tail->key);

    current = list_node_at(&test_list, 0);
    ASSERT_STREQ("key5", current->key);
    ASSERT_STREQ("key5", test_list.head->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

sds write_disk_cb(sds buffer, struct t_list_node *current, bool newline) {
    buffer = sdscatsds(buffer, current->key);
    if (newline == true) {
        buffer = sdscatlen(buffer, "\n", 1);
    }
    return buffer;
}

UTEST(list, test_list_write_to_disk) {
    init_testenv();

    struct t_list test_list;
    populate_list(&test_list);
    sds filepath = sdsnew("/tmp/mympd-test/state/test_list");
    bool rc = list_write_to_disk(filepath, &test_list, write_disk_cb);
    ASSERT_TRUE(rc);
    unlink(filepath);
    sdsfree(filepath);
    list_clear(&test_list);

    clean_testenv();
}

UTEST(list, test_list_crop_0) {
    struct t_list test_list;
    populate_list(&test_list);

    list_crop(&test_list, 0, NULL);
    ASSERT_EQ(check_list_integrity(&test_list, 0), true);
    ASSERT_TRUE(test_list.tail == NULL);
    ASSERT_TRUE(test_list.head == NULL);

    list_clear(&test_list);
}

UTEST(list, test_list_crop_2) {
    struct t_list test_list;
    populate_list(&test_list);

    list_crop(&test_list, 2, NULL);
    ASSERT_EQ(check_list_integrity(&test_list, 2), true);

    list_clear(&test_list);
}

UTEST(list, test_list_crop_gt_len) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_crop(&test_list, expected_len + 10, NULL);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_crop_len) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_crop(&test_list, expected_len, NULL);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_append) {
    struct t_list src;
    unsigned expected_len = populate_list(&src);
    struct t_list dst;
    list_init(&dst);

    list_append(&dst, &src);
    ASSERT_EQ(check_list_integrity(&dst, expected_len), true);
    list_append(&dst, &src);
    ASSERT_EQ(check_list_integrity(&dst, expected_len * 2), true);

    list_clear(&src);
    list_clear(&dst);
}

UTEST(list, test_list_dup) {
    struct t_list src;
    unsigned expected_len = populate_list(&src);

    struct t_list *new = list_dup(&src);
    ASSERT_EQ(check_list_integrity(new, expected_len), true);

    list_clear(&src);
    list_free(new);
}
