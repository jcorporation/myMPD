/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver url parser functions
 */

#include "compile_time.h"
#include "src/webserver/parser.h"

#include "src/lib/sds/sds_extras.h"
#include "src/lib/sds/sds_url.h"

/**
 * Public functions
 */

/**
 * Extract arguments from query parameters.
 * Keys and values are decodeded.
 * @param query Query string to parse
 * @return Parsed arguments as t_list struct.
 */
struct t_list *webserver_parse_arguments(struct mg_str *query) {
    assert(query != NULL);
    struct t_list *arguments = list_new();
    if (query->len == 0) {
        return arguments;
    }
    sds decoded_key = sdsempty();
    sds decoded_value = sdsempty();
    int params_count;
    sds *params = sdssplitlen(query->buf, (ssize_t)query->len, "&", 1, &params_count);
    for (int i = 0; i < params_count; i++) {
        int kv_count;
        sds *kv = sdssplitlen(params[i], (ssize_t)sdslen(params[i]), "=", 1, &kv_count);
        if (kv_count == 2) {
            decoded_key = sds_urldecode(decoded_key, kv[0], sdslen(kv[0]), false);
            decoded_value = sds_urldecode(decoded_value, kv[1], sdslen(kv[1]), false);
            list_push(arguments, decoded_key, 0, decoded_value, NULL);
            sdsclear(decoded_key);
            sdsclear(decoded_value);
        }
        sdsfreesplitres(kv, kv_count);
    }
    sdsfreesplitres(params, params_count);
    FREE_SDS(decoded_key);
    FREE_SDS(decoded_value);
    return arguments;
}

/**
 * Gets and decodes an url parameter.
 * The key will not be url decoded before matching.
 * @param query Query string to parse
 * @param name Name to get, you must append "=" to the name
 * @return URL decoded value or NULL on error
 */
sds get_uri_param(struct mg_str *query, const char *name) {
    size_t name_len = strlen(name);
    assert(query != NULL);
    assert(name[name_len - 1] == '=');
    if (query->len < name_len) {
        return NULL;
    }
    if (query->len == name_len) {
        if (memcmp(query->buf, name, name_len) == 0) {
            return sdsempty();
        }
        return NULL;
    }
    for (size_t i = 0, j = query->len - name_len; i <= j; i++) {
        if (strncmp(query->buf + i, name, name_len) == 0) {
            size_t value_start = i + name_len;
            size_t value_end = value_start;
            while (value_end < query->len && query->buf[value_end] != '&') {
                value_end++;
            }
            return sds_urldecode(sdsempty(), query->buf + value_start, value_end - value_start, false);
            break;
        }
    }
    return NULL;
}
