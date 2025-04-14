/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/sds/sds.h"
#include "dist/utest/utest.h"
#include "src/lib/http_client.h"
#include "src/lib/http_client_cache.h"
#include "src/lib/list.h"

#include <assert.h>

UTEST(http_client, test_http_client_cache) {
    init_testenv();

    struct t_config config;
    config.cache_http_keep_days = 31;
    config.cachedir = sdsnew("/tmp/mympd-test");
    const char *uri = "https://github.com/jcorporation";

    struct mg_client_response_t rw;
    http_client_response_init(&rw);
    rw.response_code = 200;
    list_push(&rw.header, "Test-Header1", 0, "Test-Value1", NULL);
    list_push(&rw.header, "Test-Header2", 0, "Test-Value2", NULL);
    rw.body = sdscat(rw.body, "Testbody");

    http_client_cache_write(&config, uri, &rw);
    struct mg_client_response_t *cached = http_client_cache_check(&config, uri);
    assert(cached);
    ASSERT_EQ(rw.response_code, cached->response_code);
    ASSERT_EQ(rw.header.length, cached->header.length);
    ASSERT_STREQ(rw.body, cached->body);

    struct t_list_node *rw_header1 = list_shift_first(&rw.header);
    struct t_list_node *cached_header1 = list_shift_first(&cached->header);
    ASSERT_STREQ(rw_header1->key, cached_header1->key);
    ASSERT_STREQ(rw_header1->value_p, cached_header1->value_p);
    list_node_free(rw_header1);
    list_node_free(cached_header1);

    struct t_list_node *rw_header2 = list_shift_first(&rw.header);
    struct t_list_node *cached_header2 = list_shift_first(&cached->header);
    ASSERT_STREQ(rw_header2->key, cached_header2->key);
    ASSERT_STREQ(rw_header2->value_p, cached_header2->value_p);
    list_node_free(rw_header2);
    list_node_free(cached_header2);

    http_client_response_clear(&rw);
    http_client_response_clear(cached);
    free(cached);
    sdsfree(config.cachedir);
    clean_testenv();
}
