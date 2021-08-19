/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_JSONRPC_H
#define MYMPD_JSONRPC_H

#include "../../dist/src/sds/sds.h"
#include "list.h"
#include "validate.h"

#include <stdbool.h>

void send_jsonrpc_notify(const char *facility, const char *severity, const char *message);
void send_jsonrpc_event(const char *event);
void ws_notify(sds message);
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
sds tojson_char(sds buffer, const char *key, const char *value, bool comma);
sds tojson_char_len(sds buffer, const char *key, const char *value, size_t len, bool comma);
sds tojson_bool(sds buffer, const char *key, bool value, bool comma);
sds tojson_long(sds buffer, const char *key, long long value, bool comma);
sds tojson_ulong(sds buffer, const char *key, unsigned long value, bool comma);
sds tojson_double(sds buffer, const char *key, double value, bool comma);

bool json_get_bool(sds s, const char *path, bool *result);
bool json_get_int_max(sds s, const char *path, int *result);
bool json_get_int(sds s, const char *path, int min, int max, int *result);
bool json_get_uint_max(sds s, const char *path, unsigned *result);
bool json_get_uint(sds s, const char *path, unsigned min, unsigned max, unsigned *result);
bool json_get_string_max(sds s, const char *path, sds *result, validate_callback vcb);
bool json_get_string(sds s, const char *path, size_t min, size_t max, sds *result, validate_callback vcb);
bool json_get_string_cmp(sds s, const char *path, size_t min, size_t max, const char *cmp, sds *result);
bool json_get_array_string(sds s, const char *path, struct list *array, validate_callback vcb, int max_elements);

#endif
