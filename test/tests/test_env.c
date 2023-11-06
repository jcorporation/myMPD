/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
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
    int testvar = getenv_int("TESTVAR", 5, 0, 20);
    ASSERT_EQ(testvar, 10);
    
    setenv("TESTVAR", "30", 1);
    testvar = getenv_int("TESTVAR", 5, 0, 20);
    ASSERT_EQ(testvar, 5);
    unsetenv("TESTVAR");
}

UTEST(env, test_getenv_uint) {
    setenv("TESTVAR", "10", 1);
    unsigned testvar = getenv_uint("TESTVAR", 5, 0, 20);
    ASSERT_EQ(testvar, (unsigned)10);

    setenv("TESTVAR", "30", 1);
    testvar = getenv_uint("TESTVAR", 5, 0, 20);
    ASSERT_EQ(testvar, (unsigned)5);
    unsetenv("TESTVAR");
}

UTEST(env, test_getenv_bool) {
    setenv("TESTVAR", "true", 1);
    bool testvar = getenv_bool("TESTVAR", true);
    ASSERT_TRUE(testvar == true);

    setenv("TESTVAR", "30", 1);
    testvar = getenv_bool("TESTVAR", false);
    ASSERT_TRUE(testvar == false);
    unsetenv("TESTVAR");
}

UTEST(env, test_getenv_string) {
    setenv("TESTVAR", "testvalue", 1);
    sds testvar = getenv_string("TESTVAR", "default", vcb_isname);
    ASSERT_STREQ(testvar, "testvalue");
    FREE_SDS(testvar);

    unsetenv("TESTVAR");
    testvar = getenv_string("TESTVAR", "default", vcb_isname);
    ASSERT_STREQ(testvar, "default");
    FREE_SDS(testvar);
}
