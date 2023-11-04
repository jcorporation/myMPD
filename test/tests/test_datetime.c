/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/datetime.h"

UTEST(env, test_getenv_int) {
    const char *str1 = "2023-11-05";
    time_t ts = parse_date(str1);
    ASSERT_EQ(ts, 1699138800);
}
