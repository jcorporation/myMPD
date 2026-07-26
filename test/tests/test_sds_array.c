/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/random.h"
#include "src/lib/sds/sds_array.h"
#include "src/lib/utf8_wrapper.h"

UTEST(sds_array, test_sds_array_push) {
    struct t_sds_array *array = sds_array_new();
    ASSERT_EQ(array->length, 0U);
    unsigned capacity = SDS_ARRAY_START_CAPACITY;
    ASSERT_EQ(array->capacity, capacity);
    unsigned len = 2000;

    for (unsigned i = 0; i < len; i++) {
        sds s = sdscatprintf(sdsempty(), "test-%u", i);
        sds_array_push(array, s);
    }
    ASSERT_EQ(array->length, len);
    ASSERT_GE(array->capacity, array->length);
    for (unsigned i = 0; i < len; i++) {
        sds s = sdscatprintf(sdsempty(), "test-%u", i);
        ASSERT_STREQ(s, array->items[i]);
        sdsfree(s);
    }
    sds_array_clear(array);
    ASSERT_EQ(array->length, 0U);
    ASSERT_GE(array->capacity, array->length);

    sds_array_push(array, sdsnew("test"));
    sds_array_free(array);
}

UTEST(sds_array, test_sds_array_shuffle) {
    unsigned len = 2000;
    struct t_sds_array *array = sds_array_new();
    for (unsigned i = 0; i < len; i++) {
        sds s = sdscatprintf(sdsempty(), "test-%u", i);
        sds_array_push(array, s);
    }
    bool rc = sds_array_shuffle(array);
    ASSERT_TRUE(rc);
    unsigned eq = 0;
    for (unsigned i = 0; i < len; i++) {
        sds s = sdscatprintf(sdsempty(), "test-%u", i);
        if (strcmp(array->items[i], s) == 0) {
            eq++;
        }
        sdsfree(s);
    }
    ASSERT_NE(eq, len);

    ASSERT_EQ(array->length, len);
    ASSERT_GE(array->capacity, array->length);

    sds_array_clear(array);
    rc = sds_array_shuffle(array);
    ASSERT_TRUE(rc);

    sds_array_free(array);
}

UTEST(sds_array, test_sds_array_sort) {
    unsigned len = 2000;
    struct t_sds_array *array = sds_array_new();

    char buffer1[101];
    for (unsigned i = 0; i < len; i++) {
        randstring(buffer1, 100);
        sds_array_push(array, sdsnew(buffer1));
    }
    bool rc = sds_array_sort(array, false);
    ASSERT_TRUE(rc);

    for (unsigned i = 0; i < len; i++) {
        for (unsigned j = i + 1; j < len; j++) {
            ASSERT_LE(utf8_wrap_casecmp(array->items[i], sdslen(array->items[i]), array->items[j], sdslen(array->items[j])), 0);
        }
    }
    ASSERT_EQ(array->length, len);
    ASSERT_GE(array->capacity, array->length);

    sds_array_free(array);
}
