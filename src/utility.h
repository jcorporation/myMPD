/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdbool.h>

#include "../dist/src/sds/sds.h"

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
int testdir(const char *name, const char *dirname, bool create);
bool validate_string(const char *data);
bool validate_string_not_empty(const char *data);
bool validate_string_not_dir(const char *data);
bool validate_songuri(const char *data);
int replacechar(char *str, const char orig, const char rep);
int uri_to_filename(char *str);
bool validate_uri(const char *data);
sds find_image_file(sds basefilename);
sds get_extension_from_filename(const char *filename);
sds get_mime_type_by_ext(const char *filename);
sds get_ext_by_mime_type(const char *mime_type);
sds get_mime_type_by_magic(const char *filename);
sds get_mime_type_by_magic_stream(sds stream);
bool strtobool(const char *value);
int strip_extension(char *s);
void strip_slash(sds s);

void my_usleep(time_t usec);
unsigned long substractUnsigned(unsigned long num1, unsigned long num2);
char *basename_uri(char *uri);
bool is_streamuri(const char *uri);
int unsigned_to_int(unsigned x);

#define FREE_PTR(PTR) do { \
    if (PTR != NULL) \
        free(PTR); \
    PTR = NULL; \
} while (0)

struct mime_type_entry {
    const char *extension;
    const char *mime_type;
};

struct magic_byte_entry {
    const char *magic_bytes;
    const char *mime_type;
};

#endif
