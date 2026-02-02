/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/sds/sds_extras.h"

UTEST(sds_extras, test_sds_split_comma_trim) {
    sds names = sdsnew("cover, folder");
    int count;
    sds *array = sds_split_comma_trim(names, &count);
    ASSERT_EQ(count, 2);
    ASSERT_STREQ(array[0], "cover");
    ASSERT_STREQ(array[1], "folder");
    sdsfree(names);
    sdsfreesplitres(array, count);
}

UTEST(sds_extras, test_sds_catchar) {
    sds s = sdsempty();
    s = sds_catchar(s, 'a');
    ASSERT_STREQ("a", s);
    ASSERT_EQ(strlen(s), sdslen(s));

    s = sdscat(s, "bcdefghijklmnopqrstuvwxy");
    s = sds_catchar(s, 'z');
    ASSERT_STREQ("abcdefghijklmnopqrstuvwxyz", s);
    ASSERT_EQ(strlen(s), sdslen(s));

    sdsfree(s);
}

UTEST(sds_extras, test_sds_replace) {
    sds s = sdsnew("old");
    s = sds_replace(s, "new");
    ASSERT_STREQ("new", s);
    sdsfree(s);
    //should also work with NULL ptr
    s = NULL;
    s = sds_replace(s, "new");
    ASSERT_STREQ("new", s);
    sdsfree(s);
}

UTEST(sds_extras, test_sds_catbool) {
    sds s = sdsempty();
    s = sds_catbool(s, true);
    ASSERT_STREQ("true", s);
    sdsfree(s);
}
