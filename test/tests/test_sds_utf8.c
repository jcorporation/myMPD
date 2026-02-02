/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/sds/sds_utf8.h"

UTEST(sds_utf8, test_sds_utf8_normalize) {
    sds test_input= sdsnew("EINSTÜRZENDE NEUBAUTEN");
    test_input = sds_utf8_normalize(test_input);
    ASSERT_STREQ("einsturzende neubauten", test_input);
    sdsclear(test_input);
    test_input = sdscat(test_input, "sdfßSdf");
    test_input = sds_utf8_normalize(test_input);
    ASSERT_STREQ("sdfsssdf", test_input);
    sdsfree(test_input);
}

UTEST(sds_utf8, test_sds_utf8_casefold) {
    sds test_input= sdsnew("EINSTÜRZENDE NEUBAUTEN");
    test_input = sds_utf8_casefold(test_input);
    ASSERT_STREQ("einstürzende neubauten", test_input);
    sdsclear(test_input);
    test_input = sdscat(test_input, "sdfßSdf");
    test_input = sds_utf8_casefold(test_input);
    ASSERT_STREQ("sdfsssdf", test_input);
    sdsfree(test_input);
}
