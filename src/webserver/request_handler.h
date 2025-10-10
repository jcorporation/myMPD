/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief HTTP request handler
 */

#ifndef MYMPD_WEB_SERVER_REQUEST_HANDLER_H
#define MYMPD_WEB_SERVER_REQUEST_HANDLER_H

#include "dist/mongoose/mongoose.h"
#include "src/webserver/mg_user_data.h"

bool request_handler_api(struct mg_connection *nc, sds body, struct mg_str *auth_header,
        struct t_mg_user_data *mg_user_data);
bool request_handler_script_api(struct mg_connection *nc, sds body);
void request_handler_browse(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data);
void request_handler_proxy(struct mg_connection *nc, struct mg_http_message *hm,
        struct mg_connection *backend_nc);
void request_handler_proxy_covercache(struct mg_connection *nc, struct mg_http_message *hm,
        struct mg_connection *backend_nc);
void request_handler_serverinfo(struct mg_connection *nc);
void request_handler_ca(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data);
void request_handler_extm3u(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data);

#endif
