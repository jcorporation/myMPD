/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/utest/utest.h"
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

UTEST(sds_extras, test_sds_sanitize_filename) {
    struct t_input_result testcases[] = {
        {"http://host:80/verz/verz/test?safsaf#798234",   "http___host_80_verz_verz_test_safsaf_798234" },
        {"https://host:443/verz/verz/test?safsaf#798234", "https___host_443_verz_verz_test_safsaf_798234" },
        {"https://host/verz/verz/test",                   "https___host_verz_verz_test" },
        {"",                                              "" },
        {"/test/test.mp3.mp3",                            "_test_test_mp3_mp3" },
        {NULL,                                            NULL}
    };
    struct t_input_result *p = testcases;
    sds test_input = sdsempty();
    while (p->input != NULL) {
        test_input = sdscatfmt(test_input, "%s", p->input);
        sds_sanitize_filename(test_input);
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
        {"/test/test.mp3",                                "test.mp3" },
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

UTEST(sds_extras, sds_urldecode) {
    sds test_input = sdsnew("/Musict/Led%20Zeppelin/1975%20-%20Physical%20Graffiti%20%5B1994%2C%20Atlantic%2C%207567-92442-2%5D/CD%201/folder.jpg");
    sds s = sds_urldecode(sdsempty(), test_input, sdslen(test_input), 0);
    ASSERT_STREQ("/Musict/Led Zeppelin/1975 - Physical Graffiti [1994, Atlantic, 7567-92442-2]/CD 1/folder.jpg", s);
    sdsfree(test_input);
    sdsfree(s);
}

UTEST(sds_extras, sds_urlencode) {
    sds test_input = sdsnew("/Musict/Led Zeppelin/1975 - Physical Graffiti [1994, Atlantic, 7567-92442-2]/CD 1/folder.jpg");
    sds s = sds_urlencode(sdsempty(), test_input, sdslen(test_input));
    ASSERT_STREQ("/Musict/Led%20Zeppelin/1975%20-%20Physical%20Graffiti%20%5B1994%2C%20Atlantic%2C%207567-92442-2%5D/CD%201/folder.jpg", s);
    sdsfree(test_input);
    sdsfree(s);
}

UTEST(sds_extras, sds_utf8_tolower) {
    sds test_input= sdsnew("EINSTÜRZENDE NEUBAUTEN");
    sds_utf8_tolower(test_input);
    ASSERT_STREQ("einstürzende neubauten", test_input);
    sdsclear(test_input);
    test_input = sdscat(test_input, "sdfßSdf");
    sds_utf8_tolower(test_input);
    ASSERT_STREQ("sdfßsdf", test_input);
    sdsfree(test_input);
}
