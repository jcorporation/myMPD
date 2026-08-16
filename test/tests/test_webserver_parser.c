/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "utility.h"

#include "dist/utest/utest.h"
#include "src/webserver/parser.h"

UTEST(webserver_parser, test_get_uri_param) {
    struct mg_str query = mg_str("param1=value1&param2=value2&param3=value3");

    // First param
    sds value1 = get_uri_param(&query, "param1=");
    ASSERT_STREQ(value1, "value1");
    sdsfree(value1);

    // Middle param
    sds value2 = get_uri_param(&query, "param2=");
    ASSERT_STREQ(value2, "value2");
    sdsfree(value2);

    // Last param
    sds value3 = get_uri_param(&query, "param3=");
    ASSERT_STREQ(value3, "value3");
    sdsfree(value3);

    // Param not found
    sds value4 = get_uri_param(&query, "param4=");
    ASSERT_TRUE(value4 == NULL);
}

UTEST(webserver_parser, test_get_uri_param_empty) {
    struct mg_str query = mg_str("param1=&param2=&param3=");

    // First param
    sds value1 = get_uri_param(&query, "param1=");
    ASSERT_STREQ(value1, "");
    sdsfree(value1);

    // Middle param
    sds value2 = get_uri_param(&query, "param2=");
    ASSERT_STREQ(value2, "");
    sdsfree(value2);

    // Last param
    sds value3 = get_uri_param(&query, "param3=");
    ASSERT_STREQ(value3, "");
    sdsfree(value3);

    // Param not found
    sds value4 = get_uri_param(&query, "param4=");
    ASSERT_TRUE(value4 == NULL);
}

UTEST(webserver_parser, test_get_uri_param_empty_query) {
    struct mg_str query = mg_str("");

    // Param not found
    sds value4 = get_uri_param(&query, "param4=");
    ASSERT_TRUE(value4 == NULL);
}

UTEST(webserver_parser, test_get_uri_param_invalid_query) {
    struct mg_str query = mg_str("adsfwerew");

    // Param not found
    sds value4 = get_uri_param(&query, "param4=");
    ASSERT_TRUE(value4 == NULL);
}

UTEST(webserver_parser, test_get_uri_param_special_cases) {
    struct mg_str query = mg_str("param4=");

    sds value4 = get_uri_param(&query, "param4=");
    ASSERT_STREQ(value4, "");
    sdsfree(value4);

    value4 = get_uri_param(&query, "param5=");
    ASSERT_TRUE(value4 == NULL);
}

UTEST(webserver_parser, test_webserver_parse_arguments) {
    struct mg_str query = mg_str("param1=value1&param2=value2&param3=value3");
    struct t_list *args = webserver_parse_arguments(&query);

    ASSERT_EQ(args->length, 3U);

    // First param
    struct t_list_node *node = list_shift_first(args);
    ASSERT_STREQ(node->key, "param1");
    ASSERT_STREQ(node->value_p, "value1");
    list_node_free(node);

    // Middle param
    node = list_shift_first(args);
    ASSERT_STREQ(node->key, "param2");
    ASSERT_STREQ(node->value_p, "value2");
    list_node_free(node);

    // Last param
    node = list_shift_first(args);
    ASSERT_STREQ(node->key, "param3");
    ASSERT_STREQ(node->value_p, "value3");
    list_node_free(node);

    list_free(args);
}

UTEST(webserver_parser, test_webserver_parse_arguments_empty) {
    struct mg_str query = mg_str("param1=&param2=&param3=");
    struct t_list *args = webserver_parse_arguments(&query);

    ASSERT_EQ(args->length, 3U);

    // First param
    struct t_list_node *node = list_shift_first(args);
    ASSERT_STREQ(node->key, "param1");
    ASSERT_STREQ(node->value_p, "");
    list_node_free(node);

    // Middle param
    node = list_shift_first(args);
    ASSERT_STREQ(node->key, "param2");
    ASSERT_STREQ(node->value_p, "");
    list_node_free(node);

    // Last param
    node = list_shift_first(args);
    ASSERT_STREQ(node->key, "param3");
    ASSERT_STREQ(node->value_p, "");
    list_node_free(node);

    list_free(args);
}

UTEST(webserver_parser, test_webserver_parse_arguments_empty_query) {
    struct mg_str query = mg_str("");
    struct t_list *args = webserver_parse_arguments(&query);

    ASSERT_EQ(args->length, 0U);
    list_free(args);
}
