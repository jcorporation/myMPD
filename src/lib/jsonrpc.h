/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_JSONRPC_H
#define MYMPD_JSONRPC_H

#include "../../dist/sds/sds.h"
#include "api.h"
#include "list.h"
#include "mympd_state.h"
#include "validate.h"

#include <stdbool.h>

enum jsonrpc_severities {
    JSONRPC_SEVERITY_INFO = 0,
    JSONRPC_SEVERITY_WARN,
    JSONRPC_SEVERITY_ERROR,
    JSONRPC_SEVERITY_MAX
};

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

enum jsonrpc_events {
    JSONRPC_EVENT_MPD_CONNECTED = 0,
    JSONRPC_EVENT_MPD_DISCONNECTED,
    JSONRPC_EVENT_NOTIFY,
    JSONRPC_EVENT_UPDATE_ALBUM_CACHE,
    JSONRPC_EVENT_UPDATE_DATABASE,
    JSONRPC_EVENT_UPDATE_FINISHED,
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
    JSONRPC_EVENT_MAX
};

typedef bool (*iterate_callback) (sds, sds, int, validate_callback, void *, sds *);

void send_jsonrpc_notify(enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *partition, const char *message);
void send_jsonrpc_event(enum jsonrpc_events event, const char *partition);
sds jsonrpc_event(sds buffer, enum jsonrpc_events event);
sds jsonrpc_notify(sds buffer, enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *message);
sds jsonrpc_notify_phrase(sds buffer, enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *message, int count, ...);
sds jsonrpc_notify_start(sds buffer, enum jsonrpc_events event);

sds jsonrpc_respond_start(sds buffer, enum mympd_cmd_ids cmd_id, long request_id);
sds jsonrpc_end(sds buffer);
sds jsonrpc_respond_ok(sds buffer, enum mympd_cmd_ids cmd_id, long request_id, enum jsonrpc_facilities facility);
sds jsonrpc_respond_message(sds buffer, enum mympd_cmd_ids cmd_id, long request_id,
        enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *message);
sds jsonrpc_respond_message_phrase(sds buffer, enum mympd_cmd_ids cmd_id, long request_id,
        enum jsonrpc_facilities facility, enum jsonrpc_severities severity, const char *message, int count, ...);

sds tojson_raw(sds buffer, const char *key, const char *value, bool comma);
sds tojson_sds(sds buffer, const char *key, sds value, bool comma);
sds tojson_char(sds buffer, const char *key, const char *value, bool comma);
sds tojson_char_len(sds buffer, const char *key, const char *value, size_t len, bool comma);
sds tojson_bool(sds buffer, const char *key, bool value, bool comma);
sds tojson_int(sds buffer, const char *key, int value, bool comma);
sds tojson_uint(sds buffer, const char *key, unsigned value, bool comma);
sds tojson_long(sds buffer, const char *key, long value, bool comma);
sds tojson_llong(sds buffer, const char *key, long long value, bool comma);
sds tojson_ulong(sds buffer, const char *key, unsigned long value, bool comma);
sds tojson_ullong(sds buffer, const char *key, unsigned long long value, bool comma);
sds tojson_double(sds buffer, const char *key, double value, bool comma);

bool json_get_bool(sds s, const char *path, bool *result, sds *error);
bool json_get_int_max(sds s, const char *path, int *result, sds *error);
bool json_get_int(sds s, const char *path, int min, int max, int *result, sds *error);
bool json_get_long_max(sds s, const char *path, long *result, sds *error);
bool json_get_long(sds s, const char *path, long min, long max, long *result, sds *error);
bool json_get_llong_max(sds s, const char *path, long long *result, sds *error);
bool json_get_llong(sds s, const char *path, long long min, long long max, long long *result, sds *error);
bool json_get_uint_max(sds s, const char *path, unsigned *result, sds *error);
bool json_get_uint(sds s, const char *path, unsigned min, unsigned max, unsigned *result, sds *error);
bool json_get_string_max(sds s, const char *path, sds *result, validate_callback vcb, sds *error);
bool json_get_string(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb, sds *error);
bool json_get_string_cmp(sds s, const char *path, size_t min, size_t max, const char *cmp, sds *result, sds *error);
bool json_get_array_string(sds s, const char *path, struct t_list *l, validate_callback vcb, int max_elements, sds *error);
bool json_get_object_string(sds s, const char *path, struct t_list *l, validate_callback vcb, int max_elements, sds *error);
bool json_iterate_object(sds s, const char *path, iterate_callback icb, void *icb_userdata, validate_callback vcb, int max_elements, sds *error);
bool json_get_tags(sds s, const char *path, struct t_tags *tags, int max_elements, sds *error);
bool json_find_key(sds s, const char *path);

const char *get_mjson_toktype_name(int vtype);
sds list_to_json_array(sds s, struct t_list *l);
sds json_get_cols_as_string(sds s, sds cols, bool *rc);

#endif
