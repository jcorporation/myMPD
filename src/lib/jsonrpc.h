/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_JSONRPC_H
#define MYMPD_JSONRPC_H

#include "../../dist/sds/sds.h"
#include "list.h"
#include "mympd_state.h"
#include "validate.h"

#include <stdbool.h>

typedef bool (*iterate_callback) (sds, sds, int, validate_callback, void *, sds *);

void send_jsonrpc_notify(const char *facility, const char *severity, const char *message);
void send_jsonrpc_event(const char *event);

sds jsonrpc_event(sds buffer, const char *event);
sds jsonrpc_notify(sds buffer, const char *facility, const char *severity, const char *message);
sds jsonrpc_notify_phrase(sds buffer, const char *facility, const char *severity, const char *message, int count, ...);
sds jsonrpc_notify_start(sds buffer, const char *method);
sds jsonrpc_result_start(sds buffer, const char *method, long id);
sds jsonrpc_result_end(sds buffer);
sds jsonrpc_respond_ok(sds buffer, const char *method, long id, const char *facility);
sds jsonrpc_respond_message(sds buffer, const char *method, long id,
        bool error, const char *facility, const char *severity, const char *message);
sds jsonrpc_respond_message_phrase(sds buffer, const char *method, long id,
        bool error, const char *facility, const char *severity, const char *message, int count, ...);
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
