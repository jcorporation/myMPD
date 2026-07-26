/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/sds/sds_array.h"

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
    sds_array_free(array);
}

UTEST(sds_array, test_sds_array_shuffle) {
    unsigned len = 2000;
    struct t_sds_array *array = sds_array_new();
    for (unsigned i = 0; i < len; i++) {
        sds s = sdscatprintf(sdsempty(), "test-%u", i);
        sds_array_push(array, s);
    }
    sds_array_shuffle(array);
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
    sds_array_free(array);
}
