/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
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
    sds placeholder_booklet;     //!< name of custom booklet image
    sds placeholder_mympd;       //!< name of custom mympd image
    sds placeholder_na;          //!< name of custom not available image
    sds placeholder_stream;      //!< name of custom stream image
    sds placeholder_playlist;    //!< name of custom playlist image
    sds placeholder_smartpls;    //!< name of custom smart playlist image
    sds placeholder_folder;      //!< name of custom folder image
    bool mympd_api_started;      //!< true if the mympd_api thread is ready, else false
    sds cert_content;            //!< the server certificate
    sds key_content;             //!< the server key
    struct mg_str cert;          //!< pointer to ssl cert_content
    struct mg_str key;           //!< pointer to ssl key_content
};

/**
 * Struct for http frontend connection user data
 */
struct t_frontend_nc_data {
    struct mg_connection *backend_nc;  //!< pointer to backend connection
    //for websocket connections only
    sds partition;                     //!< partition
    unsigned id;                       //!< jsonrpc id (client id)
    time_t last_ws_ping;               //!< last websocket ping from client
};

enum placeholder_types {
    PLACEHOLDER_NA,
    PLACEHOLDER_STREAM,
    PLACEHOLDER_MYMPD,
    PLACEHOLDER_BOOKLET,
    PLACEHOLDER_PLAYLIST,
    PLACEHOLDER_SMARTPLS,
    PLACEHOLDER_FOLDER
};

#ifdef MYMPD_EMBEDDED_ASSETS
bool webserver_serve_embedded_files(struct mg_connection *nc, sds uri);
#endif
sds print_ip(sds s, struct mg_addr *addr);
bool get_partition_from_uri(struct mg_connection *nc, struct mg_http_message *hm, struct t_frontend_nc_data *frontend_nc_data);
bool check_covercache(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data, sds uri_decoded, int offset);
sds webserver_find_image_file(sds basefilename);
bool find_image_in_folder(sds *coverfile, sds music_directory, sds path, sds *names, int names_len);
void webserver_send_error(struct mg_connection *nc, int code, const char *msg);
void webserver_serve_file(struct mg_connection *nc, struct mg_http_message *hm, const char *path, const char *file);
void webserver_serve_placeholder_image(struct mg_connection *nc, enum placeholder_types placeholder_type);
void webserver_send_header_ok(struct mg_connection *nc, size_t len, const char *headers);
void webserver_send_header_redirect(struct mg_connection *nc, const char *location);
void webserver_send_header_found(struct mg_connection *nc, const char *location);
void webserver_send_cors_reply(struct mg_connection *nc);
void webserver_send_data(struct mg_connection *nc, const char *data, size_t len, const char *headers);
void webserver_handle_connection_close(struct mg_connection *nc);
void *mg_user_data_free(struct t_mg_user_data *mg_user_data);
#endif
