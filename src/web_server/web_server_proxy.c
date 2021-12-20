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

void free_backend_nc_data(struct backend_nc_data_t *data) {
    FREE_SDS(data->uri);
    data->frontend_nc = NULL;
}

struct mg_connection *create_http_backend_connection(struct mg_connection *nc, struct mg_connection *backend_nc, sds uri, mg_event_handler_t fn) {
    if (backend_nc == NULL) {
        MYMPD_LOG_INFO("Creating new http backend connection to \"%s\"", uri);
        struct backend_nc_data_t *backend_nc_data = (struct backend_nc_data_t *)malloc(sizeof(struct backend_nc_data_t));
        backend_nc_data->uri = sdsdup(uri);
        backend_nc_data->frontend_nc = nc;
        backend_nc = mg_http_connect(nc->mgr, uri, fn, backend_nc_data);
        if (backend_nc == NULL) {
            //no backend connection, close frontend connection
            MYMPD_LOG_WARN("Can not create http backend connection");
            nc->is_closing = 1;
            //free backend_nc_data
            free_backend_nc_data(backend_nc_data);
            free(backend_nc_data);
        }
        else {
            //save backend connection pointer in frontend connection fn_data
            nc->fn_data = backend_nc;
        }
    }
    if (backend_nc != NULL) {
        //set labels
        backend_nc->label[0] = 'B';
        backend_nc->label[1] = nc->label[1];
        backend_nc->label[2] = nc->label[2];
        MYMPD_LOG_INFO("Forwarding client connection \"%lu\" to http backend connection \"%lu\"", nc->id, backend_nc->id);
    }
    return backend_nc;
}

struct mg_connection *create_tcp_backend_connection(struct mg_connection *nc, struct mg_connection *backend_nc, sds uri, mg_event_handler_t fn) {
    if (backend_nc == NULL) {
        MYMPD_LOG_INFO("Creating new tcp backend connection to \"%s\"", uri);
        struct backend_nc_data_t *backend_nc_data = (struct backend_nc_data_t *)malloc(sizeof(struct backend_nc_data_t));
        backend_nc_data->uri = sdsdup(uri);
        backend_nc_data->frontend_nc = nc;
        backend_nc = mg_connect(nc->mgr, uri, fn, backend_nc_data);
        if (backend_nc == NULL) {
            //no backend connection, close frontend connection
            MYMPD_LOG_WARN("Can not create tcp backend connection");
            nc->is_closing = 1;
            //free backend_nc_data
            free_backend_nc_data(backend_nc_data);
            free(backend_nc_data);
        }
        else {
            //save backend connection pointer in frontend connection fn_data
            nc->fn_data = backend_nc;
        }
    }
    if (backend_nc != NULL) {
        //set labels
        backend_nc->label[0] = 'B';
        backend_nc->label[1] = nc->label[1];
        backend_nc->label[2] = nc->label[2];
        MYMPD_LOG_INFO("Forwarding client connection \"%lu\" to tcp backend connection \"%lu\"", nc->id, backend_nc->id);
    }
    return backend_nc;
}

void forward_tcp_backend_to_frontend(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct backend_nc_data_t *backend_nc_data = (struct backend_nc_data_t *)fn_data;
    switch(ev) {
        case MG_EV_CONNECT:
            mg_user_data->connection_count++;
            struct mg_str host = mg_url_host(backend_nc_data->uri);
            mg_printf(nc, "GET %s HTTP/1.1\r\n"
                "Host: %.*s\r\n"
                "User-Agent: myMPD/"MYMPD_VERSION"\r\n"
                "\r\n",
                mg_url_uri(backend_nc_data->uri),
                host.len, host.ptr);
            break;
        case MG_EV_READ:
            //forward incoming data from backend to frontend
            mg_send(backend_nc_data->frontend_nc, nc->recv.buf, nc->recv.len);
            mg_iobuf_del(&nc->recv, 0, nc->recv.len);
            break;
        case MG_EV_CLOSE: {
            MYMPD_LOG_INFO("Backend HTTP connection %lu closed", nc->id);
            mg_user_data->connection_count--;
            if (backend_nc_data->frontend_nc != NULL) {
                //remove backend connection pointer from frontend connection
                backend_nc_data->frontend_nc->fn_data = NULL;
                //close frontend connection
                backend_nc_data->frontend_nc->is_closing = 1;
                //free backend_nc_data
                free_backend_nc_data(backend_nc_data);
                free(fn_data);
            }
            break;
        }
    }
	(void) ev_data;
}
