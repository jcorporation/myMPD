/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/sds/sds.h"
#include "dist/utest/utest.h"
#include "src/lib/convert.h"

UTEST(convert, test_str2int) {
    int i;
    sds s = sdsnew("123");
    enum str2int_errno e = str2int(&i, s);
    ASSERT_EQ(123, i);
    ASSERT_EQ(STR2INT_SUCCESS, (int)e);
    sdsfree(s);

    s = sdsnew("-123");
    e = str2int(&i, s);
    ASSERT_EQ(-123, i);
    ASSERT_EQ(STR2INT_SUCCESS, (int)e);
    sdsfree(s);

    s = sdsnew("asdf123");
    e = str2int(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsnew(" 123");
    e = str2int(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsnew("123dsf");
    e = str2int(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsfromlonglong(LLONG_MAX);
    e = str2int(&i, s);
    ASSERT_EQ(STR2INT_OVERFLOW, (int)e);
    sdsfree(s);

    s = sdsfromlonglong(LLONG_MIN);
    e = str2int(&i, s);
    ASSERT_EQ(STR2INT_UNDERFLOW, (int)e);
    sdsfree(s);
}

UTEST(convert, test_str2uint) {
    unsigned i;
    sds s = sdsnew("123");
    enum str2int_errno e = str2uint(&i, s);
    ASSERT_EQ(123U, i);
    ASSERT_EQ(STR2INT_SUCCESS, (int)e);
    sdsfree(s);

    s = sdsnew("-123");
    e = str2uint(&i, s);
    ASSERT_EQ(STR2INT_UNDERFLOW, (int)e);
    sdsfree(s);

    s = sdsnew("asdf123");
    e = str2uint(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsnew(" 123");
    e = str2uint(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsnew("123dsf");
    e = str2uint(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsfromlonglong(LLONG_MAX);
    e = str2uint(&i, s);
    ASSERT_EQ(STR2INT_OVERFLOW, (int)e);
    sdsfree(s);

    s = sdsfromlonglong(LLONG_MIN);
    e = str2uint(&i, s);
    ASSERT_EQ(STR2INT_UNDERFLOW, (int)e);
    sdsfree(s);
}

UTEST(convert, test_str2int64) {
    int64_t i;
    sds s = sdsnew("123");
    enum str2int_errno e = str2int64(&i, s);
    ASSERT_EQ(123, i);
    ASSERT_EQ(STR2INT_SUCCESS, (int)e);
    sdsfree(s);

    s = sdsnew("-123");
    e = str2int64(&i, s);
    ASSERT_EQ(-123, i);
    ASSERT_EQ(STR2INT_SUCCESS, (int)e);
    sdsfree(s);

    s = sdsnew("asdf123");
    e = str2int64(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsnew(" 123");
    e = str2int64(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsnew("123dsf");
    e = str2int64(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsfromlonglong(LLONG_MAX);
    s = sdscat(s, "1");
    e = str2int64(&i, s);
    ASSERT_EQ(STR2INT_OVERFLOW, (int)e);
    sdsfree(s);

    s = sdsfromlonglong(LLONG_MIN);
    s = sdscat(s, "1");
    e = str2int64(&i, s);
    ASSERT_EQ(STR2INT_UNDERFLOW, (int)e);
    sdsfree(s);
}

UTEST(convert, test_str2float) {
    float i;
    char str[20];
    sds s = sdsnew("123.234561");
    enum str2int_errno e = str2float(&i, s);
    sprintf(str, "%.4f", i);
    ASSERT_STREQ("123.2346", str);
    ASSERT_EQ(STR2INT_SUCCESS, (int)e);
    sdsfree(s);

    s = sdsnew("-123.234561");
    e = str2float(&i, s);
    sprintf(str, "%.4f", i);
    ASSERT_STREQ("-123.2346", str);
    ASSERT_EQ(STR2INT_SUCCESS, (int)e);
    sdsfree(s);

    s = sdsnew("asdf123");
    e = str2float(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsnew(" 123");
    e = str2float(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsnew("123dsf");
    e = str2float(&i, s);
    ASSERT_EQ(STR2INT_INCONVERTIBLE, (int)e);
    sdsfree(s);

    s = sdsfromlonglong(LLONG_MAX);
    s = sdscat(s, "1111111111111111111111111111");
    e = str2float(&i, s);
    ASSERT_EQ(STR2INT_OVERFLOW, (int)e);
    sdsfree(s);

    s = sdsfromlonglong(LLONG_MIN);
    s = sdscat(s, "1111111111111111111111111111");
    e = str2float(&i, s);
    ASSERT_EQ(STR2INT_UNDERFLOW, (int)e);
    sdsfree(s);
}
