/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"

#include <pwd.h>

UTEST(utility, test_get_passwd_entry) {
    struct passwd pwd;
    struct passwd *pwd_ptr = get_passwd_entry(&pwd, "root");
    ASSERT_TRUE(pwd_ptr != NULL);
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
    sds dir = sdsnew(MYMPD_BUILD_DIR);
    sds file = sdsnew("bin/unit_test");
    bool rc = is_virtual_cuedir(dir, file);
    ASSERT_TRUE(rc);
    sdsfree(dir);
    sdsfree(file);

    dir = sdsnew(MYMPD_BUILD_DIR);
    file = sdsnew("bin");
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

    const char *ext2 = get_extension_from_filename("test.mp3.txt");
    ASSERT_STREQ(ext2, "txt");

    const char *wo = get_extension_from_filename("test");
    ASSERT_TRUE(wo == NULL);
}

UTEST(utility, test_basename_uri) {
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

UTEST(utility, test_strip_slash) {
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

UTEST(utility, test_strip_file_extension) {
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

UTEST(utility, test_replace_file_extension) {
    struct t_input_result testcases[] = {
        {"/test/test.mp3",   "/test/test.lrc"},
        {"/test/woext",      "/test/woext.lrc"},
        {"",                 ""},
        {"/tes/tet.mp3.mp3", "/tes/tet.mp3.lrc"},
        {NULL,               NULL}
    };
    struct t_input_result *p = testcases;
    sds test_input = sdsempty();
    while (p->input != NULL) {
        test_input = sdscatfmt(test_input, "%s", p->input);
        sds test_output = replace_file_extension(test_input, "lrc");
        ASSERT_STREQ(p->result, test_output);
        sdsclear(test_input);
        sdsfree(test_output);
        p++;
    }
    sdsfree(test_input);
}

UTEST(utility, test_sanitize_filename) {
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

UTEST(utility, test_sanitize_filename2) {
    struct t_input_result testcases[] = {
        {"http://host:80/verz/verz/test?safsaf#798234",   "http:__host:80_verz_verz_test?safsaf#798234" },
        {"",                                              "" },
        {"/test/test.mp3.mp3",                            "_test_test.mp3.mp3" },
        {NULL,                                            NULL}
    };
    struct t_input_result *p = testcases;
    sds test_input = sdsempty();
    while (p->input != NULL) {
        test_input = sdscatfmt(test_input, "%s", p->input);
        sanitize_filename2(test_input);
        ASSERT_STREQ(p->result, test_input);
        sdsclear(test_input);
        p++;
    }
    sdsfree(test_input);
}
