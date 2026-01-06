/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver utility functions
 */

#ifndef MYMPD_WEB_SERVER_UTILITY_H
#define MYMPD_WEB_SERVER_UTILITY_H

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
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

bool webserver_enforce_acl(struct mg_connection *nc, sds acl);
bool webserver_enforce_conn_limit(struct mg_connection *nc, int connection_count);
struct mg_connection *get_nc_by_id(struct mg_mgr *mgr, unsigned long id);
sds get_uri_param(struct mg_str *query, const char *name);
sds print_ip(sds s, struct mg_addr *addr);
bool get_partition_from_uri(struct mg_connection *nc, struct mg_http_message *hm, struct t_frontend_nc_data *frontend_nc_data);
bool check_imagescache(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data, const char *type, sds uri_decoded, int offset);
sds webserver_find_image_file(sds basefilename);
bool find_image_in_folder(sds *coverfile, sds music_directory, sds path, sds *names, int names_len);
void webserver_handle_connection_close(struct mg_connection *nc);
struct t_list *webserver_parse_arguments(struct mg_http_message *hm);

#endif
