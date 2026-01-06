/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief HTTP response handler
 */

#ifndef MYMPD_WEB_SERVER_RESPONSE_H
#define MYMPD_WEB_SERVER_RESPONSE_H

#include "dist/mongoose/mongoose.h"
#include "src/lib/json/json_rpc.h"

void webserver_send_raw_response(struct mg_mgr *mgr, struct t_work_response *response);
void webserver_send_redirect(struct mg_mgr *mgr, struct t_work_response *response);
void webserver_send_api_response(struct mg_mgr *mgr, struct t_work_response *response);
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

#ifdef MYMPD_EMBEDDED_ASSETS
    bool webserver_serve_embedded_files(struct mg_connection *nc, sds uri);
#endif

#endif
