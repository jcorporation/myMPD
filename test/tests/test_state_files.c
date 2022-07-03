/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/utest/utest.h"
#include "../../src/lib/sds_extras.h"
#include "../../src/lib/state_files.h"

#include <sys/stat.h>

void mk_workdir(void) {
    mkdir("/tmp/test_workdir", 0770);
}

void rm_workdir(void) {
    unlink("/tmp/test_workdir/test");
    rmdir("/tmp/test_workdir");
}

sds get_file_content(void) {
    FILE *fp = fopen("/tmp/test_workdir/test", "r");
    sds line = sdsempty();
    sds_getline(&line, fp, 1000);
    fclose(fp);
    return line;
}

UTEST(state_files, test_camel_to_snake) {
    sds camel = sdsnew("camelCaseName");
    sds snake = camel_to_snake(camel);
    ASSERT_STREQ("camel_case_name", snake);
    sdsfree(snake);
    sdsfree(camel);
}

UTEST(state_files, test_state_file_rw_string_sds) {
    mk_workdir();
    sds v = sdsnew("blub");
    sds w = sdsnew("/tmp");
    v = state_file_rw_string_sds(w, "test_workdir", "test", v, vcb_isalnum, false);
    ASSERT_STREQ("blub", v);
    sds c = get_file_content();
    ASSERT_STREQ(c, v);
    rm_workdir();
    sdsfree(v);
    sdsfree(c);
    sdsfree(w);
}

UTEST(state_files, test_state_file_rw_string) {
    mk_workdir();
    sds w = sdsnew("/tmp");
    sds v = state_file_rw_string(w, "test_workdir", "test", "blub", vcb_isalnum, false);
    ASSERT_STREQ("blub", v);
    sds c = get_file_content();
    ASSERT_STREQ(v, c);
    rm_workdir();
    sdsfree(c);
    sdsfree(v);
    sdsfree(w);
}

UTEST(state_files, test_state_file_rw_bool) {
    mk_workdir();
    sds w = sdsnew("/tmp");
    bool v = state_file_rw_bool(w, "test_workdir", "test", true, false);
    ASSERT_TRUE(v);
    sds c = get_file_content();
    ASSERT_STREQ("true", c);
    rm_workdir();
    sdsfree(c);
    sdsfree(w);
}

UTEST(state_files, test_state_file_rw_int) {
    mk_workdir();
    sds w = sdsnew("/tmp");
    int v = state_file_rw_int(w, "test_workdir", "test", 10, 1, 20, false);
    ASSERT_EQ(10, v);
    sds c = get_file_content();
    ASSERT_STREQ("10", c);
    rm_workdir();
    sdsfree(c);
    sdsfree(w);
}

UTEST(state_files, test_state_file_rw_long) {
    mk_workdir();
    sds w = sdsnew("/tmp");
    long v = state_file_rw_long(w, "test_workdir", "test", 10, 1, 20, false);
    ASSERT_EQ(10, v);
    sds c = get_file_content();
    ASSERT_STREQ("10", c);
    rm_workdir();
    sdsfree(c);
    sdsfree(w);
}

UTEST(state_files, test_state_file_rw_uint) {
    mk_workdir();
    sds w = sdsnew("/tmp");
    unsigned v = state_file_rw_uint(w, "test_workdir", "test", 10, 1, 20, false);
    ASSERT_EQ((unsigned)10, v);
    sds c = get_file_content();
    ASSERT_STREQ("10", c);
    rm_workdir();
    sdsfree(c);
    sdsfree(w);
}

UTEST(state_files, test_state_file_write) {
    mk_workdir();
    sds w = sdsnew("/tmp");
    bool rc = state_file_write(w, "test_workdir", "test", "blub");
    ASSERT_TRUE(rc);
    sds c = get_file_content();
    ASSERT_STREQ("blub", c);
    rm_workdir();
    sdsfree(c);
    sdsfree(w);
}
