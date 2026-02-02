/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/sds/sds_json.h"

UTEST(sds_json, test_sds_catjson_plain) {
    sds s = sdsempty();
    const char *str = "test\"test";
    size_t len = strlen(str);
    s = sds_catjson_plain(s, str, len);
    ASSERT_STREQ("test\\\"test", s);
    ASSERT_EQ(strlen(s), sdslen(s));
    sdsfree(s);
}

UTEST(sds_json, test_sds_catjson_plain_long) {
    sds s = sdsempty();
    const char *str = "\"\"\"\"\"test\"\"\"\"\"\"test\"\"\"\"";
    size_t len = strlen(str);
    s = sds_catjson_plain(s, str, len);
    ASSERT_STREQ("\\\"\\\"\\\"\\\"\\\"test\\\"\\\"\\\"\\\"\\\"\\\"test\\\"\\\"\\\"\\\"", s);
    ASSERT_EQ(strlen(s), sdslen(s));
    sdsfree(s);
}

UTEST(sds_json, test_sds_catjsonchar) {
    sds s = sdsempty();
    s = sds_catjsonchar(s, '\n');
    ASSERT_STREQ("\\n", s);
    ASSERT_EQ(strlen(s), sdslen(s));
    sdsfree(s);
}

UTEST(sds_json, test_sds_catjson) {
    sds s = sdsempty();
    const char *str = "test\"test";
    size_t len = strlen(str);
    s = sds_catjson(s, str, len);
    ASSERT_STREQ("\"test\\\"test\"", s);
    ASSERT_EQ(strlen(s), sdslen(s));
    sdsfree(s);
}

UTEST(sds_json, test_sds_json_unescape) {
    sds s = sdsempty();
    const char *str = "test\"test";
    size_t len = strlen(str);
    s = sds_catjson_plain(s, str, len);
    ASSERT_STREQ("test\\\"test", s);
    sds dst = sdsempty();
    bool rc = sds_json_unescape(s, sdslen(s), &dst);
    ASSERT_TRUE(rc);
    ASSERT_STREQ(str, dst);
    ASSERT_EQ(strlen(dst), sdslen(dst));
    sdsfree(s);
    sdsfree(dst);
}
