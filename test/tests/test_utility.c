/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/utest/utest.h"
#include "../../src/lib/sds_extras.h"
#include "../../src/lib/utility.h"
#include "../utility.h"

UTEST(utility, test_getenv_check) {
    setenv("TESTVAR", "testvalue", 0);
    const char *testvar = getenv_check("TESTVAR", 20);
    ASSERT_STREQ(testvar, "testvalue");

    const char *tolong = getenv_check("TESTVAR", 5);
    bool rc = tolong == NULL ? true : false;
    ASSERT_TRUE(rc);
    unsetenv("TESTVAR");
}

UTEST(utility, test_split_coverimage_names) {
    sds names = sdsnew("cover, folder");
    int count;
    sds *array = split_coverimage_names(names, &count);
    ASSERT_EQ(count, 2);
    ASSERT_STREQ(array[0], "cover");
    ASSERT_STREQ(array[1], "folder");
    sdsfree(names);
    sdsfreesplitres(array, count);
}

UTEST(utility, test_my_msleep) {
    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    my_msleep(300);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    double secs = (end.tv_nsec - begin.tv_nsec) / 1000000000.0 + (end.tv_sec  - begin.tv_sec);
    int msecs = (int)(secs * 1000);
    bool rc = false;
    if (msecs >= 290 && msecs <= 310) {
        rc = true;
    }
    ASSERT_TRUE(rc);
}

UTEST(utility, test_is_virtual_cuedir) {
    sds dir = sdsnew(".");
    sds file = sdsnew("test");
    bool rc = is_virtual_cuedir(dir, file);
    ASSERT_TRUE(rc);
    sdsfree(dir);
    sdsfree(file);

    dir = sdsnew("..");
    file = sdsnew("build");
    rc = is_virtual_cuedir(dir, file);
    ASSERT_FALSE(rc);
    sdsfree(dir);
    sdsfree(file);
}

UTEST(utility, test_is_stream) {
    bool rc = is_streamuri("https://github.com");
    ASSERT_TRUE(rc);

    rc = is_streamuri("/github.com");
    ASSERT_FALSE(rc);
}

UTEST(utility, test_get_extension_from_filename) {
    const char *ext = get_extension_from_filename("test.txt");
    ASSERT_STREQ(ext, "txt");

    const char *wo = get_extension_from_filename("test");
    ASSERT_TRUE(wo == NULL);
}

UTEST(sds_extras, test_basename_uri) {
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
        basename_uri(test_input);
        ASSERT_STREQ(p->result, test_input);
        sdsclear(test_input);
        p++;
    }
    sdsfree(test_input);
}

UTEST(sds_extras, test_strip_slash) {
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
        strip_slash(testfilename);
        ASSERT_STREQ(p->result, testfilename);
        sdsclear(testfilename);
        p++;
    }
    sdsfree(testfilename);
}

UTEST(sds_extras, test_strip_file_extension) {
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
        strip_file_extension(test_input);
        ASSERT_STREQ(p->result, test_input);
        sdsclear(test_input);
        p++;
    }
    sdsfree(test_input);
}

UTEST(sds_extras, test_sanitize_filename) {
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
        sanitize_filename(test_input);
        ASSERT_STREQ(p->result, test_input);
        sdsclear(test_input);
        p++;
    }
    sdsfree(test_input);
}
