/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/utf8_wrapper.h"

static const char *utf8_str_valid = "Abc123";
static const char *utf8_str_invalid = "\xf0\x28\x8c\x28";

UTEST(utf8wrap, test_utf8_wrap_validate) {
    size_t len = strlen(utf8_str_valid);
    bool rc = utf8_wrap_validate(utf8_str_valid, len);
    ASSERT_TRUE(rc);

    len = strlen(utf8_str_invalid);
    rc = utf8_wrap_validate(utf8_str_invalid, len);
    ASSERT_FALSE(rc);
}

UTEST(utf8wrap, utf8_wrap_casefold) {
    size_t len = strlen(utf8_str_valid);
    char *lower = utf8_wrap_casefold(utf8_str_valid, len);
    ASSERT_STREQ(lower, "abc123");
    free(lower);

    len = strlen(utf8_str_invalid);
    lower = utf8_wrap_casefold(utf8_str_invalid, len);
    ASSERT_STREQ(lower, utf8_str_invalid);
    free(lower);
}

UTEST(utf8wrap, utf8_wrap_normalize) {
    size_t len = strlen(utf8_str_valid);
    char *lower = utf8_wrap_normalize(utf8_str_valid, len);
    ASSERT_STREQ(lower, "abc123");
    free(lower);

    len = strlen(utf8_str_invalid);
    lower = utf8_wrap_normalize(utf8_str_invalid, len);
    ASSERT_STREQ(lower, utf8_str_invalid);
    free(lower);
}

UTEST(utf8wrap, utf8_wrap_casecmp) {
    size_t len = strlen(utf8_str_valid);
    int rc = utf8_wrap_casecmp(utf8_str_valid, len, "abc123", len);
    ASSERT_EQ(rc, 0);

    rc = utf8_wrap_casecmp(utf8_str_valid, len, "abc", len);
    ASSERT_EQ(rc, 1);

    len = strlen(utf8_str_invalid);
    rc = utf8_wrap_casecmp(utf8_str_invalid, len, utf8_str_invalid, len);
    ASSERT_EQ(rc, 0);

    rc = utf8_wrap_casecmp(utf8_str_invalid, len, "abc", len);
    ASSERT_EQ(rc, 1);
}
