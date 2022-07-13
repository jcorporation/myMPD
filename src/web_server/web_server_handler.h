/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_HANDLER_H
#define MYMPD_WEB_SERVER_HANDLER_H

#include "../../dist/mongoose/mongoose.h"
#include "../lib/sds_extras.h"
#include "web_server_utility.h"

bool webserver_api_handler(struct mg_connection *nc, sds body, struct mg_str *auth_header,
        struct t_mg_user_data *mg_user_data, struct mg_connection *backend_nc);
bool webserver_script_api_handler(long long conn_id, sds body);
void webserver_browse_handler(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data);
void webserver_proxy_handler(struct mg_connection *nc, struct mg_http_message *hm,
        struct mg_connection *backend_nc);
void webserver_serverinfo_handler(struct mg_connection *nc);

#ifdef ENABLE_SSL
void webserver_ca_handler(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data, struct t_config *config);
#endif
#endif
