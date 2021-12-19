/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "web_server_proxy.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "web_server_utility.h"

struct mg_connection *create_backend_connection(struct mg_connection *nc, struct mg_connection *backend_nc, sds uri, mg_event_handler_t fn) {
    if (backend_nc == NULL) {
        MYMPD_LOG_INFO("Creating new backend connection to \"%s\"", uri);
        backend_nc = mg_connect(nc->mgr, uri, fn, nc);
        if (backend_nc == NULL) {
            //no backend connection, close frontend connection
            MYMPD_LOG_WARN("Can not create backend connection");
            nc->is_closing = 1;
        }
        else {
            if (mg_url_is_ssl(uri)) {
                struct mg_tls_opts tls_opts = {
                    .srvname = mg_url_host(uri)
                };
                mg_tls_init(backend_nc, &tls_opts);
            }
            //save backend connection pointer in frontend connection fn_data
            nc->fn_data = backend_nc;
        }
    }
    if (backend_nc != NULL) {
        //set labels
        backend_nc->label[0] = 'B';
        backend_nc->label[1] = nc->label[1];
        backend_nc->label[2] = nc->label[2];
        MYMPD_LOG_INFO("Forwarding client connection \"%lu\" to backend connection \"%lu\"", nc->id, backend_nc->id);
    }
    return backend_nc;
}

void forward_backend_to_frontend(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    struct mg_connection *frontend_nc = fn_data;
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    switch(ev) {
        case MG_EV_ACCEPT:
            mg_user_data->connection_count++;
            break;
        case MG_EV_READ:
            //forward incoming data from backend to frontend
            mg_send(frontend_nc, nc->recv.buf, nc->recv.len);
            mg_iobuf_del(&nc->recv, 0, nc->recv.len);
            break;
        case MG_EV_CLOSE: {
            MYMPD_LOG_INFO("Backend HTTP connection %lu closed", nc->id);
            mg_user_data->connection_count--;
            if (frontend_nc != NULL) {
                //remove backend connection pointer from frontend connection
                frontend_nc->fn_data = NULL;
                //close frontend connection
                frontend_nc->is_closing = 1;
            }
            break;
        }
    }
	(void) ev_data;
}
