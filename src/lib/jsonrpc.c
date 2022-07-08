/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "jsonrpc.h"

#include "../../dist/mjson/mjson.h"
#include "api.h"
#include "log.h"
#include "sds_extras.h"
#include "utility.h"

#include <limits.h>
#include <string.h>

//private definitions
static bool _icb_json_get_tag(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error);
static bool _json_get_string(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb, sds *error);
static void _set_parse_error(sds *error, const char *fmt, ...);

//public functions

void send_jsonrpc_notify(const char *facility, const char *severity, const char *message) {
    sds buffer = jsonrpc_notify(sdsempty(), facility, severity, message);
    ws_notify(buffer);
    FREE_SDS(buffer);
}

void send_jsonrpc_event(const char *event) {
    sds buffer = jsonrpc_event(sdsempty(), event);
    ws_notify(buffer);
    FREE_SDS(buffer);
}

sds jsonrpc_event(sds buffer, const char *event) {
    sdsclear(buffer);
    buffer = sdscat(buffer, "{\"jsonrpc\":\"2.0\",");
    buffer = tojson_char(buffer, "method", event, false);
    buffer = sdscatlen(buffer, "}", 1);
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
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sds_catjson(buffer, v, strlen(v));
            buffer = sdscatlen(buffer,":", 1);
        }
        else {
            buffer = sds_catjson(buffer, v, strlen(v));
        }
    }
    va_end(args);
    buffer = sdscatlen(buffer, "}}}", 3);
    return buffer;
}

sds jsonrpc_notify_start(sds buffer, const char *method) {
    sdsclear(buffer);
    buffer = sdscat(buffer, "{\"jsonrpc\":\"2.0\",");
    buffer = tojson_char(buffer, "method", method, true);
    buffer = sdscat(buffer, "\"params\":{");
    return buffer;
}

sds jsonrpc_result_start(sds buffer, const char *method, long id) {
    sdsclear(buffer);
    buffer = sdscatfmt(buffer, "{\"jsonrpc\":\"2.0\",\"id\":%l,\"result\":{", id);
    buffer = tojson_char(buffer, "method", method, true);
    return buffer;
}

sds jsonrpc_result_end(sds buffer) {
    return sdscatlen(buffer, "}}", 2);
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
    sdsclear(buffer);
    buffer = sdscatfmt(buffer, "{\"jsonrpc\":\"2.0\",\"id\":%l,\"%s\":{",
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
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sds_catjson(buffer, v, strlen(v));
            buffer = sdscatlen(buffer, ":", 1);
        }
        else {
            buffer = sds_catjson(buffer, v, strlen(v));
        }
    }
    va_end(args);
    buffer = sdscatlen(buffer, "}}}", 3);
    return buffer;
}

