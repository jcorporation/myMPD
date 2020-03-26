/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __UTILITY_H__
#define __UTILITY_H__
void send_jsonrpc_notify_error(const char *message);
sds jsonrpc_start_notify(sds buffer, const char *method);
sds jsonrpc_end_notify(sds buffer);
sds jsonrpc_notify(sds buffer, const char *method);
sds jsonrpc_start_result(sds buffer, const char *method, int id);
sds jsonrpc_end_result(sds buffer);
sds jsonrpc_respond_ok(sds buffer, const char *method, int id);
sds jsonrpc_respond_message(sds buffer, const char *method, int id, const char *message, bool error);
sds jsonrpc_start_phrase(sds buffer, const char *method, int id, const char *message, bool error);
sds jsonrpc_start_phrase_notify(sds buffer, const char *message, bool error);
sds jsonrpc_end_phrase(sds buffer);
sds tojson_char(sds buffer, const char *key, const char *value, bool comma);
sds tojson_char_len(sds buffer, const char *key, const char *value, size_t len, bool comma);
sds tojson_bool(sds buffer, const char *key, bool value, bool comma);
sds tojson_long(sds buffer, const char *key, long value, bool comma);
sds tojson_float(sds buffer, const char *key, float value, bool comma);
int testdir(const char *name, const char *dirname, bool create);
int randrange(int n);
bool validate_string(const char *data);
bool validate_string_not_empty(const char *data);
bool validate_string_not_dir(const char *data);
bool validate_songuri(const char *data);
int replacechar(char *str, const char orig, const char rep);
int uri_to_filename(char *str);
bool validate_uri(const char *data);
sds find_image_file(sds basefilename);
sds get_mime_type_by_ext(const char *filename);
sds get_ext_by_mime_type(const char *mime_type);
sds get_mime_type_by_magic(const char *filename);
sds get_mime_type_by_magic_stream(sds stream);
bool write_covercache_file(t_config *config, const char *uri, const char *mime_type, sds binary);
bool strtobool(const char *value);
int strip_extension(char *s);
void ws_notify(sds message);

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
