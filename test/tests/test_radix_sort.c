/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "dist/rax/rax.h"
#include "dist/sds/sds.h"
#include "dist/utest/utest.h"

#include <inttypes.h>

UTEST(radix_sort, test_radix_sort_asc_number) {
    rax *sort = raxNew();
    sds key = sdsempty();
    for (int i = 0; i < 2000; i++) {
        sdsclear(key);
        key = sdscatprintf(key, "%020d", i);
        raxInsert(sort, (unsigned char *)key, sdslen(key), NULL, NULL);
    }

    raxIterator iter;
    raxStart(&iter, sort);
    raxSeek(&iter, "^", NULL, 0);
    int i = 0;
    while (raxNext(&iter)) {
        sdsclear(key);
        key = sdscatlen(key, iter.key, iter.key_len);
        int nr = strtoimax(key, NULL, 10);
        ASSERT_EQ(i, nr);
        i++;
    }
    sdsfree(key);
    raxFree(sort);
}

UTEST(radix_sort, test_radix_sort_desc_number) {
    rax *sort = raxNew();
    sds key = sdsempty();
    for (int i = 0; i < 2000; i++) {
        sdsclear(key);
        key = sdscatprintf(key, "%020d", i);
        raxInsert(sort, (unsigned char *)key, sdslen(key), NULL, NULL);
    }

    raxIterator iter;
    raxStart(&iter, sort);
    raxSeek(&iter, "$", NULL, 0);
    int i = 1999;
    while (raxPrev(&iter)) {
        sdsclear(key);
        key = sdscatlen(key, iter.key, iter.key_len);
        int nr = strtoimax(key, NULL, 10);
        ASSERT_EQ(i, nr);
        i--;
    }
    sdsfree(key);
    raxFree(sort);
}
