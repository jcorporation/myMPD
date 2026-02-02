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
