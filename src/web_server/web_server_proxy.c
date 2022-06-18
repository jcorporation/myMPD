/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "web_server_proxy.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"
#include "web_server_utility.h"

//private definitions
static const char *allowed_proxy_hosts[] = {
    "jcorporation.github.io",
    "musicbrainz.org",
    "listenbrainz.org",
    NULL
};

//public functions
bool is_allowed_proxy_uri(const char *uri) {
    struct mg_str host = mg_url_host(uri);
    const char **p = NULL;
    for (p = allowed_proxy_hosts; *p != NULL; p++) {
        if (mg_vcmp(&host, *p) == 0) {
            MYMPD_LOG_DEBUG("Host \"%.*s\" is on whitelist", (int)host.len, host.ptr);
            return true;
        }
    }
    MYMPD_LOG_WARN("Host \"%.*s\" is not on whitelist", (int)host.len, host.ptr);
    return false;
}

void free_backend_nc_data(struct backend_nc_data_t *data) {
    FREE_SDS(data->uri);
    data->frontend_nc = NULL;
}

void handle_backend_close(struct mg_connection *nc, struct backend_nc_data_t *backend_nc_data) {
    MYMPD_LOG_INFO("Backend tcp connection \"%lu\" closed", nc->id);
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    mg_user_data->connection_count--;
    if (backend_nc_data->frontend_nc != NULL) {
        //remove backend connection pointer from frontend connection
        backend_nc_data->frontend_nc->fn_data = NULL;
        //close frontend connection
        backend_nc_data->frontend_nc->is_draining = 1;
    }
    //free backend_nc_data
    free_backend_nc_data(backend_nc_data);
    FREE_PTR(nc->fn_data);
}

void send_backend_request(struct mg_connection *nc, void *fn_data) {
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct backend_nc_data_t *backend_nc_data = (struct backend_nc_data_t *)fn_data;
    mg_user_data->connection_count++;
    struct mg_str host = mg_url_host(backend_nc_data->uri);
    MYMPD_LOG_INFO("Backend connection \"%lu\" established, host \"%.*s\"", nc->id, (int)host.len, host.ptr);
    if (mg_url_is_ssl(backend_nc_data->uri)) {
        struct mg_tls_opts tls_opts = {
            .srvname = host
        };
        mg_tls_init(nc, &tls_opts);
    }
    mg_printf(nc, "GET %s HTTP/1.1\r\n"
        "Host: %.*s\r\n"
        "User-Agent: myMPD/"MYMPD_VERSION"\r\n"
        "Connection: close\r\n"
        "\r\n",
        mg_url_uri(backend_nc_data->uri),
        (int)host.len, host.ptr);
    MYMPD_LOG_DEBUG("Sending GET %s HTTP/1.1 to backend connection \"%lu\"", mg_url_uri(backend_nc_data->uri), nc->id);
}

struct mg_connection *create_backend_connection(struct mg_connection *nc, struct mg_connection *backend_nc, sds uri, mg_event_handler_t fn) {
    if (backend_nc == NULL) {
        MYMPD_LOG_INFO("Creating new http backend connection to \"%s\"", uri);
        struct backend_nc_data_t *backend_nc_data = malloc(sizeof(struct backend_nc_data_t));
        backend_nc_data->uri = sdsdup(uri);
        backend_nc_data->frontend_nc = nc;
        backend_nc = mg_http_connect(nc->mgr, uri, fn, backend_nc_data);
        if (backend_nc == NULL) {
            //no backend connection, close frontend connection
            MYMPD_LOG_WARN("Can not create http backend connection");
            nc->is_closing = 1;
            //free backend_nc_data
            free_backend_nc_data(backend_nc_data);
            FREE_PTR(backend_nc_data);
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
        backend_nc->label[3] = nc->label[3];
        MYMPD_LOG_INFO("Forwarding client connection \"%lu\" to http backend connection \"%lu\"", nc->id, backend_nc->id);
    }
    return backend_nc;
}

void forward_backend_to_frontend(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    (void) ev_data;
    struct backend_nc_data_t *backend_nc_data = (struct backend_nc_data_t *)fn_data;
    switch(ev) {
        case MG_EV_CONNECT: {
            send_backend_request(nc, fn_data);
            break;
        }
        case MG_EV_ERROR:
            MYMPD_LOG_ERROR("HTTP connection \"%lu\" failed", nc->id);
            break;
        case MG_EV_READ:
            if (backend_nc_data->frontend_nc != NULL) {
                mg_send(backend_nc_data->frontend_nc, nc->recv.buf, nc->recv.len);
            }
            mg_iobuf_del(&nc->recv, 0, nc->recv.len);
            break;
        case MG_EV_CLOSE: {
            handle_backend_close(nc, backend_nc_data);
            break;
        }
    }
}
