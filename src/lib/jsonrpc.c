/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/jsonrpc.h"

#include "dist/mjson/mjson.h"
#include "src/lib/api.h"
#include "src/lib/convert.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/sticker.h"
#include "src/mpd_client/tags.h"

#include <errno.h>
#include <string.h>

/**
 * This unit provides functions for jsonrpc and json parsing and printing
 * Json parsing is done by mjson
 */

/**
 * private definitions
 */
static bool icb_json_get_field(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error);
static bool icb_json_get_tag_value(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error);
static bool json_get_string_unescape(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb, struct t_jsonrpc_parse_error *error);
static void set_parse_error(struct t_jsonrpc_parse_error *error, const char *path, const char *key, const char *fmt, ...);
static const char *jsonrpc_facility_name(enum jsonrpc_facilities facility);
static const char *jsonrpc_severity_name(enum jsonrpc_severities severity);
static const char *jsonrpc_event_name(enum jsonrpc_events event);

/**
 * Names for enum jsonrpc_events
 */
static const char *jsonrpc_severity_names[JSONRPC_SEVERITY_MAX] = {
    [JSONRPC_SEVERITY_INFO] = "info",
    [JSONRPC_SEVERITY_WARN] = "warn",
    [JSONRPC_SEVERITY_ERROR] = "error"
};

/**
 * Names for enum jsonrpc_facilities
 */
static const char *jsonrpc_facility_names[JSONRPC_FACILITY_MAX] = {
    [JSONRPC_FACILITY_DATABASE] = "database",
    [JSONRPC_FACILITY_GENERAL] = "general",
    [JSONRPC_FACILITY_HOME] = "home",
    [JSONRPC_FACILITY_JUKEBOX] = "jukebox",
    [JSONRPC_FACILITY_LYRICS] = "lyrics",
    [JSONRPC_FACILITY_MPD] = "mpd",
    [JSONRPC_FACILITY_PLAYLIST] = "playlist",
    [JSONRPC_FACILITY_PLAYER] = "player",
    [JSONRPC_FACILITY_QUEUE] = "queue",
    [JSONRPC_FACILITY_SESSION] = "session",
    [JSONRPC_FACILITY_SCRIPT] = "script",
    [JSONRPC_FACILITY_STICKER] = "sticker",
    [JSONRPC_FACILITY_TIMER] = "timer",
    [JSONRPC_FACILITY_TRIGGER] = "trigger"
};

/**
 * Names for myMPD events
 */
static const char *jsonrpc_event_names[JSONRPC_EVENT_MAX] = {
    [JSONRPC_EVENT_MPD_CONNECTED] = "mpd_connected",
    [JSONRPC_EVENT_MPD_DISCONNECTED] = "mpd_disconnected",
    [JSONRPC_EVENT_NOTIFY] = "notify",
    [JSONRPC_EVENT_UPDATE_DATABASE] = "update_database",
    [JSONRPC_EVENT_UPDATE_FINISHED] = "update_finished",
    [JSONRPC_EVENT_UPDATE_HOME] = "update_home",
    [JSONRPC_EVENT_UPDATE_JUKEBOX] = "update_jukebox",
    [JSONRPC_EVENT_UPDATE_LAST_PLAYED] = "update_last_played",
    [JSONRPC_EVENT_UPDATE_OPTIONS] = "update_options",
    [JSONRPC_EVENT_UPDATE_OUTPUTS] = "update_outputs",
    [JSONRPC_EVENT_UPDATE_QUEUE] = "update_queue",
    [JSONRPC_EVENT_UPDATE_STARTED] = "update_started",
    [JSONRPC_EVENT_UPDATE_STATE] = "update_state",
    [JSONRPC_EVENT_UPDATE_STORED_PLAYLIST] = "update_stored_playlist",
    [JSONRPC_EVENT_UPDATE_VOLUME] = "update_volume",
    [JSONRPC_EVENT_WELCOME] = "welcome",
    [JSONRPC_EVENT_UPDATE_CACHE_STARTED] = "update_cache_started",
    [JSONRPC_EVENT_UPDATE_CACHE_FINISHED] = "update_cache_finished"
};

/**
 * public functions
 */

/**
 * Jsonrpc printing
 */

/**
 * Creates and sends a jsonrpc notify to all connected websockets for a partition
 * @param facility one of enum jsonrpc_facilities
 * @param severity one of enum jsonrpc_severities
 * @param partition mpd partition
 * @param message the message to send
 */
void send_jsonrpc_notify(enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *partition, const char *message) {
    sds buffer = jsonrpc_notify(sdsempty(), facility, severity, message);
    ws_notify(buffer, partition);
    FREE_SDS(buffer);
}

/**
 * Creates and sends a jsonrpc notify to a client
 * @param facility one of enum jsonrpc_facilities
 * @param severity one of enum jsonrpc_severities
 * @param request_id the jsonrpc id of the client
 * @param message the message to send
 */
