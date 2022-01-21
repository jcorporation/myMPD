/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/utest/utest.h"
#include "../../src/lib/utility.h"

UTEST(utility, test_my_msleep) {
    struct timespec begin, end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    my_msleep(300);
    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    float secs = (end.tv_nsec - begin.tv_nsec) / 1000000000.0 + (end.tv_sec  - begin.tv_sec);
    int msecs = secs * 1000;
    ASSERT_EQ(300, msecs);
}
