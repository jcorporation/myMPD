/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/filehandler.h"

UTEST(filehandler, test_cleanup_rm_directory) {
    init_testenv();

    int rc = testdir("/tmp/mympd-test/tmp2", "/tmp/mympd-test/tmp2", true, false);
    ASSERT_EQ(rc, DIR_CREATED);
    bool rc2 = clean_rm_directory("/tmp/mympd-test/tmp2");
    ASSERT_TRUE(rc2);

    clean_testenv();
}

UTEST(filehandler, test_testdir) {
    init_testenv();

    int rc = testdir("workdir", workdir, false, false);
    ASSERT_EQ(rc, DIR_EXISTS);

    rc = testdir("/tmp/mympd-test/tmp2", "/tmp/mympd-test/tmp2", false, false);
    ASSERT_EQ(rc, DIR_NOT_EXISTS);

    rc = testdir("/tmp/mympd-test/tmp2", "/tmp/mympd-test/tmp2", true, false);
    ASSERT_EQ(rc, DIR_CREATED);
    rmdir("/tmp/mympd-test/tmp2");

    rc = testdir("/tmp/mympd-test/tmp2/tmp2", "/tmp/mympd-test/tmp2/tmp2", true, false);
    ASSERT_EQ(rc, DIR_CREATE_FAILED);

    clean_testenv();
}

UTEST(filehandler, test_write_data_to_file) {
    init_testenv();

    bool rc = create_testfile();
    ASSERT_TRUE(rc);

    clean_testenv();
}

UTEST(filehandler, test_testfile_read) {
    init_testenv();

    create_testfile();

    sds file  = sdsnew("/tmp/mympd-test/state/test");
    bool rc = testfile_read(file);
    ASSERT_TRUE(rc);
    sdsclear(file);

    file = sdscat(file, "/tmp/mympd-test/state/test-notexist");
    rc = testfile_read(file);
    ASSERT_FALSE(rc);
    sdsfree(file);

    clean_testenv();
}

UTEST(filehandler, test_sds_getfile_from_fp) {
    init_testenv();

    create_testfile();

    FILE *fp = fopen("/tmp/mympd-test/state/test", "r");
    int nread = 0;
    sds line = sds_getfile_from_fp(sdsempty(), fp, 1000, false, &nread);
    fclose(fp);
    ASSERT_EQ(nread, TESTFILE_CONTENT_LEN);
    ASSERT_STREQ(TESTFILE_CONTENT, line);

    //remove newline
    fp = fopen("/tmp/mympd-test/state/test", "r");
    nread = 0;
    line = sds_getfile_from_fp(line, fp, 1000, true, &nread);
    fclose(fp);
    ASSERT_EQ(nread, TESTFILE_CONTENT_NW_LEN);
    ASSERT_STREQ(TESTFILE_CONTENT_NW, line);

    // too big
    fp = fopen("/tmp/mympd-test/state/test", "r");
    nread = 0;
    line = sds_getfile_from_fp(line, fp, 5, true, &nread);
    fclose(fp);
    ASSERT_EQ(nread, -2);

    sdsfree(line);
    clean_testenv();
}

UTEST(filehandler, test_sds_getfile) {
    init_testenv();

    create_testfile();

    int nread = 0;
    sds line = sds_getfile(sdsempty(), "/tmp/mympd-test/state/test", 1000, false, true, &nread);
    ASSERT_GE(nread, 0);
    ASSERT_STREQ(line, "asdfjlkasdfjklsafd\nasfdsdfawaerwer");

    // too big
    nread = 0;
    line = sds_getfile(line, "/tmp/mympd-test/state/test", 5, true, true, &nread);
    ASSERT_EQ(nread, -2);
    sdsfree(line);

    clean_testenv();
}

UTEST(filehandler, test_sds_getline) {
    init_testenv();

    create_testfile();

    FILE *fp = fopen("/tmp/mympd-test/state/test", "re");
    int nread = 0;
    sds line = sds_getline(sdsempty(), fp, 1000, &nread);
    fclose(fp);
    ASSERT_GT(nread, 0);
    ASSERT_STREQ(line, "asdfjlkasdfjklsafd");

    // read 5 chars
    fp = fopen("/tmp/mympd-test/state/test", "re");
    nread = 0;
    line = sds_getline(line, fp, 5, &nread);
    fclose(fp);
    ASSERT_EQ(nread, 5);
    sdsfree(line);

    clean_testenv();
}
