/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "global.h"
#include "web_server.h"
#include "mpd_client.h"
#include "../dist/src/mongoose/mongoose.h"

//non-api definitions
static unsigned long s_next_id = 1;
struct mg_mgr mgr;
static struct mg_serve_http_opts s_http_server_opts;

static int is_websocket(const struct mg_connection *nc);
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data);
static void ev_handler_redirect(struct mg_connection *nc_http, int ev, void *ev_data);
static void send_ws_notify(struct mg_mgr *mgr, struct work_result_t *response);
static void send_api_response(struct mg_mgr *mgr, struct work_result_t *response);

//api functions
bool web_server_init() {
    struct mg_connection *nc;
    struct mg_connection *nc_http;
    struct mg_bind_opts bind_opts;
    const char *err;
    
    mg_mgr_init(&mgr, NULL);

    if (config.ssl == true) {
        nc_http = mg_bind(&mgr, config.webport, ev_handler_redirect);
        if (nc_http == NULL) {
            printf("Error listening on port %s\n", config.webport);
            mg_mgr_free(&mgr);
            return false;
        }
        mg_set_protocol_http_websocket(nc_http);
        LOG_INFO() printf("Listening on http port %s (redirect only).\n", config.webport);

        memset(&bind_opts, 0, sizeof(bind_opts));
        bind_opts.ssl_cert = config.sslcert;
        bind_opts.ssl_key = config.sslkey;
        bind_opts.error_string = &err;
        nc = mg_bind_opt(&mgr, config.sslport, ev_handler, bind_opts);
        if (nc == NULL) {
            printf("Error listening on port %s: %s\n", config.sslport, err);
            mg_mgr_free(&mgr);
            return false;
        } 
        LOG_INFO() printf("Listening on ssl port %s\n", config.sslport);
    }
    else {
        nc = mg_bind(&mgr, config.webport, ev_handler);
        if (nc == NULL) {
            printf("Error listening on port %s\n", config.webport);
            mg_mgr_free(&mgr);
            return false;
        }
        LOG_INFO() printf("Listening on http port %s\n", config.webport);
    }
    
    mg_set_protocol_http_websocket(nc);
    s_http_server_opts.document_root = DOC_ROOT;
    s_http_server_opts.enable_directory_listing = "no";
    return true;
}

void web_server_free() {
    mg_mgr_free(&mgr);
}

void *web_server_thread() {
    while (s_signal_received == 0) {
        mg_mgr_poll(&mgr, 10);
        unsigned web_server_queue_length = tiny_queue_length(web_server_queue);
        if (web_server_queue_length > 0) {
            struct work_result_t *response = tiny_queue_shift(web_server_queue);
            if (response->conn_id == 0) {
                //Websocket notify from mpd idle
                send_ws_notify(&mgr, response);
            } 
            else {
                //api response
                send_api_response(&mgr, response);
            }
        }
    }
    mg_mgr_free(&mgr);
    return NULL;
}

//non-api functions
static int is_websocket(const struct mg_connection *nc) {
    return nc->flags & MG_F_IS_WEBSOCKET;
}

static void send_ws_notify(struct mg_mgr *mgr, struct work_result_t *response) {
    struct mg_connection *c;
    LOG_DEBUG() fprintf(stderr, "DEBUG: Got ws notify, broadcasting\n");
    
    for (c = mg_next(mgr, NULL); c != NULL; c = mg_next(mgr, c)) {
        if (!is_websocket(c))
            continue;
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, response->data, response->length);
    }
    free(response);
}

static void send_api_response(struct mg_mgr *mgr, struct work_result_t *response) {
    struct mg_connection *c;
    LOG_DEBUG() fprintf(stderr, "DEBUG: Got API response for connection %lu.\n", response->conn_id);
    
    for (c = mg_next(mgr, NULL); c != NULL; c = mg_next(mgr, c)) {
        if (c->user_data != NULL) {
            if ((unsigned long)c->user_data == response->conn_id) {
                LOG_DEBUG() fprintf(stderr, "DEBUG: Sending to connection %lu: %s\n", (unsigned long)c->user_data, response->data);
                mg_send_head(c, 200, response->length, "Content-Type: application/json");
                mg_printf(c, "%s", response->data);
            }
        }
    }
    free(response);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    (void) nc;
    (void) ev_data;
    
    switch(ev) {
        case MG_EV_ACCEPT: {
            nc->user_data = (void *)++s_next_id;
            LOG_DEBUG() fprintf(stderr, "DEBUG: New connection id %lu.\n", s_next_id);
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            LOG_VERBOSE() printf("New websocket request: %.*s\n", hm->uri.len, hm->uri.p);
            if (mg_vcmp(&hm->uri, "/ws") != 0) {
                printf("ERROR: Websocket request not to /ws, closing connection\n");
                mg_printf(nc, "%s", "HTTP/1.1 403 FORBIDDEN\r\n\r\n");
                nc->flags |= MG_F_SEND_AND_CLOSE;
            }
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
             LOG_VERBOSE() printf("New Websocket connection established.\n");
             char response[] = "{\"type\": \"welcome\", \"data\": {\"mympdVersion\": \"" MYMPD_VERSION "\"}}";
             mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, response, strlen(response));
             break;
        }
        case MG_EV_HTTP_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            LOG_VERBOSE() printf("HTTP request: %.*s\n", hm->uri.len, hm->uri.p);
            if (mg_vcmp(&hm->uri, "/api") == 0) {
                struct work_request_t *request = (struct work_request_t*)malloc(sizeof(struct work_request_t));
                request->conn_id = (unsigned long)nc->user_data;
                request->length = copy_string(request->data, hm->body.p, 1000, hm->body.len);
                tiny_queue_push(mpd_client_queue, request);
            }
            else {
                mg_serve_http(nc, hm, s_http_server_opts);
            }
            break;
        }
        case MG_EV_CLOSE: {
            if (nc->user_data) {
                LOG_VERBOSE() fprintf(stderr, "HTTP connection %lu closed.\n", (unsigned long)nc->user_data);
                nc->user_data = NULL;
            }
            else {
                LOG_VERBOSE() printf("HTTP connection closed.\n");
            }
            break;
        }
        default: {
            break;
        }
    }
}

static void ev_handler_redirect(struct mg_connection *nc_http, int ev, void *ev_data) {
    char *host;
    char *crap;
    char host_header[1024];
    switch(ev) {
        case MG_EV_HTTP_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            struct mg_str *host_hdr = mg_get_http_header(hm, "Host");
            snprintf(host_header, 1024, "%.*s", host_hdr->len, host_hdr->p);
            host = strtok_r(host_header, ":", &crap);
            char s_redirect[250];
            if (strcmp(config.sslport, "443") == 0)
                snprintf(s_redirect, 250, "https://%s/", host);
            else
                snprintf(s_redirect, 250, "https://%s:%s/", host, config.sslport);
            LOG_VERBOSE() printf("Redirecting to %s\n", s_redirect);
            mg_http_send_redirect(nc_http, 301, mg_mk_str(s_redirect), mg_mk_str(NULL));
            break;
        }
        default: {
            break;
        }
    }
}
