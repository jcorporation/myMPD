/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/src/utest/utest.h"
#include "../../src/lib/jsonrpc.h"

UTEST(jsonrpc, test_json_get_bool) {
    bool result;
    //valid
    sds data = sdsnew("{\"key1\": true}");
    ASSERT_TRUE(json_get_bool(data, "$.key1", &result, NULL));
    sdsclear(data);
    //invalid
    data = sdscat(data, "{\"key1\": \"true\"}");
    ASSERT_FALSE(json_get_bool(data, "", &result, NULL));
    sdsfree(data);
}
