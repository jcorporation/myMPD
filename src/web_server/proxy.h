/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief HTTP proxy functions
 */

#ifndef MYMPD_WEB_SERVER_PROXY_H
#define MYMPD_WEB_SERVER_PROXY_H

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/api.h"

#include <stdbool.h>

/**
 * Struct for http proxy backend connections
 */
struct t_backend_nc_data {
    struct mg_connection *frontend_nc;  //!< pointer to frontend connection
    sds uri;                            //!< uri to connect the backend connection
    enum mympd_cmd_ids cmd_id;          //!< jsonrpc method of the frontend connection
};

bool is_allowed_proxy_uri(const char *uri);
void send_backend_request(struct mg_connection *nc);
void handle_backend_close(struct mg_connection *nc);
void free_backend_nc_data(struct t_backend_nc_data *data);
struct mg_connection *create_backend_connection(struct mg_connection *nc, struct mg_connection *backend_nc,
        sds uri, mg_event_handler_t fn, bool stream);
void forward_backend_to_frontend_stream(struct mg_connection *nc, int ev, void *ev_data);
void forward_backend_to_frontend_covercache(struct mg_connection *nc, int ev, void *ev_data);
#endif
