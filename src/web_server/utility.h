/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_UTILITY_H
#define MYMPD_WEB_SERVER_UTILITY_H

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/config_def.h"
#include "src/lib/list.h"

#include <stdbool.h>

/**
 * Struct for mg_mgr userdata
 */
struct t_mg_user_data {
    struct t_config *config;     //!< Pointer to myMPD configuration
    sds browse_directory;        //!< document root
    sds music_directory;         //!< mpd music directory
    sds *coverimage_names;       //!< sds array of coverimage names
    int coverimage_names_len;    //!< length of coverimage_names array
    sds *thumbnail_names;        //!< sds array of coverimage thumbnail names
    int thumbnail_names_len;     //!< length of thumbnail_names array
    bool feat_albumart;          //!< feature flag for md albumart command
    bool publish_playlists;      //!< true if mpd playlist directory is configured
    bool publish_music;          //!< true if mpd music directory is accessible
    int connection_count;        //!< number of http connections
    struct t_list stream_uris;   //!< uri for the mpd stream reverse proxy
    struct t_list session_list;  //!< list of myMPD sessions (pin protection mode)
    sds custom_booklet_image;    //!< name of custom booklet image
    sds custom_mympd_image;      //!< name of custom mympd image
    sds custom_na_image;         //!< name of custom not available image
    sds custom_stream_image;     //!< name of custom stream image
    bool mympd_api_started;      //!< true if the mympd_api thread is ready, else false
};

/**
 * Struct for http frontend connection user data
 */
struct t_frontend_nc_data {
    struct mg_connection *backend_nc;  //!< pointer to backend connection
    //for websocket connections only
    sds partition;                     //!< partition
    long id;                           //!< jsonrpc id (client id)
};

#ifdef MYMPD_EMBEDDED_ASSETS
bool webserver_serve_embedded_files(struct mg_connection *nc, sds uri);
#endif
int mg_str_to_int(struct mg_str *str);
long mg_str_to_long(struct mg_str *str);
sds print_ip(sds s, struct mg_addr *addr);
bool get_partition_from_uri(struct mg_connection *nc, struct mg_http_message *hm, struct t_frontend_nc_data *frontend_nc_data);
bool check_covercache(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data, sds uri_decoded, int offset);
sds webserver_find_image_file(sds basefilename);
void webserver_send_error(struct mg_connection *nc, int code, const char *msg);
void webserver_serve_na_image(struct mg_connection *nc);
void webserver_serve_stream_image(struct mg_connection *nc);
void webserver_serve_mympd_image(struct mg_connection *nc);
void webserver_serve_booklet_image(struct mg_connection *nc);
void webserver_send_header_ok(struct mg_connection *nc, size_t len, const char *headers);
void webserver_send_header_redirect(struct mg_connection *nc, const char *location);
void webserver_send_header_found(struct mg_connection *nc, const char *location);
void webserver_send_cors_reply(struct mg_connection *nc);
void webserver_send_data(struct mg_connection *nc, const char *data, size_t len, const char *headers);
void webserver_handle_connection_close(struct mg_connection *nc);
void *mg_user_data_free(struct t_mg_user_data *mg_user_data);
#endif
