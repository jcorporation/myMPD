/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "jsonrpc.h"

#include "../global.h"
#include "log.h"
#include "sds_extras.h"

#include <string.h>

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
