/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_UTILITY_H
#define MYMPD_WEB_SERVER_UTILITY_H

#include "../../dist/mongoose/mongoose.h"
#include "../../dist/sds/sds.h"
#include "../lib/list.h"
#include "../lib/mympd_configuration.h"

#include <stdbool.h>

//struct for mg_mgr userdata
struct t_mg_user_data {
    struct t_config *config; //pointer to mympd config
    sds browse_directory;
    sds music_directory;
    sds *coverimage_names;
    int coverimage_names_len;
    sds *thumbnail_names;
    int thumbnail_names_len;
    bool feat_mpd_albumart;
    bool publish_playlists;
    bool publish_music;
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
void webserver_send_header_found(struct mg_connection *nc, const char *location);
void webserver_send_data(struct mg_connection *nc, const char *data, size_t len, const char *headers);
void webserver_handle_connection_close(struct mg_connection *nc);
struct mg_str mg_str_strip_parent(struct mg_str *path, int count);
void mg_user_data_free(struct t_mg_user_data *mg_user_data);
#endif
