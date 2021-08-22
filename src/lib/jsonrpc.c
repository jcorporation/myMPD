/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "jsonrpc.h"

#include "../../dist/src/mjson/mjson.h"
#include "api.h"
#include "log.h"
#include "sds_extras.h"

#include <limits.h>
#include <string.h>

//private definitions
static bool _json_get_string(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb);


//public functions

void send_jsonrpc_notify(const char *facility, const char *severity, const char *message) {
    sds buffer = jsonrpc_notify(sdsempty(), facility, severity, message);
    ws_notify(buffer);
    sdsfree(buffer);
}

void ws_notify(sds message) {
    MYMPD_LOG_DEBUG("Push websocket notify to queue: %s", message);
    t_work_result *response = create_result_new(0, 0, INTERNAL_API_WEBSERVER_NOTIFY);
    response->data = sdsreplace(response->data, message);
    tiny_queue_push(web_server_queue, response, 0);
}

void send_jsonrpc_event(const char *event) {
    sds buffer = jsonrpc_event(sdsempty(), event);
    ws_notify(buffer);
    sdsfree(buffer);
}

sds jsonrpc_event(sds buffer, const char *event) {
    buffer = sdscrop(buffer);
    buffer = sdscat(buffer, "{\"jsonrpc\":\"2.0\",");
    buffer = tojson_char(buffer, "method", event, false);
    buffer = sdscat(buffer, "}");
    return buffer;
}

sds jsonrpc_notify(sds buffer, const char *facility, const char *severity, const char *message) {
    return jsonrpc_notify_phrase(buffer, facility, severity, message, 0);
}

sds jsonrpc_notify_phrase(sds buffer, const char *facility, const char *severity, const char *message, int count, ...) {
    buffer = jsonrpc_notify_start(buffer, "notify");
    buffer = tojson_char(buffer, "facility", facility, true);
    buffer = tojson_char(buffer, "severity", severity, true);
    buffer = tojson_char(buffer, "message", message, true);
    buffer = sdscat(buffer, "\"data\":{");
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        const char *v = va_arg(args, char *);
        if (i % 2 == 0) {
            if (i > 0) {
                buffer = sdscat(buffer, ",");
            }
            buffer = sdscatjson(buffer, v, strlen(v));
            buffer = sdscat(buffer,":");
        }
        else {
            buffer = sdscatjson(buffer, v, strlen(v));
        }
    }
    va_end(args);
    buffer = sdscat(buffer, "}}}");
    return buffer;
}

sds jsonrpc_notify_start(sds buffer, const char *method) {
    buffer = sdscrop(buffer);
    buffer = sdscat(buffer, "{\"jsonrpc\":\"2.0\",");
    buffer = tojson_char(buffer, "method", method, true);
    buffer = sdscat(buffer, "\"params\":{");
    return buffer;
}

sds jsonrpc_result_start(sds buffer, const char *method, long id) {
    buffer = sdscrop(buffer);
    buffer = sdscatprintf(buffer, "{\"jsonrpc\":\"2.0\",\"id\":%ld,\"result\":{", id);
    buffer = tojson_char(buffer, "method", method, true);
    return buffer;
}

sds jsonrpc_result_end(sds buffer) {
    return sdscat(buffer, "}}");
}

sds jsonrpc_respond_ok(sds buffer, const char *method, long id, const char *facility) {
    return jsonrpc_respond_message(buffer, method, id, false, facility, "info", "ok");
}

sds jsonrpc_respond_message(sds buffer, const char *method, long id, bool error, 
                            const char *facility, const char *severity, const char *message)
{
    return jsonrpc_respond_message_phrase(buffer, method, id, error, facility, severity, message, 0);
}

sds jsonrpc_respond_message_phrase(sds buffer, const char *method, long id, bool error, 
                            const char *facility, const char *severity, const char *message, int count, ...)
{
    buffer = sdscrop(buffer);
    buffer = sdscatprintf(buffer, "{\"jsonrpc\":\"2.0\",\"id\":%ld,\"%s\":{", 
        id, (error == true ? "error" : "result"));
    buffer = tojson_char(buffer, "method", method, true);
    buffer = tojson_char(buffer, "facility", facility, true);
    buffer = tojson_char(buffer, "severity", severity, true);
    buffer = tojson_char(buffer, "message", message, true);
    buffer = sdscat(buffer, "\"data\":{");
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        const char *v = va_arg(args, char *);
        if (i % 2 == 0) {
            if (i > 0) {
                buffer = sdscat(buffer, ",");
            }
            buffer = sdscatjson(buffer, v, strlen(v));
            buffer = sdscat(buffer,":");
        }
        else {
            buffer = sdscatjson(buffer, v, strlen(v));
        }
    }
    va_end(args);
    buffer = sdscat(buffer, "}}}");
    return buffer;
}

