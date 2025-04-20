/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Jsonrpc implementation
 */

#ifndef MYMPD_JSONRPC_H
#define MYMPD_JSONRPC_H

#include "dist/sds/sds.h"
#include "src/lib/api.h"

#include <stdbool.h>

/**
 * Response types for error messages
 */
enum jsonrpc_response_types {
    RESPONSE_TYPE_JSONRPC_RESPONSE = 0,
    RESPONSE_TYPE_JSONRPC_NOTIFY,
    RESPONSE_TYPE_PLAIN,
    RESPONSE_TYPE_NONE
};

/**
 * Jsonrpc severities
 */
enum jsonrpc_severities {
    JSONRPC_SEVERITY_EMERG = 0,
    JSONRPC_SEVERITY_ALERT,
    JSONRPC_SEVERITY_CRIT,
    JSONRPC_SEVERITY_ERROR,
    JSONRPC_SEVERITY_WARN,
    JSONRPC_SEVERITY_NOTICE,
    JSONRPC_SEVERITY_INFO,
    JSONRPC_SEVERITY_DEBUG,
    JSONRPC_SEVERITY_MAX
};

/**
 * Jsonrpc facilities
 */
enum jsonrpc_facilities {
    JSONRPC_FACILITY_DATABASE = 0,
    JSONRPC_FACILITY_GENERAL,
    JSONRPC_FACILITY_HOME,
    JSONRPC_FACILITY_JUKEBOX,
    JSONRPC_FACILITY_LYRICS,
    JSONRPC_FACILITY_MPD,
    JSONRPC_FACILITY_PLAYLIST,
    JSONRPC_FACILITY_PLAYER,
    JSONRPC_FACILITY_QUEUE,
    JSONRPC_FACILITY_SESSION,
    JSONRPC_FACILITY_SCRIPT,
    JSONRPC_FACILITY_STICKER,
    JSONRPC_FACILITY_TIMER,
    JSONRPC_FACILITY_TRIGGER,
    JSONRPC_FACILITY_MAX
};

/**
 * Jsonrpc events
 */
enum jsonrpc_events {
    JSONRPC_EVENT_MPD_CONNECTED = 0,
    JSONRPC_EVENT_MPD_DISCONNECTED,
    JSONRPC_EVENT_NOTIFY,
    JSONRPC_EVENT_UPDATE_DATABASE,
    JSONRPC_EVENT_UPDATE_FINISHED,
    JSONRPC_EVENT_UPDATE_HOME,
    JSONRPC_EVENT_UPDATE_JUKEBOX,
    JSONRPC_EVENT_UPDATE_LAST_PLAYED,
    JSONRPC_EVENT_UPDATE_OPTIONS,
    JSONRPC_EVENT_UPDATE_OUTPUTS,
    JSONRPC_EVENT_UPDATE_QUEUE,
    JSONRPC_EVENT_UPDATE_STARTED,
    JSONRPC_EVENT_UPDATE_STATE,
    JSONRPC_EVENT_UPDATE_STORED_PLAYLIST,
    JSONRPC_EVENT_UPDATE_VOLUME,
    JSONRPC_EVENT_WELCOME,
    JSONRPC_EVENT_UPDATE_CACHE_STARTED,
    JSONRPC_EVENT_UPDATE_CACHE_FINISHED,
    JSONRPC_EVENT_MAX
};

void send_jsonrpc_notify(enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *partition, const char *message);
void send_jsonrpc_notify_client(enum jsonrpc_facilities facility, enum jsonrpc_severities severity, unsigned request_id, const char *message);
void send_jsonrpc_event(enum jsonrpc_events event, const char *partition);
sds jsonrpc_event(sds buffer, enum jsonrpc_events event);
sds jsonrpc_notify(sds buffer, enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *message);
sds jsonrpc_notify_phrase(sds buffer, enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *message, int count, ...);
sds jsonrpc_notify_start(sds buffer, enum jsonrpc_events event);

sds jsonrpc_respond_start(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id);
sds jsonrpc_end(sds buffer);
sds jsonrpc_respond_ok(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id, enum jsonrpc_facilities facility);
sds jsonrpc_respond_with_ok_or_error(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id,
        bool rc, enum jsonrpc_facilities facility, const char *error);
sds jsonrpc_respond_with_message_or_error(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id,
        bool rc, enum jsonrpc_facilities facility, const char *message, const char *error);
sds jsonrpc_respond_message(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id,
        enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *message);
sds jsonrpc_respond_message_phrase(sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id,
        enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *message, int count, ...);

#endif
