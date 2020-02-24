/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __WEB_SERVER_UTILITY_H__
#define __WEB_SERVER_UTILITY_H__

#define EXTRA_HEADERS_DIR "Content-Security-Policy: default-src 'none'; "\
                          "style-src 'self' 'unsafe-inline'; font-src 'self'; script-src 'self' 'unsafe-inline'; img-src 'self' data:; "\
                          "connect-src 'self' ws: wss:; manifest-src 'self'; "\
                          "media-src *; frame-ancestors 'none'; base-uri 'none';\r\n"\
                          "X-Content-Type-Options: nosniff\r\n"\
                          "X-XSS-Protection: 1; mode=block\r\n"\
                          "X-Frame-Options: deny"

#define EXTRA_HEADERS "Content-Security-Policy: default-src 'none'; "\
                      "style-src 'self'; font-src 'self'; script-src 'self'; img-src 'self' data:; "\
                      "connect-src 'self' ws: wss:; manifest-src 'self'; "\
                      "media-src *; frame-ancestors 'none'; base-uri 'none';\r\n"\
                      "X-Content-Type-Options: nosniff\r\n"\
                      "X-XSS-Protection: 1; mode=block\r\n"\
                      "X-Frame-Options: deny"

#define EXTRA_HEADERS_CACHE "Cache-Control: max-age=604800"

#define CUSTOM_MIME_TYPES ".html=text/html; charset=utf-8,.manifest=application/manifest+json,.woff2=application/font-woff"

typedef struct t_mg_user_data {
    void *config; //pointer to mympd config
    sds browse_document_root;
    sds music_directory;
    sds playlist_directory;
    sds rewrite_patterns;
    sds *coverimage_names;
    int coverimage_names_len;
    bool feat_library;
    bool feat_mpd_albumart;
    int conn_id;
} t_mg_user_data;

#ifndef DEBUG
bool serve_embedded_files(struct mg_connection *nc, sds uri, struct http_message *hm);
#endif
bool rm_mk_dir(sds dir_name, bool create);
void manage_emptydir(sds varlibdir, bool pics, bool smartplaylists, bool music, bool playlists);
sds *split_coverimage_names(const char *coverimage_name, sds *coverimage_names, int *count);
void send_error(struct mg_connection *nc, int code, const char *msg);
void serve_na_image(struct mg_connection *nc, struct http_message *hm);
void serve_stream_image(struct mg_connection *nc, struct http_message *hm);
void serve_asset_image(struct mg_connection *nc, struct http_message *hm, const char *name);
void populate_dummy_hm(struct http_message *hm);
#endif
