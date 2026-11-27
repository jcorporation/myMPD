/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "dist/utest/utest.h"
#include "src/lib/array/array.h"
#include "src/lib/array/array_shuffle.h"
#include "src/lib/array/array_sort.h"

static bool compare_value_i_desc(struct t_array_node *first, struct t_array_node *second,
        enum array_sort_direction direction)
{
    (void) direction;
    return first->value_i < second->value_i;
}

UTEST(array_sort, sort_by_value_i_and_callback) {
    struct t_array *array = array_new();
    ASSERT_TRUE(array_push(array, "four", NULL, 4, NULL));
    ASSERT_TRUE(array_push(array, "one-a", NULL, 1, NULL));
    ASSERT_TRUE(array_push(array, "one-b", NULL, 1, NULL));
    ASSERT_TRUE(array_push(array, "two", NULL, 2, NULL));

    ASSERT_TRUE(array_sort_by_value_i(array, ARRAY_SORT_ASC));
    ASSERT_EQ(1, array->nodes[0].value_i);
    ASSERT_STREQ("one-a", array->nodes[0].key);
    ASSERT_STREQ("one-b", array->nodes[1].key);
    ASSERT_EQ(4, array->nodes[3].value_i);

    ASSERT_TRUE(array_sort_by_callback(array, ARRAY_SORT_DESC, compare_value_i_desc));
    ASSERT_EQ(4, array->nodes[0].value_i);
    ASSERT_EQ(1, array->nodes[3].value_i);
    array_free(array, NULL);
}

UTEST(array_sort, sort_by_key_and_value_p) {
    struct t_array *array = array_new();
    ASSERT_TRUE(array_push(array, "b", "charlie", 0, NULL));
    ASSERT_TRUE(array_push(array, "a", "alpha", 0, NULL));
    ASSERT_TRUE(array_push(array, "c", "bravo", 0, NULL));

    ASSERT_TRUE(array_sort_by_key(array, ARRAY_SORT_ASC));
    ASSERT_STREQ("a", array->nodes[0].key);
    ASSERT_STREQ("c", array->nodes[2].key);

    ASSERT_TRUE(array_sort_by_value_p(array, ARRAY_SORT_DESC));
    ASSERT_STREQ("charlie", array->nodes[0].value_p);
    ASSERT_STREQ("alpha", array->nodes[2].value_p);

    array_free(array, NULL);
}

UTEST(array_sort, sort_by_value_i_key_value_p_with_nullable_keys) {
    struct t_array *array = array_new();
    ASSERT_TRUE(array_push(array, NULL, "b", 10, NULL));
    ASSERT_TRUE(array_push(array, "a", "c", 10, NULL));
    ASSERT_TRUE(array_push(array, "b", "a", 10, NULL));
    ASSERT_TRUE(array_push(array, NULL, "a", 10, NULL));
    ASSERT_TRUE(array_push(array, "a", "a", 10, NULL));

    ASSERT_TRUE(array_sort_by_value_i_key_value_p(array, ARRAY_SORT_ASC));
    ASSERT_STREQ("a", array->nodes[0].key);
    ASSERT_STREQ("a", array->nodes[0].value_p);
    ASSERT_STREQ("a", array->nodes[1].key);
    ASSERT_STREQ("c", array->nodes[1].value_p);
    ASSERT_STREQ("b", array->nodes[2].key);
    ASSERT_TRUE(array->nodes[3].key == NULL);
    ASSERT_STREQ("a", array->nodes[3].value_p);
    ASSERT_TRUE(array->nodes[4].key == NULL);
    ASSERT_STREQ("b", array->nodes[4].value_p);

    ASSERT_TRUE(array_sort_by_value_i_key_value_p(array, ARRAY_SORT_DESC));
    ASSERT_TRUE(array->nodes[0].key == NULL);
    ASSERT_STREQ("b", array->nodes[0].value_p);
    ASSERT_TRUE(array->nodes[1].key == NULL);
    ASSERT_STREQ("a", array->nodes[1].value_p);
    ASSERT_STREQ("b", array->nodes[2].key);
    ASSERT_STREQ("a", array->nodes[3].key);
    ASSERT_STREQ("c", array->nodes[3].value_p);
    ASSERT_STREQ("a", array->nodes[4].key);
    ASSERT_STREQ("a", array->nodes[4].value_p);

    array_free(array, NULL);
}

