/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/lib/sds/sds_url.h"

#include <libgen.h>

UTEST(sds_url, test_sds_urldecode) {
    sds test_input = sdsnew("/Musict/Led%20Zeppelin/1975%20-%20Physical%20Graffiti%20%5B1994%2C%20Atlantic%2C%207567-92442-2%5D/CD%201/folder.jpg");
    sds s = sds_urldecode(sdsempty(), test_input, sdslen(test_input), false);
    ASSERT_STREQ("/Musict/Led Zeppelin/1975 - Physical Graffiti [1994, Atlantic, 7567-92442-2]/CD 1/folder.jpg", s);
    ASSERT_EQ(strlen(s), sdslen(s));
    sdsfree(test_input);
    sdsfree(s);
}

UTEST(sds_url, test_sds_urlencode) {
    sds test_input = sdsnew("/Musict/Led Zeppelin/1975 - Physical Graffiti [1994, Atlantic, 7567-92442-2]/CD 1/folder.jpg");
    sds s = sds_urlencode(sdsempty(), test_input, sdslen(test_input));
    ASSERT_STREQ("/Musict/Led%20Zeppelin/1975%20-%20Physical%20Graffiti%20%5B1994%2C%20Atlantic%2C%207567-92442-2%5D/CD%201/folder.jpg", s);
    ASSERT_EQ(strlen(s), sdslen(s));
    sdsfree(test_input);
    sdsfree(s);
}
