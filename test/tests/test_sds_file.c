/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/sds/sds_file.h"

#include <libgen.h>

const char *test_dirnames[] = {
    "/dir1/file1",
    "/dir1/file1/",
    "/dir1/dir2/file1",
    "/dir1/dir2//",
    "file1",
    "dir1/file1",
    "dir1/file1/",
    "/",
    "",
    NULL
};

UTEST(sds_file, test_sds_dirname) {
    const char **p = test_dirnames;
    while (*p != NULL) {
        printf("Testing: \"%s\"\n", *p);

        sds dir = sdsnew(*p);
        dir = sds_dirname(dir);

        char *dir_check = strdup(*p);
        char *dir_check_org = dir_check;
        dir_check = dirname(dir_check);

        ASSERT_STREQ(dir_check, dir);
        sdsfree(dir);
        free(dir_check_org);
        p++;
    }
}

UTEST(sds_file, test_sds_basename) {
    const char **p = test_dirnames;
    while (*p != NULL) {
        printf("Testing: \"%s\"\n", *p);

        sds dir = sdsnew(*p);
        dir = sds_basename(dir);

        char *dir_check = strdup(*p);
        char *dir_check_org = dir_check;
        dir_check = basename(dir_check);

        ASSERT_STREQ(dir_check, dir);
        sdsfree(dir);
        free(dir_check_org);
        p++;
    }
}

UTEST(sds_file, test_basename_uri) {
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

UTEST(sds_file, test_strip_slash) {
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

UTEST(sds_file, test_strip_file_extension) {
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

UTEST(sds_file, test_replace_file_extension) {
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
        sds test_output = sds_replace_file_extension(test_input, "lrc");
        ASSERT_STREQ(p->result, test_output);
        sdsclear(test_input);
        sdsfree(test_output);
        p++;
    }
    sdsfree(test_input);
}

UTEST(sds_file, test_sanitize_filename) {
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

UTEST(sds_file, test_sanitize_filename2) {
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
        sds_sanitize_filename2(test_input);
        ASSERT_STREQ(p->result, test_input);
        sdsclear(test_input);
        p++;
    }
    sdsfree(test_input);
}