sds tojson_raw(sds buffer, const char *key, const char *value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%s", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

sds tojson_char(sds buffer, const char *key, const char *value, bool comma) {
    //treat NULL values as empty
    size_t len = value != NULL ? strlen(value) : 0;
    return tojson_char_len(buffer, key, value, len, comma);
}

sds tojson_sds(sds buffer, const char *key, sds value, bool comma) {
    return tojson_char_len(buffer, key, value, sdslen(value), comma);
}

sds tojson_char_len(sds buffer, const char *key, const char *value, size_t len, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":", key);
    if (value != NULL) {
        buffer = sds_catjson(buffer, value, len);
    }
    else {
        buffer = sdscatlen(buffer, "\"\"", 2);
    }
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

sds tojson_bool(sds buffer, const char *key, bool value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%s", key, value == true ? "true" : "false");
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

sds tojson_int(sds buffer, const char *key, int value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%i", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

sds tojson_uint(sds buffer, const char *key, unsigned value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%u", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

sds tojson_long(sds buffer, const char *key, long value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%l", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

sds tojson_llong(sds buffer, const char *key, long long value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%I", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

sds tojson_ulong(sds buffer, const char *key, unsigned long value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%L", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

sds tojson_ullong(sds buffer, const char *key, unsigned long long value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%U", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

sds tojson_double(sds buffer, const char *key, double value, bool comma) {
    buffer = sdscatprintf(buffer, "\"%s\":%f", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

//prints the keys of a list as a json array
sds list_to_json_array(sds s, struct t_list *l) {
    struct t_list_node *current = l->head;
    int i = 0;
    while (current != NULL) {
        if (i++) {
            s = sdscatlen(s, ",", 1);
        }
        s = sds_catjson(s, current->key, sdslen(current->key));
        current = current->next;
    }
    return s;
}

sds json_get_cols_as_string(sds s, sds cols, bool *rc) {
    struct t_list col_list;
    list_init(&col_list);
    if (json_get_array_string(s, "$.params.cols", &col_list, vcb_iscolumn, 20, NULL) == true) {
        cols = list_to_json_array(cols, &col_list);
        *rc = true;
    }
    else {
        *rc = false;
    }
    list_clear(&col_list);
    return cols;
}

bool json_get_bool(sds s, const char *path, bool *result, sds *error) {
    int v = 0;
    if (mjson_get_bool(s, (int)sdslen(s), path, &v) != 0) {
        *result = v == 1 ? true : false;
        return true;
    }
    _set_parse_error(error, "JSON path \"%s\" not found", path);
    return false;
}

bool json_get_int_max(sds s, const char *path, int *result, sds *error) {
    return json_get_int(s, path, JSONRPC_INT_MIN, JSONRPC_INT_MAX, result, error);
}

bool json_get_int(sds s, const char *path, int min, int max, int *result, sds *error) {
    long result_long;
    bool rc = json_get_long(s, path, min, max, &result_long, error);
    if (rc == true) {
        *result = (int)result_long;
    }
    return rc;
}

bool json_get_long_max(sds s, const char *path, long *result, sds *error) {
    return json_get_long(s, path, JSONRPC_LONG_MIN, JSONRPC_LONG_MAX, result, error);
}

bool json_get_long(sds s, const char *path, long min, long max, long *result, sds *error) {
    double value;
    if (mjson_get_number(s, (int)sdslen(s), path, &value) != 0) {
        long value_long = (long)value;
        if (value_long >= min && value_long <= max) {
            *result = value_long;
            return true;
        }
        _set_parse_error(error, "Number out of range for JSON path \"%s\"", path);
    }
    else {
        _set_parse_error(error, "JSON path \"%s\" not found", path);
    }
    return false;
}

bool json_get_uint_max(sds s, const char *path, unsigned *result, sds *error) {
    return json_get_uint(s, path, 0, JSONRPC_INT_MAX, result, error);
}

bool json_get_uint(sds s, const char *path, unsigned min, unsigned max, unsigned *result, sds *error) {
    double value;
    if (mjson_get_number(s, (int)sdslen(s), path, &value) != 0) {
        if (value >= min && value <= max) {
            *result = (unsigned)value;
            return true;
        }
        _set_parse_error(error, "Number out of range for JSON path \"%s\"", path);
    }
    else {
        _set_parse_error(error, "JSON path \"%s\" not found", path);
    }
    return false;
}

bool json_get_string_max(sds s, const char *path, sds *result, validate_callback vcb, sds *error) {
    if (vcb == NULL) {
        _set_parse_error(error, "Validation callback is NULL");
        return false;
    }
    return _json_get_string(s, path, 0, JSONRPC_STR_MAX, result, vcb, error);
}

bool json_get_string_cmp(sds s, const char *path, size_t min, size_t max, const char *cmp, sds *result, sds *error) {
    if (_json_get_string(s, path, min, max, result, NULL, error) == false) {
        return false;
    }
    if (strcmp(*result, cmp) != 0) {
        sdsclear(*result);
        _set_parse_error(error, "Value of JSON path \"%s\" is not equal \"%s\"", path, cmp);
        return false;
    }
    return true;
}

bool json_get_string(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb, sds *error) {
    if (vcb == NULL) {
        _set_parse_error(error, "Validation callback is NULL");
        return false;
    }
    return _json_get_string(s, path, min, max, result, vcb, error);
}

bool json_iterate_object(sds s, const char *path, iterate_callback icb, void *icb_userdata, validate_callback vcb, int max_elements, sds *error) {
    if (icb == NULL) {
        _set_parse_error(error, "Iteration callback is NULL");
        return false;
    }
    const char *p;
    int n;
    int otype = mjson_find(s, (int)sdslen(s), path, &p, &n);
    if (otype != MJSON_TOK_OBJECT && otype != MJSON_TOK_ARRAY) {
        _set_parse_error(error, "Invalid json object type for JSON path \"%s\": %d", path, otype);
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
        if (klen > JSONRPC_KEY_MAX) {
            _set_parse_error(error, "Key in JSON path \"%s\" is too long", path);
            FREE_SDS(value);
            FREE_SDS(key);
            return false;
        }
        if (klen > 2) {
            if (sds_json_unescape(p + koff + 1, (size_t)(klen - 2), &key) == false ||
                vcb_isalnum(value) == false)
            {
                _set_parse_error(error, "Validation of key in path \"%s\" has failed. Key must be alphanumeric.", path);
                FREE_SDS(value);
                FREE_SDS(key);
                return false;
            }
        }
        if (vlen > JSONRPC_STR_MAX) {
            _set_parse_error(error, "Value for key \"%s\" in JSON path \"%s\" is too long", key, path);
            FREE_SDS(value);
            FREE_SDS(key);
            return false;
        }
        switch(vtype) {
            case MJSON_TOK_STRING:
                if (vlen > 2) {
                    if (sds_json_unescape(p + voff + 1, (size_t)(vlen - 2), &value) == false) {
                        _set_parse_error(error, "JSON unescape error for value for key \"%s\" in JSON path \"%s\" has failed", key, path);
                        FREE_SDS(value);
                        FREE_SDS(key);
                        return false;
                    }
                }
                break;
            case MJSON_TOK_INVALID:
            case MJSON_TOK_NULL:
                _set_parse_error(error, "Invalid json value type");
                FREE_SDS(value);
                FREE_SDS(key);
                return false;
            default:
                value = sdscatlen(value, p + voff, (size_t)vlen);
                break;
        }

        if (icb(key, value, vtype, vcb, icb_userdata, error) == false) {
            MYMPD_LOG_WARN("Iteration callback for path \"%s\" has failed", path);
            FREE_SDS(value);
            FREE_SDS(key);
            return false;
        }

        sdsclear(value);
        sdsclear(key);
        i++;

        if (i == max_elements) {
            break;
        }
    }
    FREE_SDS(value);
    FREE_SDS(key);
    return true;
}

static bool icb_json_get_array_string(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error) {
    (void) key;
    if (vtype != MJSON_TOK_STRING ||
        vcb(value) == false)
    {
        _set_parse_error(error, "Validation of value \"%s\" has failed", value);
        return false;
    }
    struct t_list *l = (struct t_list *)userdata;
    list_push(l, value, 0, NULL, NULL);
    return true;
}

bool json_get_array_string(sds s, const char *path, struct t_list *l, validate_callback vcb, int max_elements, sds *error) {
    return json_iterate_object(s, path, icb_json_get_array_string, l, vcb, max_elements, error);
}

static bool icb_json_get_object_string(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error) {
    if (sdslen(key) == 0 ||
        vtype != MJSON_TOK_STRING ||
        vcb(value) == false)
    {
        _set_parse_error(error, "Validation of key \"%s\" with value \"%s\" has failed", key, value);
        return false;
    }
    struct t_list *l = (struct t_list *)userdata;
    list_push(l, key, 0, value, NULL);
    return true;
}

bool json_get_object_string(sds s, const char *path, struct t_list *l, validate_callback vcb, int max_elements, sds *error) {
    return json_iterate_object(s, path, icb_json_get_object_string, l, vcb, max_elements, error);
}

bool json_get_tags(sds s, const char *path, struct t_tags *tags, int max_elements, sds *error) {
    return json_iterate_object(s, path, _icb_json_get_tag, tags, NULL, max_elements, error);
}

bool json_find_key(sds s, const char *path) {
    const char *p;
    int n;
    int vtype = mjson_find(s, (int)sdslen(s), path, &p, &n);
    return vtype == MJSON_TOK_INVALID ? false : true;
}

const char *get_mjson_toktype_name(int vtype) {
    switch(vtype) {
        case MJSON_TOK_INVALID: return "MJSON_TOK_INVALID";
        case MJSON_TOK_KEY:     return "MJSON_TOK_KEY";
        case MJSON_TOK_STRING:  return "MJSON_TOK_STRING";
        case MJSON_TOK_NUMBER:  return "MJSON_TOK_NUMBER";
        case MJSON_TOK_TRUE:    return "MJSON_TOK_TRUE";
        case MJSON_TOK_FALSE:   return "MJSON_TOK_FALSE";
        case MJSON_TOK_NULL:    return "MJSON_TOK_NULL";
        case MJSON_TOK_ARRAY:   return "MJSON_TOK_ARRAY";
        case MJSON_TOK_OBJECT:  return "MJSON_TOK_OBJECT";
        default:                return "MJSON_TOK_UNKNOWN";
    }
}

//private functions

static bool _icb_json_get_tag(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error) {
    (void) vcb;
    (void) key;
    if (vtype != MJSON_TOK_STRING) {
        _set_parse_error(error, "Value is not a string \"%s\"", value);
        return false;
    }

    struct t_tags *tags = (struct t_tags *) userdata;
    enum mpd_tag_type tag = mpd_tag_name_iparse(value);
    if (tag != MPD_TAG_UNKNOWN) {
        tags->tags[tags->len++] = tag;
    }
    return true;
}

static void _set_parse_error(sds *error, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (error != NULL && *error != NULL) {
        *error = sdscatvprintf(*error, fmt, args); // NOLINT(clang-diagnostic-format-nonliteral)
        MYMPD_LOG_WARN("%s", *error);
    }
    else {
        sds e = sdscatvprintf(sdsempty(), fmt, args); // NOLINT(clang-diagnostic-format-nonliteral)
        MYMPD_LOG_WARN("%s", e);
        FREE_SDS(e);
    }
    va_end(args);
}

static bool _json_get_string(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb, sds *error) {
    if (*result != NULL) {
        MYMPD_LOG_ERROR("Result parameter must be NULL, path: \"%s\"", path);
        return false;
    }
    const char *p;
    int n;
    int vtype = mjson_find(s, (int)sdslen(s), path, &p, &n);
    if (vtype != MJSON_TOK_STRING) {
        *result = NULL;
        _set_parse_error(error, "JSON path \"%s\" not found or value is not string type, found type is \"%s\"",
            path, get_mjson_toktype_name(vtype));
        return false;
    }
    *result = sdsempty();
    if (n <= 2) {
        //empty string
        if (min == 0) {
            return true;
        }
        _set_parse_error(error, "Value length for JSON path \"%s\" is too short", path);
        FREE_SDS(*result);
        return false;
    }

    //strip quotes
    n = n - 2;
    p++;

    if ((sds_json_unescape(p, (size_t)n, result) == false) ||
        (sdslen(*result) < min || sdslen(*result) > max))
    {
        _set_parse_error(error, "Value length %lu for JSON path \"%s\" is out of bounds", sdslen(*result), path);
        FREE_SDS(*result);
        return false;
    }

    if (vcb != NULL) {
        if (vcb(*result) == false) {
            _set_parse_error(error, "Validation of value for JSON path \"%s\" has failed", path);
            FREE_SDS(*result);
            return false;
        }
    }

    return true;
}
