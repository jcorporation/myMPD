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
#include "src/lib/list/sort.h"
#include "src/lib/random.h"
#include "src/lib/utf8_wrapper.h"

static long populate_list(struct t_list *l) {
    list_init(l);
    list_push(l, "key1", 1, "value1", NULL);
    list_push(l, "key2", 2, "value2", NULL);
    list_push(l, "key3", 3, "value3", NULL);
    list_push(l, "key4", 4, "value4", NULL);
    list_push(l, "key5", 5, "value5", NULL);
    list_insert(l, "key0", 0, "value0", NULL);
    return l->length;
}

static long populate_large_list(struct t_list *l, bool shuffled) {
    list_init(l);
    char buffer1[21];
    char buffer2[21];
    for (int i = 0; i < 10000; i++) {
        randstring(buffer1, 21);
        randstring(buffer2, 21);
        list_push(l, buffer1, i, buffer2, NULL);
    }
    if (shuffled == true) {
        list_shuffle(l);
    }
    return l->length;
}

static bool check_list_integrity(struct t_list *l, unsigned expected_len) {
    struct t_list_node *current = l->head;
    unsigned len = 0;
    // Iterate through the list
    while (current != NULL) {
        len++;
        if (current->next == NULL) {
            break;
        }
        current = current->next;
    }
    if (len != expected_len) {
        return false;
    }
    // Empty list
    if (current == NULL &&
        l->tail == NULL &&
        len == 0)
    {
        return true;
    }
    // current is now the last node in the list -> tail
    if (current != l->tail) {
        return false;
    }
    // Check length
    if (len != l->length) {
        return false;
    }
    // Tails next must be NULL
    if (current->next != NULL) {
        return false;
    }
    if (l->tail->next != NULL) {
        return false;
    }
    return true;
}

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

UTEST(list, list_sort_by_value_i_desc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_sort_by_value_i(&test_list, LIST_SORT_DESC);
    ASSERT_EQ(0, test_list.tail->value_i);
    ASSERT_EQ(5, test_list.head->value_i);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, list_sort_by_value_i_asc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_sort_by_value_i(&test_list, LIST_SORT_ASC);
    ASSERT_EQ(0, test_list.head->value_i);
    ASSERT_EQ(5, test_list.tail->value_i);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);
    
    list_clear(&test_list);
}

UTEST(list, test_list_sort_by_value_p_desc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_sort_by_value_p(&test_list, LIST_SORT_DESC);
    ASSERT_STREQ("value0", test_list.tail->value_p);
    ASSERT_STREQ("value5", test_list.head->value_p);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_sort_by_value_p_asc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_sort_by_value_p(&test_list, LIST_SORT_ASC);
    ASSERT_STREQ("value0", test_list.head->value_p);
    ASSERT_STREQ("value5", test_list.tail->value_p);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_sort_by_key_desc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_sort_by_key(&test_list, LIST_SORT_DESC);
    ASSERT_STREQ("key0", test_list.tail->key);
    ASSERT_STREQ("key5", test_list.head->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list, test_list_sort_by_key_asc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    
    list_sort_by_key(&test_list, LIST_SORT_ASC);
    ASSERT_STREQ("key0", test_list.head->key);
    ASSERT_STREQ("key5", test_list.tail->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
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

UTEST(list, test_list_large_shuffle) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true);

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
    ASSERT_LT(eq, 10U);
    ASSERT_EQ(check_list_integrity(test_list, expected_len), true);

    list_free(test_list);
}

UTEST(list, test_list_sort_large_by_key_asc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true);

    list_sort_by_key(test_list, LIST_SORT_ASC);
    struct t_list_node *current = test_list->head;
    while (current != NULL) {
        if (current->next != NULL) {
            ASSERT_LE(utf8_wrap_casecmp(current->key, sdslen(current->key), current->next->key, sdslen(current->next->key)), 0);
        }
        current = current->next;
    }
    ASSERT_EQ(check_list_integrity(test_list, expected_len), true);

    list_free(test_list);
}

UTEST(list, test_list_sort_large_by_key_desc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true);

    list_sort_by_key(test_list, LIST_SORT_DESC);
    struct t_list_node *current = test_list->head;
    while (current != NULL) {
        if (current->next != NULL) {
            ASSERT_GE(utf8_wrap_casecmp(current->key, sdslen(current->key), current->next->key, sdslen(current->next->key)), 0);
        }
        current = current->next;
    }
    ASSERT_EQ(check_list_integrity(test_list, expected_len), true);

    list_free(test_list);
}

UTEST(list, test_list_sort_large_by_value_p_asc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true);

    list_sort_by_value_p(test_list, LIST_SORT_ASC);
    struct t_list_node *current = test_list->head;
    while (current != NULL) {
        if (current->next != NULL) {
            ASSERT_LE(utf8_wrap_casecmp(current->value_p, sdslen(current->value_p), current->next->value_p, sdslen(current->next->value_p)), 0);
        }
        current = current->next;
    }
    ASSERT_EQ(check_list_integrity(test_list, expected_len), true);

    list_free(test_list);
}

UTEST(list, test_list_sort_large_by_value_p_desc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true);

    list_sort_by_value_p(test_list, LIST_SORT_DESC);
    struct t_list_node *current = test_list->head;
    while (current != NULL) {
        if (current->next != NULL) {
            ASSERT_GE(utf8_wrap_casecmp(current->value_p, sdslen(current->value_p), current->next->value_p, sdslen(current->next->value_p)), 0);
        }
        current = current->next;
    }
    ASSERT_EQ(check_list_integrity(test_list, expected_len), true);

    list_free(test_list);
}

UTEST(list, test_list_sort_large_by_value_i_asc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true);

    list_sort_by_value_i(test_list, LIST_SORT_ASC);
    struct t_list_node *current = test_list->head;
    while (current != NULL) {
        if (current->next != NULL) {
            ASSERT_TRUE(current->value_i <= current->next->value_i);
        }
        current = current->next;
    }
    ASSERT_EQ(check_list_integrity(test_list, expected_len), true);

    list_free(test_list);
}

UTEST(list, test_list_sort_large_by_value_i_desc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true);

    list_sort_by_value_i(test_list, LIST_SORT_DESC);
    struct t_list_node *current = test_list->head;
    while (current != NULL) {
        if (current->next != NULL) {
            ASSERT_FALSE(current->value_i <= current->next->value_i);
        }
        current = current->next;
    }
    ASSERT_EQ(check_list_integrity(test_list, expected_len), true);

    list_free(test_list);
}
