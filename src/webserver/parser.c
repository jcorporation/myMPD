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

#include "src/lib/log.h"
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

    const char *pos = query->buf;
    const char *end = query->buf + query->len;

    while (pos < end) {
        // Find key boundaries
        const char *key_start = pos;
        const char *key_end = memchr(pos, '=', (size_t)(end - pos));
        if (key_end == NULL) {
            // No '=' found, skip malformed parameter
            MYMPD_LOG_WARN(NULL, "Malformed query parameter: '%.*s'", (int)(end - pos), pos);
            break;
        }

        // Find value boundaries
        const char *val_start = key_end + 1;
        const char *val_end = memchr(val_start, '&', (size_t)(end - val_start));
        if (val_end == NULL) {
            // Last query parameter
            val_end = end;
        }

        // Decode key and value only if they're not empty
        size_t key_len = (size_t)(key_end - key_start);
        if (key_len > 0) {
            decoded_key = sds_urldecode(decoded_key, key_start, key_len, false);
            size_t val_len = (size_t)(val_end - val_start);
            decoded_value = sds_urldecode(decoded_value, val_start, val_len, false);
            list_push(arguments, decoded_key, 0, decoded_value, NULL);
        }
        else {
            MYMPD_LOG_WARN(NULL, "Empty key in query parameter: '%.*s'", (int)(val_end - key_start), key_start);
        }
        sdsclear(decoded_key);
        sdsclear(decoded_value);

        // Move to next parameter
        pos = val_end + 1;
    }

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