sds tojson_char(sds buffer, const char *key, const char *value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":", key);
    if (value != NULL) {
        buffer = sdscatjson(buffer, value, strlen(value)); /* Flawfinder: ignore */
    }
    else {
        buffer = sdscat(buffer, "\"\"");
    }
    if (comma) {
        buffer = sdscat(buffer, ",");
    }
    return buffer;
}

sds tojson_char_len(sds buffer, const char *key, const char *value, size_t len, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":", key);
    if (value != NULL) {
        buffer = sdscatjson(buffer, value, len);
    }
    else {
        buffer = sdscat(buffer, "\"\"");
    }
    if (comma) {
        buffer = sdscat(buffer, ",");
    }
    return buffer;
}

sds tojson_bool(sds buffer, const char *key, bool value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%s", key, value == true ? "true" : "false");
    if (comma) {
        buffer = sdscat(buffer, ",");
    }
    return buffer;
}

sds tojson_long(sds buffer, const char *key, long long value, bool comma) {
    buffer = sdscatprintf(buffer, "\"%s\":%lld", key, value);
    if (comma) {
        buffer = sdscat(buffer, ",");
    }
    return buffer;
}

sds tojson_ulong(sds buffer, const char *key, unsigned long value, bool comma) {
    buffer = sdscatprintf(buffer, "\"%s\":%lu", key, value);
    if (comma) {
        buffer = sdscat(buffer, ",");
    }
    return buffer;
}

sds tojson_double(sds buffer, const char *key, double value, bool comma) {
    buffer = sdscatprintf(buffer, "\"%s\":%f", key, value);
    if (comma) {
        buffer = sdscat(buffer, ",");
    }
    return buffer;
}

bool json_get_bool(sds s, const char *path, bool *result) {
    int v;
    if (mjson_get_bool(s, (int)sdslen(s), path, &v) != 0) {
        *result = v == 1 ? true : false;
        return true;
    }
    MYMPD_LOG_WARN("Path \"%s\" not found", path);
    return false;
}

bool json_get_int_max(sds s, const char *path, int *result) {
    return json_get_int(s, path, INT_MIN, INT_MAX, result);
}

bool json_get_int(sds s, const char *path, int min, int max, int *result) {
    double value;
    if (mjson_get_number(s, (int)sdslen(s), path, &value) != 0) {
        if (value >= min && value <= max) {
            *result = (int)value;
            return true;
        }
        MYMPD_LOG_WARN("Number out of range for path \"%s\"", path);
    }
    else {
        MYMPD_LOG_WARN("Path \"%s\" not found", path);
    }
    return false;
}

bool json_get_uint_max(sds s, const char *path, unsigned *result) {
    return json_get_uint(s, path, 0, UINT_MAX, result);
}

bool json_get_uint(sds s, const char *path, unsigned min, unsigned max, unsigned *result) {
    double value;
    if (mjson_get_number(s, (int)sdslen(s), path, &value) != 0) {
        if (value >= min && value <= max) {
            *result = (unsigned)value;
            return true;
        }
        MYMPD_LOG_WARN("Number out of range for path \"%s\"", path);
    }
    else {
        MYMPD_LOG_WARN("Path \"%s\" not found", path);
    }
    return false;
}

bool json_get_string_max(sds s, const char *path, sds *result, validate_callback vcb) {
    if (vcb == NULL) {
        MYMPD_LOG_ERROR("Validation callback is NULL");
        return false;
    }
    return _json_get_string(s, path, 0, 200, result, vcb);
}

bool json_get_string_cmp(sds s, const char *path, size_t min, size_t max, const char *cmp, sds *result) {
    if (_json_get_string(s, path, min, max, result, NULL) == false ||
        strcmp(*result, cmp) != 0) 
    {
        sdsclear(*result);
        MYMPD_LOG_WARN("Validation of path \"%s\" has failed", path);
        return false;
    }
    return true;
}

bool json_get_string(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb) {
    if (vcb == NULL) {
        MYMPD_LOG_ERROR("Validation callback is NULL");
        return false;
    }
    return _json_get_string(s, path, min, max, result, vcb);
}

