/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/list/list.h"
#include "src/lib/list/sort.h"
#include "src/lib/utf8_wrapper.h"

UTEST(list_sort, list_sort_by_value_i_desc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_sort_by_value_i(&test_list, LIST_SORT_DESC);
    ASSERT_EQ(0, test_list.tail->value_i);
    ASSERT_EQ(5, test_list.head->value_i);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list_sort, list_sort_by_value_i_asc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_sort_by_value_i(&test_list, LIST_SORT_ASC);
    ASSERT_EQ(0, test_list.head->value_i);
    ASSERT_EQ(5, test_list.tail->value_i);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);
    
    list_clear(&test_list);
}

UTEST(list_sort, test_list_sort_by_value_p_desc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_sort_by_value_p(&test_list, LIST_SORT_DESC);
    ASSERT_STREQ("value0", test_list.tail->value_p);
    ASSERT_STREQ("value5", test_list.head->value_p);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list_sort, test_list_sort_by_value_p_asc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_sort_by_value_p(&test_list, LIST_SORT_ASC);
    ASSERT_STREQ("value0", test_list.head->value_p);
    ASSERT_STREQ("value5", test_list.tail->value_p);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list_sort, test_list_sort_by_key_desc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);

    list_sort_by_key(&test_list, LIST_SORT_DESC);
    ASSERT_STREQ("key0", test_list.tail->key);
    ASSERT_STREQ("key5", test_list.head->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list_sort, test_list_sort_by_key_asc) {
    struct t_list test_list;
    unsigned expected_len = populate_list(&test_list);
    
    list_sort_by_key(&test_list, LIST_SORT_ASC);
    ASSERT_STREQ("key0", test_list.head->key);
    ASSERT_STREQ("key5", test_list.tail->key);
    ASSERT_EQ(check_list_integrity(&test_list, expected_len), true);

    list_clear(&test_list);
}

UTEST(list_sort, test_list_sort_large_by_key_asc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true, 2000);

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

UTEST(list_sort, test_list_sort_large_by_key_desc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true, 2000);

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

UTEST(list_sort, test_list_sort_large_by_value_p_asc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true, 2000);

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

UTEST(list_sort, test_list_sort_large_by_value_p_desc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true, 2000);

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

UTEST(list_sort, test_list_sort_large_by_value_i_asc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true, 2000);

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

UTEST(list_sort, test_list_sort_large_by_value_i_desc) {
    struct t_list *test_list = list_new();
    unsigned expected_len = populate_large_list(test_list, true, 2000);

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

UTEST(list_sort, test_list_sort_empty) {
    struct t_list test_list;
    list_init(&test_list);

    list_sort_by_value_i(&test_list, LIST_SORT_DESC);
    ASSERT_EQ(check_list_integrity(&test_list, 0), true);

    list_push(&test_list, "test", 0, NULL, NULL);
    list_sort_by_value_i(&test_list, LIST_SORT_DESC);
    ASSERT_EQ(check_list_integrity(&test_list, 1), true);

    list_clear(&test_list);
}
