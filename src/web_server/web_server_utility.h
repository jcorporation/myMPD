/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_UTILITY_H
#define MYMPD_WEB_SERVER_UTILITY_H

#include "../../dist/mongoose/mongoose.h"
#include "../../dist/sds/sds.h"
#include "../lib/list.h"
#include "../lib/mympd_configuration.h"

#include <stdbool.h>

#define EXTRA_HEADERS_MISC "X-Content-Type-Options: nosniff\r\n"\
    "X-XSS-Protection: 1; mode=block\r\n"\
    "X-Frame-Options: deny\r\n"

#define EXTRA_HEADERS_UNSAFE "Content-Security-Policy: default-src 'none'; "\
    "style-src 'self' 'unsafe-inline'; font-src 'self'; script-src 'self' 'unsafe-inline'; img-src 'self' data:; "\
    "connect-src 'self' ws: wss:; manifest-src 'self'; "\
    "media-src 'self'; frame-ancestors *; base-uri 'none';\r\n"\
    EXTRA_HEADERS_MISC

#define EXTRA_HEADERS_SAFE "Content-Security-Policy: default-src 'none'; "\
    "style-src 'self'; font-src 'self'; script-src 'self'; img-src 'self' data:; "\
    "connect-src 'self' ws: wss:; manifest-src 'self'; "\
    "media-src 'self'; frame-ancestors *; base-uri 'none'; "\
    "require-trusted-types-for 'script'\r\n"\
    EXTRA_HEADERS_MISC

#define EXTRA_HEADERS_CACHE "Cache-Control: max-age=604800\r\n"

#define EXTRA_HEADERS_SAFE_CACHE EXTRA_HEADERS_SAFE\
    EXTRA_HEADERS_CACHE

#define DIRECTORY_LISTING_CSS "h1{top:0;font-size:inherit;font-weight:inherit}address{bottom:0;font-style:normal}"\
    "h1,address{background-color:#343a40;color:#f8f9fa;padding:1rem;position:fixed;"\
    "box-sizing:border-box;width:100%;margin-top:0}body{margin:5rem 0;background-color:#f7f7f7;"\
    "color:#212529;font-family:sans-serif;font-size:1rem;font-weight:400;line-height:1.5}"\
    "table{border-collapse:collapse;margin:1rem}th{border-bottom:2px solid #dee2e6;"\
    "border-top:1px solid #dee2e6;text-align:left;padding:.3rem;font-family:inherit}"\
    "td{text-align:left;padding:.3rem;font-family:inherit;border-bottom:1px solid #dee2e6}"\
    "td:last-child{text-align:right}a,a:visited,a:active{color:#212529;text-decoration:none}"\
    "a:hover{text-decoration:underline}"

#define EXTRA_MIME_TYPES "avif=image/avif,flac=audio/flac,oga=audio/ogg,ogg=audio/ogg,"\
    "opus=audio/ogg,spx=audio/ogg,pem=application/x-x509-ca-cert,woff2=font/woff2"

//struct for mg_mgr userdata
struct t_mg_user_data {
    struct t_config *config; //pointer to mympd config
    sds browse_document_root;
    sds pics_document_root;
    sds music_directory;
    sds smartpls_document_root;
    sds playlist_directory;
    sds *coverimage_names;
    int coverimage_names_len;
    bool feat_library;
    bool feat_mpd_albumart;
    int connection_count;
    sds stream_uri;
    bool covercache;
    struct t_list session_list;
};

#ifdef EMBEDDED_ASSETS
bool webserver_serve_embedded_files(struct mg_connection *nc, sds uri);
#endif
void webserver_manage_emptydir(sds workdir, bool pics, bool smartplaylists, bool music, bool playlists);
sds *webserver_split_coverimage_names(sds coverimage_name, sds *coverimage_names, int *count);
sds webserver_find_image_file(sds basefilename);
void webserver_send_error(struct mg_connection *nc, int code, const char *msg);
void webserver_serve_na_image(struct mg_connection *nc, struct mg_http_message *hm);
void webserver_serve_stream_image(struct mg_connection *nc, struct mg_http_message *hm);
void webserver_serve_asset_image(struct mg_connection *nc, struct mg_http_message *hm, const char *name);
void webserver_populate_dummy_hm(struct mg_connection *nc, struct mg_http_message *hm);
void webserver_send_header_ok(struct mg_connection *nc, size_t len, const char *headers);
void webserver_send_header_redirect(struct mg_connection *nc, const char *location);
void webserver_send_data(struct mg_connection *nc, const char *data, size_t len, const char *headers);
void webserver_handle_connection_close(struct mg_connection *nc);
struct mg_str mg_str_strip_parent(struct mg_str *path, int count);
void mg_user_data_free(struct t_mg_user_data *mg_user_data);
#endif