static bool _json_get_string(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb) {
    const char *p;
    int n;
    if (mjson_find(s, (int)sdslen(s), path, &p, &n) != MJSON_TOK_STRING) {
        *result = NULL;
        MYMPD_LOG_WARN("String for path \"%s\" not found", path);
        return false;
    }
    *result = sdsempty();
    if (n <= 2) {
        //empty string
        if (min == 0) {
            return true;
        }
        else {
            MYMPD_LOG_WARN("String for path \"%s\" is too short", path);
            return false;
        }
    }
    
    //remove quotes
    n = n - 2;
    p++;

    if ((sds_json_unescape(p, n, result) == false) ||
        (sdslen(*result) < min && sdslen(*result) > max))
    {
        MYMPD_LOG_WARN("String for path \"%s\" is out of min or max", path);
        sdsclear(*result);
        return false;
    }

    if (vcb != NULL) {
        if (vcb(*result) == false) {
            MYMPD_LOG_WARN("Validation of path \"%s\" has failed", path);
            sdsclear(*result);
            return false;
        }
    }

    return true;
}

bool json_iterate_object(sds s, const char *path, iterate_callback icb, void *icb_userdata, validate_callback vcb, int max_elements) {
    if (icb == NULL) {
        MYMPD_LOG_ERROR("Iteration callback is NULL");
        return false;
    }
    if (vcb == NULL) {
        MYMPD_LOG_ERROR("Validation callback is NULL");
        return false;
    }
    const char *p;
    int n;
    int otype = mjson_find(s, (int)sdslen(s), path, &p, &n);
    if ( otype != MJSON_TOK_OBJECT && otype != MJSON_TOK_ARRAY) {
        MYMPD_LOG_WARN("Invalid json object type");
        return false;
    }
    if (n == 2) {
        //empty object
        return true;
    }

    sds value = sdsempty();
    sds key = sdsempty();
    int i = 0;
    int koff = 0;
    int klen = 0;
    int voff = 0;
    int vlen = 0;
    int vtype = 0;
    int off = 0;
    for (off = 0; (off = mjson_next(p, n, off, &koff, &klen, &voff, &vlen, &vtype)) != 0;) {
        if (klen > 2) {
            if (sds_json_unescape(p + koff + 1, klen - 2, &key) == false ||
                vcb_isalnum(value) == false)
            {
                MYMPD_LOG_WARN("Validation of path \"%s\" has failed", path);
                sdsfree(value);
                sdsfree(key);
                return false;
            }
        }
        switch(vtype) {
            case MJSON_TOK_STRING:
                if (vlen > 2) {
                    if (sds_json_unescape(p + voff + 1, vlen - 2, &value) == false) {
                        sdsfree(value);
                        sdsfree(key);
                        MYMPD_LOG_WARN("Validation of path \"%s\" has failed", path);
                        return false;
                    }
                }
                break;
            case MJSON_TOK_FALSE:
            case MJSON_TOK_TRUE:
            case MJSON_TOK_NUMBER:
                break;
            default:
                MYMPD_LOG_WARN("Invalid json value type");
                sdsfree(value);
                sdsfree(key);
                return false;
        }

        if (icb(key, value, vtype, vcb, icb_userdata) == false) {
            MYMPD_LOG_WARN("Iteration cb for path \"%s\" has failed", path);
            sdsfree(value);
            sdsfree(key);
            return false;
        }

        sdsclear(value);
        sdsclear(key);
        i++;

        if (i == max_elements) {
            break;
        }
    }
    sdsfree(value);
    sdsfree(key);
    return true;
}

static bool icb_json_get_array_string(sds key, sds value, int vtype, validate_callback vcb, void *userdata) {
    (void) key;
    if (vtype != MJSON_TOK_STRING ||
        vcb(value) == false)
    {
        MYMPD_LOG_WARN("Validation of value \"%s\" has failed", value);
        return false;
    }
    struct list *l = (struct list *)userdata;
    list_push(l, value, 0, NULL, NULL);
    return true;
}

bool json_get_array_string(sds s, const char *path, struct list *l, validate_callback vcb, int max_elements) {
    return json_iterate_object(s, path, icb_json_get_array_string, l, vcb, max_elements);
}

static bool icb_json_get_object_string(sds key, sds value, int vtype, validate_callback vcb, void *userdata) {
    if (sdslen(key) == 0 ||
        vtype != MJSON_TOK_STRING ||
        vcb(value) == false)
    {
        MYMPD_LOG_WARN("Validation of value \"%s\" has failed", value);
        return false;
    }
    struct list *l = (struct list *)userdata;
    list_push(l, key, 0, value, NULL);
    return true;
}

bool json_get_object_string(sds s, const char *path, struct list *l, validate_callback vcb, int max_elements) {
    return json_iterate_object(s, path, icb_json_get_object_string, l, vcb, max_elements);
}

sds list_to_json_array(sds s, struct list *l) {
    struct list_node *current = l->head;
    int i = 0;
    while (current != NULL) {
        if (i++) {
            s = sdscatlen(s, ",", 1);
        }
        s = sdscatjson(s, current->key, sdslen(current->key));
        current = current->next;
    }
    return s;
}