UTEST(array_sort, sort_by_key_value_p_value_i_with_null_keys) {
    struct t_array *array = array_new();
    ASSERT_TRUE(array_push(array, NULL, NULL, 0, NULL));
    ASSERT_TRUE(array_push(array, "b", "value", 0, NULL));
    ASSERT_TRUE(array_push(array, NULL, "first", 0, NULL));
    ASSERT_TRUE(array_push(array, "a", "value", 0, NULL));

    ASSERT_TRUE(array_sort_by_key_value_p_value_i(array, ARRAY_SORT_ASC));
    ASSERT_STREQ("a", array->nodes[0].key);
    ASSERT_STREQ("b", array->nodes[1].key);
    ASSERT_TRUE(array->nodes[2].key == NULL);
    ASSERT_STREQ("first", array->nodes[2].value_p);
    ASSERT_TRUE(array->nodes[3].key == NULL);
    ASSERT_TRUE(array->nodes[3].value_p == NULL);

    ASSERT_TRUE(array_sort_by_key_value_p_value_i(array, ARRAY_SORT_DESC));
    ASSERT_TRUE(array->nodes[0].key == NULL);
    ASSERT_TRUE(array->nodes[0].value_p == NULL);
    ASSERT_TRUE(array->nodes[1].key == NULL);
    ASSERT_STREQ("first", array->nodes[1].value_p);
    ASSERT_STREQ("b", array->nodes[2].key);
    ASSERT_STREQ("a", array->nodes[3].key);

    array_free(array, NULL);
}

#ifdef MYMPD_ENABLE_UTF8
UTEST(array_sort, sort_utf8_keys) {
    struct t_array *array = array_new();
    ASSERT_TRUE(array_push(array, "\xC3\x84pfel", NULL, 0, NULL));
    ASSERT_TRUE(array_push(array, "\xC3\xA4pfel", NULL, 0, NULL));
    ASSERT_TRUE(array_push(array, "Zebra", NULL, 0, NULL));

    ASSERT_TRUE(array_sort_by_key(array, ARRAY_SORT_ASC));
    ASSERT_STREQ("Zebra", array->nodes[0].key);
    ASSERT_STREQ("\xC3\x84pfel", array->nodes[1].key);
    ASSERT_STREQ("\xC3\xA4pfel", array->nodes[2].key);
    array_free(array, NULL);
}
#endif

UTEST(array_sort, empty_single_and_invalid_callback) {
    struct t_array *array = array_new();
    ASSERT_TRUE(array_sort_by_value_i(array, ARRAY_SORT_ASC));
    ASSERT_TRUE(array_shuffle(array));

    ASSERT_TRUE(array_push(array, "seven", NULL, 7, NULL));
    ASSERT_TRUE(array_sort_by_callback(array, ARRAY_SORT_ASC, NULL));
    ASSERT_TRUE(array_shuffle(array));
    ASSERT_FALSE(array_sort_by_value_i(array, (enum array_sort_direction)2));
    array_free(array, NULL);
}

UTEST(array, growth_clear_and_shuffle) {
    struct t_array *array = array_new();
    for (unsigned i = 0; i < 64; i++) {
        ASSERT_TRUE(array_push(array, "entry", NULL, i, NULL));
    }
    ASSERT_EQ(64U, array->length);
    ASSERT_TRUE(array->capacity >= 64);
    ASSERT_TRUE(array_shuffle(array));

    bool seen[64] = {false};
    for (unsigned i = 0; i < array->length; i++) {
        int64_t value = array->nodes[i].value_i;
        ASSERT_TRUE(value >= 0 && value < 64);
        ASSERT_FALSE(seen[value]);
        seen[value] = true;
    }
    for (unsigned i = 0; i < 64; i++) {
        ASSERT_TRUE(seen[i]);
    }

    array_clear(array, NULL);
    ASSERT_EQ(0U, array->length);
    ASSERT_TRUE(array_push(array, "reused", NULL, 1, NULL));
    array_free(array, NULL);
}
