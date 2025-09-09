/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/json/json_print.h"

UTEST(jsonprint, test_tojson_float) {
    // nan handling
    float f = strtof("nan", NULL);
    sds s = sdsempty();
    s = tojson_float(s, "float", f, false);
    ASSERT_STREQ("\"float\":null", s);

    // inf handling
    f = strtof("inf", NULL);
    sdsclear(s);
    s = tojson_float(s, "float", f, false);
    ASSERT_STREQ("\"float\":null", s);

    // string
    f = strtof("abc", NULL);
    sdsclear(s);
    s = tojson_float(s, "float", f, false);
    ASSERT_STREQ("\"float\":0.00", s);

    // floating point number
    f = strtof("1.234", NULL);
    sdsclear(s);
    s = tojson_float(s, "float", f, false);
    ASSERT_STREQ("\"float\":1.23", s);

    sdsfree(s);
}
