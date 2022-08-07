/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "../../dist/utest/utest.h"
#include "../../dist/sds/sds.h"
#include "../../src/lib/http_client.h"

UTEST(http_client, test_http_client) {
    struct mg_client_request_t request = {
        .method = "GET",
        .uri = "https://raw.githubusercontent.com/jcorporation/myMPD/master/README.md",
        .extra_headers = "",
        .post_data = ""
    };

    struct mg_client_response_t response = {
        .rc = -1,
        .response = sdsempty(),
        .header = sdsempty(),
        .body = sdsempty()
    };

    http_client_request(&request, &response);
    sdsfree(response.response);
    sdsfree(response.header);
    sdsfree(response.body);
    ASSERT_EQ(0, response.rc);
}
