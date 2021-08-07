/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __JSONRPC_H__
#define __JSONRPC_H__

#include <stdbool.h>

#include "../../dist/src/sds/sds.h"

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
#endif
