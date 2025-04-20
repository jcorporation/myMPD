/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Jsonrpc implementation
 */

#include "compile_time.h"
#include "src/lib/json/json_rpc.h"

#include "src/lib/api.h"
#include "src/lib/json/json_print.h"
#include "src/lib/sds_extras.h"

#include <string.h>

/**
 * This unit provides functions for jsonrpc printing
 * Json parsing is done by mjson
 */

/**
 * private definitions
 */
static const char *jsonrpc_facility_name(enum jsonrpc_facilities facility);
static const char *jsonrpc_severity_name(enum jsonrpc_severities severity);
static const char *jsonrpc_event_name(enum jsonrpc_events event);

/**
 * Names for enum jsonrpc_events
 */
static const char *jsonrpc_severity_names[JSONRPC_SEVERITY_MAX] = {
    [JSONRPC_SEVERITY_EMERG] = "emerg",
    [JSONRPC_SEVERITY_ALERT] = "alert",
    [JSONRPC_SEVERITY_CRIT] = "crit",
    [JSONRPC_SEVERITY_ERROR] = "error",
    [JSONRPC_SEVERITY_WARN] = "warn",
    [JSONRPC_SEVERITY_NOTICE] = "notice",
    [JSONRPC_SEVERITY_INFO] = "info",
    [JSONRPC_SEVERITY_DEBUG] = "debug"
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
        request_id, (severity > JSONRPC_SEVERITY_WARN ? "result" : "error"));
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
