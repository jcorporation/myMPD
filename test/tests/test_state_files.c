/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/filehandler.h"
#include "src/lib/sds_extras.h"
#include "src/lib/state_files.h"

#include <sys/stat.h>

sds get_file_content(void) {
    sds line = sdsempty();
    sds_getfile(&line, "/tmp/mympd-test/state/test", 1000, true, true);
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
    init_testenv();

    sds value = sdsnew("blub");
    value = state_file_rw_string_sds(workdir, "state", "test", value, vcb_isalnum, true);
    ASSERT_STREQ("blub", value);
    sds content = get_file_content();
    ASSERT_STREQ(content, value);
    sdsfree(value);
    sdsfree(content);

    clean_testenv();
}

UTEST(state_files, test_state_file_rw_string) {
    init_testenv();

    sds value = state_file_rw_string(workdir, "state", "test", "blub", vcb_isalnum, true);
    ASSERT_STREQ("blub", value);
    sds content = get_file_content();
    ASSERT_STREQ(value, content);
    sdsfree(content);
    sdsfree(value);

    clean_testenv();
}

UTEST(state_files, test_state_file_rw_bool) {
    init_testenv();

    bool value = state_file_rw_bool(workdir, "state", "test", true, true);
    ASSERT_TRUE(value);
    sds content = get_file_content();
    ASSERT_STREQ("true", content);
    sdsfree(content);

    clean_testenv();
}

UTEST(state_files, test_state_file_rw_int) {
    init_testenv();

    int value = state_file_rw_int(workdir, "state", "test", 10, 1, 20, true);
    ASSERT_EQ(10, value);
    sds content = get_file_content();
    ASSERT_STREQ("10", content);
    sdsfree(content);

    clean_testenv();
}

UTEST(state_files, test_state_file_rw_uint) {
    init_testenv();

    unsigned value = state_file_rw_uint(workdir, "state", "test", 10, 1, 20, true);
    ASSERT_EQ((unsigned)10, value);
    sds content = get_file_content();
    ASSERT_STREQ("10", content);
    sdsfree(content);

    clean_testenv();
}

UTEST(state_files, test_state_file_rw_tag) {
    init_testenv();

    enum mpd_tag_type tag = state_file_rw_tag(workdir, "state", "test", MPD_TAG_ALBUM, true);
    ASSERT_EQ(MPD_TAG_ALBUM, tag);
    sds content = get_file_content();
    ASSERT_STREQ("Album", content);
    sdsfree(content);

    clean_testenv();
}

UTEST(state_files, test_state_file_write) {
    init_testenv();

    bool rc = state_file_write(workdir, "state", "test", "blub");
    ASSERT_TRUE(rc);
    sds content = get_file_content();
    ASSERT_STREQ("blub", content);
    sdsfree(content);

    clean_testenv();
}