void send_jsonrpc_notify_client(enum jsonrpc_facilities facility, enum jsonrpc_severities severity, unsigned request_id, const char *message) {
    sds buffer = jsonrpc_notify(sdsempty(), facility, severity, message);
    ws_notify_client(buffer, request_id);
    FREE_SDS(buffer);
}

/**
 * Creates and sends a jsonrpc event to all connected websockets
 * @param event the event to send
 * @param partition mpd partition
 */
void send_jsonrpc_event(enum jsonrpc_events event, const char *partition) {
    sds buffer = jsonrpc_event(sdsempty(), event);
    ws_notify(buffer, partition);
    FREE_SDS(buffer);
}

/**
 * Creates a simple jsonrpc notification with the event as method
 * @param buffer pointer to already allocated sds string
 * @param event the event to use
 * @return pointer to buffer with jsonrpc string
 */
sds jsonrpc_event(sds buffer, enum jsonrpc_events event) {
    const char *event_name = jsonrpc_event_name(event);
    sdsclear(buffer);
    buffer = sdscat(buffer, "{\"jsonrpc\":\"2.0\",");
    buffer = tojson_char(buffer, "method", event_name, true);
    buffer = sdscat(buffer, "\"params\":{}}");
    return buffer;
}

/**
 * Creates a jsonrpc notification with facility, severity and a message
 * @param buffer pointer to already allocated sds string
 * @param facility one of enum jsonrpc_facilities
 * @param severity one of enum jsonrpc_severities
 * @param message the message to send
 * @return pointer to buffer with jsonrpc string
 */
sds jsonrpc_notify(sds buffer, enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *message) {
    return jsonrpc_notify_phrase(buffer, facility, severity, message, 0);
}

/**
 * Creates a jsonrpc notification with facility, severity and a message phrase.
 * A message phrase can include %{key} placeholders that are replaced on the client side
 * with the value. Key/value pairs are variadic arguments.
 * @param buffer pointer to already allocated sds string
 * @param facility one of enum jsonrpc_facilities
 * @param severity one of enum jsonrpc_severities
 * @param message the message to send
 * @param count number of following variadic arguments
 * @param ... key/value pairs for the phrase
 * @return pointer to buffer with jsonrpc string
 */
sds jsonrpc_notify_phrase(sds buffer, enum jsonrpc_facilities facility, enum jsonrpc_severities severity,
        const char *message, int count, ...)
{
    buffer = jsonrpc_notify_start(buffer, JSONRPC_EVENT_NOTIFY);
    const char *facility_name = jsonrpc_facility_name(facility);
    const char *severity_name = jsonrpc_severity_name(severity);
    buffer = tojson_char(buffer, "facility", facility_name, true);
    buffer = tojson_char(buffer, "severity", severity_name, true);
    buffer = tojson_char(buffer, "message", message, true);
    buffer = sdscat(buffer, "\"data\":{");
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        const char *v = va_arg(args, char *);
        if (i % 2 == 0) {
            //key
            if (i > 0) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sds_catjson(buffer, v, strlen(v));
            buffer = sdscatlen(buffer,":", 1);
        }
        else {
            //value
            buffer = sds_catjson(buffer, v, strlen(v));
        }
    }
    va_end(args);
    buffer = sdscatlen(buffer, "}}}", 3);
    return buffer;
}

/**
 * Creates the start of a jsonrpc notification.
 * @param buffer pointer to already allocated sds string
 * @param event the event to use
 * @return pointer to buffer with jsonrpc string
 */
sds jsonrpc_notify_start(sds buffer, enum jsonrpc_events event) {
    const char *event_name = jsonrpc_event_name(event);
    sdsclear(buffer);
    buffer = sdscat(buffer, "{\"jsonrpc\":\"2.0\",");
    buffer = tojson_char(buffer, "method", event_name, true);
    buffer = sdscat(buffer, "\"params\":{");
    return buffer;
}

/**
 * Creates the start of a jsonrpc response.
 * @param buffer pointer to already allocated sds string
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id id of the jsonrpc request to answer
 * @return pointer to buffer with jsonrpc string
 */
sds jsonrpc_respond_start(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id) {
    const char *method = get_cmd_id_method_name(cmd_id);
    sdsclear(buffer);
    buffer = sdscatfmt(buffer, "{\"jsonrpc\":\"2.0\",\"id\":%u,\"result\":{", request_id);
    buffer = tojson_char(buffer, "method", method, true);
    return buffer;
}

/**
 * Creates the end of a jsonrpc response
 * @param buffer pointer to already allocated sds string
 * @return pointer to buffer with jsonrpc string
 */
sds jsonrpc_end(sds buffer) {
    return sdscatlen(buffer, "}}", 2);
}

