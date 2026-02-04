/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/utility.h"

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
