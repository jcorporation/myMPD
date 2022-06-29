/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/utest/utest.h"
#include "../../src/lib/sds_extras.h"
#include "../../src/lib/utility.h"

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

UTEST(utility, test_testdir) {
    int rc = testdir("tmp", "/tmp", false);
    ASSERT_EQ(rc, DIR_EXISTS);

    rc = testdir("tmp2", "/tmp2", false);
    ASSERT_EQ(rc, DIR_NOT_EXISTS);

    rc = testdir("tmp/tmp2", "/tmp/tmp2", true);
    ASSERT_EQ(rc, DIR_CREATED);
    rmdir("/tmp/tmp2");

    rc = testdir("tmp/tmp2", "/tmp/tmp2/tmp2", true);
    ASSERT_EQ(rc, DIR_CREATE_FAILED);
}

UTEST(utility, test_my_msleep) {
    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    my_msleep(300);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    float secs = (end.tv_nsec - begin.tv_nsec) / 1000000000.0 + (end.tv_sec  - begin.tv_sec);
    int msecs = secs * 1000;
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

UTEST(utility, test_write_data_to_file) {
    sds file = sdsnew("/tmp/test");
    const char *data ="asdfjlkasdfjklsafd\nasfdsdfawaerwer\n";
    size_t len = strlen(data);
    bool rc = write_data_to_file(file, data, len);
    ASSERT_TRUE(rc);
    sdsfree(file);
}

UTEST(sds_extras, test_sds_getfile) {
    sds line = sdsempty();
    FILE *fp = fopen("/tmp/test", "r");
    int rc = sds_getfile(&line, fp, 1000);
    fclose(fp);
    ASSERT_EQ(rc, 0);
    ASSERT_STREQ(line, "asdfjlkasdfjklsafd\nasfdsdfawaerwer");

    fp = fopen("/tmp/test", "r");
    rc = sds_getfile(&line, fp, 5);
    fclose(fp);
    ASSERT_EQ(rc, -2);
    sdsfree(line);
}

UTEST(sds_extras, test_sds_getline) {
    sds line = sdsempty();
    FILE *fp = fopen("/tmp/test", "r");
    int rc = sds_getline(&line, fp, 1000);
    fclose(fp);
    ASSERT_EQ(rc, 0);
    ASSERT_STREQ(line, "asdfjlkasdfjklsafd");

    fp = fopen("/tmp/test", "r");
    rc = sds_getline(&line, fp, 5);
    fclose(fp);
    ASSERT_EQ(rc, -2);
    sdsfree(line);
}

UTEST(sds_extras, test_sds_getline_n) {
    sds line = sdsempty();
    FILE *fp = fopen("/tmp/test", "r");
    int rc = sds_getline_n(&line, fp, 1000);
    fclose(fp);
    ASSERT_EQ(rc, 0);
    ASSERT_STREQ(line, "asdfjlkasdfjklsafd\n");

    unlink("/tmp/test");
    sdsfree(line);
}

UTEST(utility, test_get_extension_from_filename) {
    const char *ext = get_extension_from_filename("test.txt");
    ASSERT_STREQ(ext, "txt");

    const char *wo = get_extension_from_filename("test");
    ASSERT_TRUE(wo == NULL);
}