/**
 * Creates a simple jsonrpc response with "ok" as message
 * @param buffer pointer to already allocated sds string
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id id of the jsonrpc request to answer
 * @param facility one of enum jsonrpc_facilities
 * @return pointer to buffer with jsonrpc string
 */
sds jsonrpc_respond_ok(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id, enum jsonrpc_facilities facility) {
    return jsonrpc_respond_message_phrase(buffer, cmd_id, request_id, facility, JSONRPC_SEVERITY_INFO, "ok", 0);
}

/**
 * Checks rc and responses with ok or the message
 * @param buffer already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param rc return code to check
 * @param facility one of enum jsonrpc_facilities
 * @param error the response message, if rc == false
 * @return pointer to buffer
 */
sds jsonrpc_respond_with_ok_or_error(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id,
        bool rc, enum jsonrpc_facilities facility, const char *error)
{
    return jsonrpc_respond_with_message_or_error(buffer, cmd_id, request_id,
        rc, facility, "ok", error);
}

/**
 * Checks rc and responses with ok or the message
 * @param buffer already allocated sds string for the jsonrpc response
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id jsonrpc request id to respond
 * @param rc return code to check
 * @param facility one of enum jsonrpc_facilities
 * @param message the response message, if rc == true
 * @param error the response message, if rc == false
 * @return pointer to buffer
 */
sds jsonrpc_respond_with_message_or_error(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id,
        bool rc, enum jsonrpc_facilities facility, const char *message, const char *error)
{
    return rc == true
        ? jsonrpc_respond_message(buffer, cmd_id, request_id, facility, JSONRPC_SEVERITY_INFO, message)
        : jsonrpc_respond_message(buffer, cmd_id, request_id, facility, JSONRPC_SEVERITY_ERROR, error);
}

/**
 * Creates a simple jsonrpc response with a custom message
 * @param buffer pointer to already allocated sds string
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id id of the jsonrpc request to answer
 * @param facility one of enum jsonrpc_facilities
 * @param severity one of enum jsonrpc_severities
 * @param message the response message
 * @return pointer to buffer with jsonrpc string
 */
sds jsonrpc_respond_message(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id,
        enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *message)
{
    return jsonrpc_respond_message_phrase(buffer, cmd_id, request_id, facility, severity, message, 0);
}

/**
 * Creates a jsonrpc response with facility, severity and a message phrase.
 * A message phrase can include %{key} placeholders that are replaced on the client side
 * with the value. Key/value pairs are variadic arguments.
 * @param buffer pointer to already allocated sds string
 * @param cmd_id enum mympd_cmd_ids
 * @param request_id id of the jsonrpc request to answer
 * @param facility one of enum jsonrpc_facilities
 * @param severity one of enum jsonrpc_severities
 * @param message the message to send
 * @param count number of following variadic arguments
 * @param ... key/value pairs for the phrase
 * @return pointer to buffer with jsonrpc string
 */
