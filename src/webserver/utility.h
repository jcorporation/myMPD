/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver utility functions
 */

#ifndef MYMPD_WEB_SERVER_UTILITY_H
#define MYMPD_WEB_SERVER_UTILITY_H

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/api.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/list.h"
#include "src/webserver/mg_user_data.h"

#include <stdbool.h>

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

#ifdef MYMPD_EMBEDDED_ASSETS
bool webserver_serve_embedded_files(struct mg_connection *nc, sds uri);
#endif
sds get_uri_param(struct mg_str *query, const char *name);
sds print_ip(sds s, struct mg_addr *addr);
bool get_partition_from_uri(struct mg_connection *nc, struct mg_http_message *hm, struct t_frontend_nc_data *frontend_nc_data);
bool check_imagescache(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data, const char *type, sds uri_decoded, int offset);
sds webserver_find_image_file(sds basefilename);
bool find_image_in_folder(sds *coverfile, sds music_directory, sds path, sds *names, int names_len);
void webserver_send_error(struct mg_connection *nc, int code, const char *msg);
void webserver_serve_file(struct mg_connection *nc, struct mg_http_message *hm,
        const char *headers, const char *file);
void webserver_send_header_ok(struct mg_connection *nc, size_t len, const char *headers);
void webserver_send_header_redirect(struct mg_connection *nc, const char *location, const char *headers);
void webserver_send_header_found(struct mg_connection *nc, const char *location, const char *headers);
void webserver_send_cors_reply(struct mg_connection *nc);
void webserver_send_data(struct mg_connection *nc, const char *data, size_t len, const char *headers);
void webserver_send_raw(struct mg_connection *nc, const char *data, size_t len);
void webserver_send_jsonrpc_response(struct mg_connection *nc,
        enum mympd_cmd_ids cmd_id, unsigned request_id,
        enum jsonrpc_facilities facility, enum jsonrpc_severities severity,
        const char *message);
void webserver_handle_connection_close(struct mg_connection *nc);
struct t_list *webserver_parse_arguments(struct mg_http_message *hm);

#endif
