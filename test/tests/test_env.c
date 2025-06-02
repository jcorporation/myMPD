/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/env.h"
#include "src/lib/sds_extras.h"
#include "src/lib/validate.h"

#include <stdbool.h>
#include <stdlib.h>

UTEST(env, test_getenv_int) {
    setenv("TESTVAR", "10", 1);
    bool rc;
    int testvar = getenv_int("TESTVAR", 5, 0, 20, &rc);
    ASSERT_EQ(testvar, 10);
    ASSERT_TRUE(rc);
    
    setenv("TESTVAR", "30", 1);
    testvar = getenv_int("TESTVAR", 5, 0, 20, &rc);
    ASSERT_EQ(testvar, 5);
    ASSERT_FALSE(rc);
    unsetenv("TESTVAR");
}

UTEST(env, test_getenv_uint) {
    setenv("TESTVAR", "10", 1);
    bool rc;
    unsigned testvar = getenv_uint("TESTVAR", 5, 0, 20, &rc);
    ASSERT_EQ(testvar, (unsigned)10);
    ASSERT_TRUE(rc);

    setenv("TESTVAR", "30", 1);
    testvar = getenv_uint("TESTVAR", 5, 0, 20, &rc);
    ASSERT_EQ(testvar, (unsigned)5);
    ASSERT_FALSE(rc);
    unsetenv("TESTVAR");
}

UTEST(env, test_getenv_bool) {
    setenv("TESTVAR", "true", 1);
    bool rc;
    bool testvar = getenv_bool("TESTVAR", true, &rc);
    ASSERT_TRUE(testvar);
    ASSERT_TRUE(rc);

    setenv("TESTVAR", "false", 1);
    testvar = getenv_bool("TESTVAR", true, &rc);
    ASSERT_FALSE(testvar);
    ASSERT_TRUE(rc);

    setenv("TESTVAR", "30", 1);
    testvar = getenv_bool("TESTVAR", false, &rc);
    ASSERT_FALSE(testvar);
    ASSERT_FALSE(rc);
    unsetenv("TESTVAR");
}

UTEST(env, test_getenv_string) {
    setenv("TESTVAR", "testvalue", 1);
    bool rc;
    sds testvar = getenv_string("TESTVAR", "default", vcb_isname, &rc);
    ASSERT_STREQ(testvar, "testvalue");
    ASSERT_TRUE(rc);
    FREE_SDS(testvar);

    unsetenv("TESTVAR");
    testvar = getenv_string("TESTVAR", "default", vcb_isname, &rc);
    ASSERT_STREQ(testvar, "default");
    ASSERT_FALSE(rc);
    FREE_SDS(testvar);
}
