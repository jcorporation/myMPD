/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_PROXY_H
#define MYMPD_WEB_SERVER_PROXY_H

#include "../../dist/mongoose/mongoose.h"
#include "../../dist/sds/sds.h"
#include "../lib/api.h"

struct backend_nc_data_t {
    struct mg_connection *frontend_nc;
    sds uri;
    enum mympd_cmd_ids cmd_id;
};

bool is_allowed_proxy_uri(const char *uri);
void free_backend_nc_data(struct backend_nc_data_t *data);
struct mg_connection *create_http_backend_connection(struct mg_connection *nc, struct mg_connection *backend_nc,
        sds uri, mg_event_handler_t fn);
struct mg_connection *create_tcp_backend_connection(struct mg_connection *nc, struct mg_connection *backend_nc,
        sds uri, mg_event_handler_t fn);
void forward_tcp_backend_to_frontend(struct mg_connection *nc, int ev, void *ev_data, void *fn_data);
void forward_http_backend_to_frontend(struct mg_connection *nc, int ev, void *ev_data, void *fn_data);
#endif