sds jsonrpc_respond_message_phrase(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id,
        enum jsonrpc_facilities facility, enum jsonrpc_severities severity,
        const char *message, int count, ...)
{
    const char *method = get_cmd_id_method_name(cmd_id);
    const char *facility_name = jsonrpc_facility_name(facility);
    const char *severity_name = jsonrpc_severity_name(severity);
    sdsclear(buffer);
    buffer = sdscatfmt(buffer, "{\"jsonrpc\":\"2.0\",\"id\":%u,\"%s\":{",
        request_id, (severity == JSONRPC_SEVERITY_INFO ? "result" : "error"));
    buffer = tojson_char(buffer, "method", method, true);
    buffer = tojson_char(buffer, "facility", facility_name, true);
    buffer = tojson_char(buffer, "severity", severity_name, true);
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

/**
 * Json emitting
 */

/**
 * Appends a comma on demand.
 * Comma is omitted on start of string or end of string is already a comma.
 * @param buffer sds string to append
 * @return pointer to buffer
 */
sds json_comma(sds buffer) {
    size_t len = sdslen(buffer);
    if (len == 0 ||
        buffer[len - 1] == ',')
    {
        return buffer;
    }
    return sdscatlen(buffer, ",", 1);
}

/**
 * Prints a json key/value pair for already encoded values
 * value is printed raw without any encoding done
 * @param buffer sds string to append
 * @param key json key
 * @param value raw data
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_raw(sds buffer, const char *key, const char *value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%s", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for 0-terminated char values
 * value is encoded as json
 * @param buffer sds string to append
 * @param key json key
 * @param value to encode as json
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_char(sds buffer, const char *key, const char *value, bool comma) {
    //treat NULL values as empty
    size_t len = value != NULL ? strlen(value) : 0;
    return tojson_char_len(buffer, key, value, len, comma);
}

/**
 * Prints a json key/value pair for sds values
 * value is encoded as json
 * @param buffer sds string to append
 * @param key json key
 * @param value as sds string to encode as json
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_sds(sds buffer, const char *key, sds value, bool comma) {
    return tojson_char_len(buffer, key, value, sdslen(value), comma);
}

/**
 * Prints a json key/value pair for not 0-terminated values
 * value is encoded as json
 * @param buffer sds string to append
 * @param key json key
 * @param value as sds string to encode as json
 * @param len length of value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
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

/**
 * Prints a json key/value pair for a bool value
 * @param buffer sds string to append
 * @param key json key
 * @param value bool value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_bool(sds buffer, const char *key, bool value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%s", key, value == true ? "true" : "false");
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for an int value
 * @param buffer sds string to append
 * @param key json key
 * @param value integer value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_int(sds buffer, const char *key, int value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%i", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for an unsigned
 * @param buffer sds string to append
 * @param key json key
 * @param value unsigned integer value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_uint(sds buffer, const char *key, unsigned value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%u", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for a time_t value
 * @param buffer sds string to append
 * @param key json key
 * @param value time_t value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_time(sds buffer, const char *key, time_t value, bool comma) {
    return tojson_int64(buffer, key, (int64_t)value, comma);
}

/**
 * Prints a json key/value pair for a float value
 * @param buffer sds string to append
 * @param key json key
 * @param value double value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_float(sds buffer, const char *key, float value, bool comma) {
    buffer = sdscatprintf(buffer, "\"%s\":%.2f", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for an int64_t value
 * @param buffer sds string to append
 * @param key json key
 * @param value int64_t value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_int64(sds buffer, const char *key, int64_t value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%I", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints a json key/value pair for an uint64_t value
 * @param buffer sds string to append
 * @param key json key
 * @param value uint64_t value
 * @param comma true to append a comma
 * @return pointer to buffer
 */
sds tojson_uint64(sds buffer, const char *key, uint64_t value, bool comma) {
    buffer = sdscatfmt(buffer, "\"%s\":%U", key, value);
    if (comma) {
        buffer = sdscatlen(buffer, ",", 1);
    }
    return buffer;
}

/**
 * Prints the keys of a list as a json array
 * Leading and ending square brackets are added
 * @param s sds string to append
 * @param l pointer to list to add keys from
 * @return pointer to s
 */
sds list_to_json_array(sds s, struct t_list *l) {
    s = sdscatlen(s, "[", 1);
    struct t_list_node *current = l->head;
    int i = 0;
    while (current != NULL) {
        if (i++) {
            s = sdscatlen(s, ",", 1);
        }
        s = sds_catjson(s, current->key, sdslen(current->key));
        current = current->next;
    }
    s = sdscatlen(s, "]", 1);
    return s;
}

/**
 * Json parsing functions
 * All this functions are validating the result.
 */

/**
 * Initializes the sds strings t_jsonrpc_parse_error struct to NULL
 * @param parse_error t_jsonrpc_parse_error struct to initialize
 */
void jsonrpc_parse_error_init(struct t_jsonrpc_parse_error *parse_error) {
    parse_error->message = NULL;
    parse_error->path = NULL;
}

/**
 * Clears the sds strings
 * @param parse_error t_jsonrpc_parse_error struct to clear
 */
void jsonrpc_parse_error_clear(struct t_jsonrpc_parse_error *parse_error) {
    FREE_SDS(parse_error->message);
    FREE_SDS(parse_error->path);
}

/**
 * Helper function to get myMPD fields out of a jsonrpc request
 * and return a validated json array
 * @param s sds string to parse
 * @param fields sds string to append the fields
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success, else false
 */
bool json_get_fields_as_string(sds s, sds *fields, struct t_jsonrpc_parse_error *error) {
    struct t_list col_list;
    list_init(&col_list);
    bool rc = json_get_array_string(s, "$.params.fields", &col_list, vcb_isfield, 20, error);
    if (rc == true) {
        *fields = list_to_json_array(*fields, &col_list);
    }
    list_clear(&col_list);
    return rc;
}

/**
 * Gets a bool value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to bool with the result
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_bool(sds s, const char *path, bool *result, struct t_jsonrpc_parse_error *error) {
    int v = 0;
    if (mjson_get_bool(s, (int)sdslen(s), path, &v) != 0) {
        *result = v == 1
            ? true
            : false;
        return true;
    }
    set_parse_error(error, path, "", "JSON path \"%s\" not found", path);
    return false;
}

/**
 * Gets a int value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to int with the result
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_int_max(sds s, const char *path, int *result, struct t_jsonrpc_parse_error *error) {
    return json_get_int(s, path, JSONRPC_INT_MIN, JSONRPC_INT_MAX, result, error);
}

/**
 * Gets a int value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @param result pointer to int with the result
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_int(sds s, const char *path, int min, int max, int *result, struct t_jsonrpc_parse_error *error) {
    double value;
    if (mjson_get_number(s, (int)sdslen(s), path, &value) != 0) {
        if (value >= JSONRPC_INT_MIN &&
            value <= JSONRPC_INT_MAX)
        {
            int value_int = (int)value;
            if (value_int >= min &&
                value_int <= max)
            {
                *result = value_int;
                return true;
            }
        }
        set_parse_error(error, path, "", "Number is out of valid range");
    }
    else {
        set_parse_error(error, path, "", "JSON path \"%s\" not found", path);
    }
    return false;
}

/**
 * Gets a time_t value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to time_t with the result
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_time_max(sds s, const char *path, time_t *result, struct t_jsonrpc_parse_error *error) {
    double value;
    if (mjson_get_number(s, (int)sdslen(s), path, &value) != 0) {
        if (value >= JSONRPC_TIME_MIN &&
            value <= JSONRPC_TIME_MAX)
        {
            time_t value_time = (time_t)value;
            *result = value_time;
            return true;
        }
        set_parse_error(error, path, "", "Number is out of valid range");
    }
    else {
        set_parse_error(error, path, "", "JSON path \"%s\" not found", path);
    }
    return false;
}

/**
 * Gets an int64_t value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to int64_t with the result
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_int64_max(sds s, const char *path, int64_t *result, struct t_jsonrpc_parse_error *error) {
    return json_get_int64(s, path, JSONRPC_INT64_MIN, JSONRPC_INT64_MAX, result, error);
}

/**
 * Gets an int64_t value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @param result pointer to int64_t with the result
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_int64(sds s, const char *path, int64_t min, int64_t max, int64_t *result, struct t_jsonrpc_parse_error *error) {
    double value;
    if (mjson_get_number(s, (int)sdslen(s), path, &value) != 0) {
        if (value >= (double)JSONRPC_INT64_MIN &&
            value <= (double)JSONRPC_INT64_MAX)
        {
            int64_t value_int64 = (int64_t)value;
            if (value_int64 >= min && value_int64 <= max) {
                *result = value_int64;
                return true;
            }
        }
        set_parse_error(error, path, "", "Number is out of valid range");
    }
    else {
        set_parse_error(error, path, "", "JSON path \"%s\" not found", path);
    }
    return false;
}

/**
 * Gets a unsigned int value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to unsigned int with the result
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_uint_max(sds s, const char *path, unsigned *result, struct t_jsonrpc_parse_error *error) {
    return json_get_uint(s, path, 0, JSONRPC_INT_MAX, result, error);
}

/**
 * Gets a unsigned int value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param min minimum value (including)
 * @param max maximum value (including)
 * @param result pointer to unsigned int with the result
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_uint(sds s, const char *path, unsigned min, unsigned max, unsigned *result, struct t_jsonrpc_parse_error *error) {
    double value;
    if (mjson_get_number(s, (int)sdslen(s), path, &value) != 0) {
        if (value >= JSONRPC_UINT_MIN &&
            value <= JSONRPC_UINT_MAX)
        {
            if (value >= min &&
                value <= max)
            {
                *result = (unsigned)value;
                return true;
            }
        }
        set_parse_error(error, path, "", "Number is out of valid range");
    }
    else {
        set_parse_error(error, path, "", "JSON path \"%s\" not found", path);
    }
    return false;
}

/**
 * Gets a string value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to sds with the result
 * @param vcb validation callback
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_string_max(sds s, const char *path, sds *result, validate_callback vcb, struct t_jsonrpc_parse_error *error) {
    if (vcb == NULL) {
        set_parse_error(error, path, "", "Validation callback is NULL");
        return false;
    }
    return json_get_string_unescape(s, path, 0, JSONRPC_STR_MAX, result, vcb, error);
}

/**
 * Gets a string value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to sds with the result
 * @param min minimum length (inclusive)
 * @param max maximum length (inclusive)
 * @param cmp compare result against this string
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_string_cmp(sds s, const char *path, size_t min, size_t max, const char *cmp, sds *result, struct t_jsonrpc_parse_error *error) {
    if (json_get_string_unescape(s, path, min, max, result, NULL, error) == false) {
        return false;
    }
    if (strcmp(*result, cmp) != 0) {
        sdsclear(*result);
        set_parse_error(error, path, "", "Value of JSON path \"%s\" is not equal \"%s\"", path, cmp);
        return false;
    }
    return true;
}

/**
 * Gets a string value by json path
 * @param s json object to parse
 * @param path mjson path expression
 * @param result pointer to int with the result
 * @param min minimum length (inclusive)
 * @param max maximum length (inclusive)
 * @param vcb validation callback
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_string(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb, struct t_jsonrpc_parse_error *error) {
    if (vcb == NULL) {
        set_parse_error(error, path, "", "Validation callback is NULL");
        return false;
    }
    return json_get_string_unescape(s, path, min, max, result, vcb, error);
}

/**
 * Iterates through object/array found by path
 * @param s json object to parse
 * @param path mjson path expression
 * @param icb iteration callback
 * @param icb_userdata custom data for iteration callback
 * @param vcb_key validation callback for the key
 * @param vcb_value validation callback for the value
 * @param max_elements maximum of elements
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_iterate_object(sds s, const char *path, iterate_callback icb, void *icb_userdata,
        validate_callback vcb_key, validate_callback vcb_value, int max_elements, struct t_jsonrpc_parse_error *error)
{
    if (icb == NULL) {
        set_parse_error(error, path, "", "Iteration callback is NULL");
        return false;
    }
    if (vcb_key == NULL) {
        vcb_key = vcb_isalnum;
    }
    const char *p;
    int n;
    int otype = mjson_find(s, (int)sdslen(s), path, &p, &n);
    if (otype != MJSON_TOK_OBJECT &&
        otype != MJSON_TOK_ARRAY)
    {
        set_parse_error(error, path, "", "Invalid json object type for JSON path \"%s\": %s", path, get_mjson_toktype_name(otype));
        return false;
    }
    if (n <= 2)
    {
        //empty object/array
        return true;
    }
    //iterable object/array
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
            set_parse_error(error, path, key, "Key in JSON path \"%s\" is too long", path);
            FREE_SDS(value);
            FREE_SDS(key);
            return false;
        }
        if (klen > 2) {
            if (sds_json_unescape(p + koff + 1, (size_t)(klen - 2), &key) == false ||
                vcb_key(key) == false)
            {
                set_parse_error(error, path, key, "Validation of key in path \"%s\" has failed.", path);
                FREE_SDS(value);
                FREE_SDS(key);
                return false;
            }
        }
        if (vlen > JSONRPC_STR_MAX) {
            set_parse_error(error, path, key, "Value for key \"%s\" in JSON path \"%s\" is too long", key, path);
            FREE_SDS(value);
            FREE_SDS(key);
            return false;
        }
        switch(vtype) {
            case MJSON_TOK_STRING:
                if (vlen > 2) {
                    if (sds_json_unescape(p + voff + 1, (size_t)(vlen - 2), &value) == false) {
                        set_parse_error(error, path, key, "JSON unescape error for value for key \"%s\" in JSON path \"%s\" has failed", key, path);
                        FREE_SDS(value);
                        FREE_SDS(key);
                        return false;
                    }
                }
                break;
            case MJSON_TOK_INVALID:
            case MJSON_TOK_NULL:
                set_parse_error(error, path, key, "Invalid json value type: %s", get_mjson_toktype_name(vtype));
                FREE_SDS(value);
                FREE_SDS(key);
                return false;
            default:
                value = sdscatlen(value, p + voff, (size_t)vlen);
                break;
        }
        if (sdslen(key) == 0) {
            //array - fallback to parent key
            const char *key_ptr = strrchr(path, '.');
            if (key_ptr != NULL) {
                key = sdscat(key, key_ptr + 1);
            }
        }
        if (icb(path, key, value, vtype, vcb_value, icb_userdata, error) == false) {
            MYMPD_LOG_WARN(NULL, "Iteration callback for path \"%s\" has failed", path);
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

/**
 * Iteration callback to populate mpd_song tag values
 * @param path json path
 * @param key json key
 * @param value json value
 * @param vtype mjson value type
 * @param vcb validation callback
 * @param userdata pointer to a t_list struct to populate
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
static bool icb_json_get_tag_value(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error) {
    enum mpd_tag_type tag = mpd_tag_name_parse(key);
    if (tag == MPD_TAG_UNKNOWN) {
        set_parse_error(error, path, "", "Unknown mpd tag type \"%s\"", key);
        return false;
    }
    if (vtype != MJSON_TOK_STRING) {
        set_parse_error(error, path, "", "Invalid type for tag \"%s\": %s", key, get_mjson_toktype_name(vtype));
        return false;
    }
    if (vcb(value) == false) {
        set_parse_error(error, path, "", "Validation of value \"%s\" has failed", value);
    }
    else {
        mympd_mpd_song_add_tag_dedup((struct mpd_song *)userdata, tag, value);
    }
    
    return true;
}

/**
 * Converts a json array to a mpd song tag value(s)
 * Shortcut for json_iterate_object with icb_json_get_tag_value
 * @param s json object to parse
 * @param path mjson path expression
 * @param song mpd_song struct
 * @param vcb validation callback
 * @param max_elements maximum of elements
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_tag_values(sds s, const char *path, struct mpd_song *song, validate_callback vcb, int max_elements, struct t_jsonrpc_parse_error *error) {
    return json_iterate_object(s, path, icb_json_get_tag_value, song, vcb, vcb, max_elements, error);
}

/**
 * Iteration callback to populate a list with json array of strings
 * @param path json path
 * @param key json key
 * @param value json value
 * @param vtype mjson value type
 * @param vcb validation callback
 * @param userdata pointer to a t_list struct to populate
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
static bool icb_json_get_array_string(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error) {
    (void)key;
    if (vtype != MJSON_TOK_STRING ||
        vcb(value) == false)
    {
        set_parse_error(error, path, "", "Validation of value \"%s\" has failed", value);
        return false;
    }
    struct t_list *l = (struct t_list *)userdata;
    list_push(l, value, 0, NULL, NULL);
    return true;
}

/**
 * Converts a json array of strings to a t_list struct
 * Shortcut for json_iterate_object with icb_json_get_array_string
 * @param s json object to parse
 * @param path mjson path expression
 * @param l t_list struct to populate
 * @param vcb validation callback
 * @param max_elements maximum of elements
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_array_string(sds s, const char *path, struct t_list *l, validate_callback vcb, int max_elements, struct t_jsonrpc_parse_error *error) {
    return json_iterate_object(s, path, icb_json_get_array_string, l, vcb, vcb, max_elements, error);
}

/**
 * Iteration callback to populate a list with json array of int64_t
 * @param path json path
 * @param key json key
 * @param value json value
 * @param vtype mjson value type
 * @param vcb validation callback - not used
 * @param userdata pointer to a t_list struct to populate
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
static bool icb_json_get_array_int64(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error) {
    (void)key;
    (void)vcb;
    if (vtype != MJSON_TOK_NUMBER) {
        set_parse_error(error, path, "", "Validation of value \"%s\" has failed", value);
        return false;
    }
    int64_t value_int64; 
    enum str2int_errno rc = str2int64(&value_int64, value);
    if (rc != STR2INT_SUCCESS) {
        return false;
    }
    struct t_list *l = (struct t_list *)userdata;
    list_push(l, "", value_int64, NULL, NULL);
    return true;
}

/**
 * Converts a json array of int64_t to a t_list struct
 * Shortcut for json_iterate_object with icb_json_get_array_int64
 * @param s json object to parse
 * @param path mjson path expression
 * @param l t_list struct to populate
 * @param max_elements maximum of elements
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_array_int64(sds s, const char *path, struct t_list *l, int max_elements, struct t_jsonrpc_parse_error *error) {
    return json_iterate_object(s, path, icb_json_get_array_int64, l, NULL, NULL, max_elements, error);
}

/**
 * Iteration callback to populate a list with json object key/values
 * @param path json path
 * @param key json key
 * @param value json value
 * @param vtype mjson value type
 * @param vcb validation callback
 * @param userdata pointer to a t_list struct to populate
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
static bool icb_json_get_object_string(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error) {
    if (sdslen(key) == 0 ||
        vtype != MJSON_TOK_STRING ||
        vcb(value) == false)
    {
        set_parse_error(error, path, key, "Validation of key \"%s\" with value \"%s\" has failed", key, value);
        return false;
    }
    struct t_list *l = (struct t_list *)userdata;
    list_push(l, key, 0, value, NULL);
    return true;
}

/**
 * Converts a json object key/values to a t_list struct
 * Shortcut for json_iterate_object with icb_json_get_object_string
 * @param s json object to parse
 * @param path mjson path expression
 * @param l t_list struct to populate
 * @param vcb validation callback
 * @param max_elements maximum of elements
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_object_string(sds s, const char *path, struct t_list *l, validate_callback vcb_key,
    validate_callback vcb_value, int max_elements, struct t_jsonrpc_parse_error *error)
{
    return json_iterate_object(s, path, icb_json_get_object_string, l, vcb_key, vcb_value, max_elements, error);
}

/**
 * Converts a json array to a struct t_tags
 * Shortcut for json_iterate_object with icb_json_get_tag
 * @param s json object to parse
 * @param path mjson path expression
 * @param tags t_tags struct to populate
 * @param max_elements maximum of elements
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
bool json_get_fields(sds s, const char *path, struct t_fields *tags, int max_elements, struct t_jsonrpc_parse_error *error) {
    return json_iterate_object(s, path, icb_json_get_field, tags, NULL, NULL, max_elements, error);
}

/**
 * Searches for a key in json object
 * @param s json object to search
 * @param path mjson path expression
 */
bool json_find_key(sds s, const char *path) {
    const char *p;
    int n;
    int vtype = mjson_find(s, (int)sdslen(s), path, &p, &n);
    return vtype == MJSON_TOK_INVALID ? false : true;
}

/**
 * Searches for a key in json object and returns its value as sds string
 * @param s json object to search
 * @param path mjson path expression
 */
sds json_get_key_as_sds(sds s, const char *path) {
    const char *p;
    int n;
    if (mjson_find(s, (int)sdslen(s), path, &p, &n) == MJSON_TOK_INVALID) {
        return false;
    }
    return sdsnewlen(p, (size_t)n);
}

/**
 * Returns the name of a mjson token type
 * @param vtype token type
 * @return token type as string
 */
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

/**
 * Returns the facility name
 * @param facility enum jsonrpc_facilities
 * @return name of the facility
 */
static const char *jsonrpc_facility_name(enum jsonrpc_facilities facility) {
    if ((unsigned)facility >= JSONRPC_FACILITY_MAX) {
        return "unknown";
    }
    return jsonrpc_facility_names[facility];
}

/**
 * Returns the severity name
 * @param severity enum jsonrpc_severities
 * @return name of the severity
 */
static const char *jsonrpc_severity_name(enum jsonrpc_severities severity) {
    if ((unsigned)severity >= JSONRPC_SEVERITY_MAX) {
        return "unknown";
    }
    return jsonrpc_severity_names[severity];
}

/**
 * Returns the event name
 * @param event enum jsonrpc_events
 * @return name of the event
 */
static const char *jsonrpc_event_name(enum jsonrpc_events event) {
    if ((unsigned)event >= JSONRPC_EVENT_MAX) {
        return "unknown";
    }
    return jsonrpc_event_names[event];
}

/**
 * Iteration callback to populate a t_tags struct
 * @param path json path
 * @param key not used
 * @param value value to parse as mpd tag
 * @param vtype mjson value type
 * @param vcb not used - we validate directly
 * @param userdata void pointer to t_tags struct
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success else false
 */
static bool icb_json_get_field(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error) {
    (void) key;
    (void) vcb;
    if (vtype != MJSON_TOK_STRING) {
        set_parse_error(error, path, key, "Value is not a string \"%s\"", value);
        return false;
    }

    struct t_fields *fields = (struct t_fields *) userdata;
    enum mpd_tag_type tag = mpd_tag_name_iparse(value);
    if (tag != MPD_TAG_UNKNOWN) {
        fields->tags.tags[fields->tags.len++] = tag;
        return true;
    }
    enum mympd_sticker_types sticker = sticker_name_parse(value);
    if (sticker != STICKER_UNKNOWN) {
        fields->stickers.stickers[fields->stickers.len++] = sticker;
    }
    return true;
}

/**
 * Helper function to set parsing errors
 * @param error already initialized t_jsonrpc_parse_error struct to propagate the error
 *              or NULL for log only
 * @param path json path where the error occurred
 * @param key optional json key inside the path where the error occurred
 *            set this to a blank string for none object paths
 * @param fmt printf format string
 * @param ... arguments for the format string
 */
static void set_parse_error(struct t_jsonrpc_parse_error *error, const char *path, const char *key, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (error != NULL) {
        error->message = sdscatvprintf(sdsempty(), fmt, args); // NOLINT(clang-diagnostic-format-nonliteral)
        error->path = key[0] != '\0'
            ? sdscatfmt(sdsempty(), "%s.%s", path, key)
            : sdsnew(path);
        MYMPD_LOG_WARN(NULL, "%s: %s", error->path, error->message);
    }
    else {
        sds e = sdscatvprintf(sdsempty(), fmt, args); // NOLINT(clang-diagnostic-format-nonliteral)
        MYMPD_LOG_WARN(NULL, "%s: %s", path, e);
        FREE_SDS(e);
    }
    va_end(args);
}

/**
 * Helper function to get a string from a json object
 * Enclosing quotes are removed and string is unescaped
 * @param s json object to parse
 * @param path path to the string to extract
 * @param min minimum length
 * @param max maximum length
 * @param result newly allocated sds string with the result
 * @param vcb validation callback
 * @param error pointer to t_jsonrpc_parse_error
 */
static bool json_get_string_unescape(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb, struct t_jsonrpc_parse_error *error) {
    if (*result != NULL) {
        MYMPD_LOG_ERROR(NULL, "Result parameter must be NULL, path: \"%s\"", path);
        return false;
    }
    const char *p;
    int n;
    int vtype = mjson_find(s, (int)sdslen(s), path, &p, &n);
    if (vtype != MJSON_TOK_STRING) {
        *result = NULL;
        set_parse_error(error, path, "", "JSON path \"%s\" not found or value is not string type, found type is \"%s\"",
            path, get_mjson_toktype_name(vtype));
        return false;
    }
    *result = sdsempty();
    if (n <= 2) {
        //empty string
        if (min == 0) {
            return true;
        }
        set_parse_error(error, path, "", "Value length for JSON path \"%s\" is too short", path);
        FREE_SDS(*result);
        return false;
    }

    //strip quotes
    n = n - 2;
    p++;

    if ((sds_json_unescape(p, (size_t)n, result) == false) ||
        (sdslen(*result) < min || sdslen(*result) > max))
    {
        set_parse_error(error, path, "", "Value length %lu for JSON path \"%s\" is out of bounds", (unsigned long)sdslen(*result), path);
        FREE_SDS(*result);
        return false;
    }

    if (vcb != NULL) {
        if (vcb(*result) == false) {
            set_parse_error(error, path, "", "Validation of value for JSON path \"%s\" has failed", path);
            FREE_SDS(*result);
            return false;
        }
    }

    return true;
}
