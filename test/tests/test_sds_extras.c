/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/src/utest/utest.h"
#include "../../src/lib/sds_extras.h"
#include "../utility.h"

UTEST(sds_extras, test_sds_strip_file_extension) {
    struct t_input_result testcases[] = {
        {"/test/test.mp3",   "/test/test"},
        {"/test/woext",      "/test/woext"},
        {"",                 ""},
        {"/tes/tet.mp3.mp3", "/tes/tet.mp3"},
        {NULL,               NULL}
    };
    struct t_input_result *p = testcases;
    sds test_input = sdsempty();
    while (p->input != NULL) {
        test_input = sdscatfmt(test_input, "%s", p->input);
        sds_strip_file_extension(test_input);
        ASSERT_STREQ(p->result, test_input);
        sdsclear(test_input);
        p++;
    }
    sdsfree(test_input);
}

UTEST(sds_extras, test_sds_streamuri_to_filename) {
    struct t_input_result testcases[] = {
        {"http://host:80/verz/verz/test?safsaf#798234",   "host_80_verz_verz_test_safsaf_798234" },
        {"https://host:443/verz/verz/test?safsaf#798234", "host_443_verz_verz_test_safsaf_798234" },
        {"https://host/verz/verz/test",                   "host_verz_verz_test" },
        {"",                                              "" },
        {"/test/test.mp3.mp3",                            "_test_test_mp3_mp3" },
        {NULL,                                            NULL}
    };
    struct t_input_result *p = testcases;
    sds test_input = sdsempty();
    while (p->input != NULL) {
        test_input = sdscatfmt(test_input, "%s", p->input);
        sds_streamuri_to_filename(test_input);
        ASSERT_STREQ(p->result, test_input);
        sdsclear(test_input);
        p++;
    }
    sdsfree(test_input);
}

UTEST(sds_extras, sds_get_extension_from_filename) {
    struct t_input_result testcases[] = {
        {"/test/test.mp3",   "mp3"},
        {"/test/woext",      ""},
        {"",                 ""},
        {"/tes/tet.mp3.mp3", "mp3"},
        {"/",                ""},
        {NULL,               NULL}
    };
    struct t_input_result *p = testcases;
    while (p->input != NULL) {
        sds ext = sds_get_extension_from_filename(p->input);
        ASSERT_STREQ(p->result, ext);
        sdsfree(ext);
        p++;
    }
}

UTEST(sds_extras, sds_strip_slash) {
    struct t_input_result testcases[] = {
        {"//",           ""},
        {"/test/woext/", "/test/woext"},
        {"",             ""},
        {"sdf/",         "sdf"},
        {"/",            ""},
        {NULL,           NULL}
    };
    struct t_input_result *p = testcases;
    sds testfilename = sdsempty();
    while (p->input != NULL) {
        testfilename = sdscatfmt(testfilename, "%s", *p);
        sds_strip_slash(testfilename);
        ASSERT_STREQ(p->result, testfilename);
        sdsclear(testfilename);
        p++;
    }
    sdsfree(testfilename);
}

UTEST(sds_extras, sds_basename_uri) {
    struct t_input_result testcases[] = {
        {"http://host:80/verz/verz/test?safsaf#798234",   "http://host:80/verz/verz/test" },
        {"https://host:443/verz/verz/test?safsaf#798234", "https://host:443/verz/verz/test" },
        {"https://host/verz/verz/test",                   "https://host/verz/verz/test" },
        {"",                                              "" },
        {"/test/test.mp3",                            "test.mp3" },
        {NULL,                                            NULL}
    };
    struct t_input_result *p = testcases;
    sds test_input = sdsempty();
    while (p->input != NULL) {
        test_input = sdscatfmt(test_input, "%s", p->input);
        sds_basename_uri(test_input);
        ASSERT_STREQ(p->result, test_input);
        sdsclear(test_input);
        p++;
    }
    sdsfree(test_input);
}
