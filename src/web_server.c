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

#include <limits.h>
#include <stdbool.h>
#include <pthread.h>

#include "list.h"
#include "tiny_queue.h"
#include "global.h"
#include "web_server.h"
#include "mpd_client.h"
#include "../dist/src/mongoose/mongoose.h"
#include "../dist/src/frozen/frozen.h"

//private definitions
static int is_websocket(const struct mg_connection *nc);
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data);
static void ev_handler_redirect(struct mg_connection *nc_http, int ev, void *ev_data);
static void send_ws_notify(struct mg_mgr *mgr, t_work_result *response);
static void send_api_response(struct mg_mgr *mgr, t_work_result *response);
static bool handle_api(long conn_id, const char *request, int request_len);

typedef struct t_user_data {
    void *config; //pointer to mympd config
    long conn_id; 
} t_user_data;

//public functions
bool web_server_init(void *arg_mgr, t_config *config) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    struct mg_connection *nc_https;
    struct mg_connection *nc_http;
    struct mg_bind_opts bind_opts_https;
    struct mg_bind_opts bind_opts_http;
    const char *err_https;
    const char *err_http;

    t_user_data *user_data = (t_user_data*)malloc(sizeof(t_user_data));
    user_data->config = config;
    user_data->conn_id = 1;
    
    mg_mgr_init(mgr, NULL);
    
    //bind to webport
    memset(&bind_opts_http, 0, sizeof(bind_opts_http));
    bind_opts_http.user_data = (void *)user_data;
    bind_opts_http.error_string = &err_http;
    if (config->ssl == true)
        nc_http = mg_bind_opt(mgr, config->webport, ev_handler_redirect, bind_opts_http);
    else
        nc_http = mg_bind_opt(mgr, config->webport, ev_handler, bind_opts_http);
    if (nc_http == NULL) {
        printf("Error listening on port %s\n", config->webport);
        mg_mgr_free(mgr);
        return false;
    }
    mg_set_protocol_http_websocket(nc_http);
    LOG_INFO() printf("Listening on http port %s.\n", config->webport);

    //bind to sslport
    if (config->ssl == true) {
        memset(&bind_opts_https, 0, sizeof(bind_opts_https));
        bind_opts_https.user_data = (void *)user_data;
        bind_opts_https.error_string = &err_https;
        bind_opts_https.ssl_cert = config->sslcert;
        bind_opts_https.ssl_key = config->sslkey;
        nc_https = mg_bind_opt(mgr, config->sslport, ev_handler, bind_opts_https);
        if (nc_https == NULL) {
            printf("Error listening on port %s: %s\n", config->sslport, err_https);
            mg_mgr_free(mgr);
            return false;
        } 
        LOG_INFO() printf("Listening on ssl port %s\n", config->sslport);
        mg_set_protocol_http_websocket(nc_https);
    }
    return mgr;
}

void web_server_free(void *arg_mgr) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    mg_mgr_free(mgr);
}

void *web_server_loop(void *arg_mgr) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    while (s_signal_received == 0) {
        mg_mgr_poll(mgr, 50);
//        unsigned web_server_queue_length = tiny_queue_length(web_server_queue, 100);
//        if (web_server_queue_length > 0) {
            t_work_result *response = tiny_queue_shift(web_server_queue, 50);
            if (response != NULL) {
                if (response->conn_id == 0) {
                    //Websocket notify from mpd idle
                    send_ws_notify(mgr, response);
                } 
                else {
                    //api response
                    send_api_response(mgr, response);
                }
            }
//        }
    }
    mg_mgr_free(mgr);
    return NULL;
}

//private functions
static int is_websocket(const struct mg_connection *nc) {
    return nc->flags & MG_F_IS_WEBSOCKET;
}

static void send_ws_notify(struct mg_mgr *mgr, t_work_result *response) {
    struct mg_connection *nc;
    for (nc = mg_next(mgr, NULL); nc != NULL; nc = mg_next(mgr, nc)) {
        if (!is_websocket(nc))
            continue;
        mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, response->data, response->length);
    }
    free(response);
}

static void send_api_response(struct mg_mgr *mgr, t_work_result *response) {
    struct mg_connection *nc;
    for (nc = mg_next(mgr, NULL); nc != NULL; nc = mg_next(mgr, nc)) {
        if (nc->user_data != NULL) {
            t_user_data *user_data = (t_user_data *) nc->user_data;
            if (user_data->conn_id == response->conn_id) {
                mg_send_head(nc, 200, response->length, "Content-Type: application/json");
                mg_printf(nc, "%s", response->data);
            }
        }
    }
    free(response);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    t_user_data *user_data = (t_user_data *) nc->user_data;
    t_config *config = (t_config *) user_data->config;
    
    switch(ev) {
        case MG_EV_ACCEPT: {
            //increment conn_id
            if (user_data->conn_id < LONG_MAX)
                user_data->conn_id++;
            else
                user_data->conn_id = 1;
            
            //replace mgr user_data with connection specific user_data
            t_user_data *nc_user_data = (t_user_data*)malloc(sizeof(t_user_data));
            nc_user_data->config = config;
            nc_user_data->conn_id = user_data->conn_id;
            nc->user_data = nc_user_data;
            LOG_DEBUG() fprintf(stderr, "DEBUG: New connection id %ld.\n", user_data->conn_id);
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            LOG_VERBOSE() printf("New websocket request (%ld): %.*s\n", user_data->conn_id, hm->uri.len, hm->uri.p);
            if (mg_vcmp(&hm->uri, "/ws") != 0) {
                printf("ERROR: Websocket request not to /ws, closing connection\n");
                mg_printf(nc, "%s", "HTTP/1.1 403 FORBIDDEN\r\n\r\n");
                nc->flags |= MG_F_SEND_AND_CLOSE;
            }
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
             LOG_VERBOSE() printf("New Websocket connection established (%ld).\n", user_data->conn_id);
             char response[] = "{\"type\": \"welcome\", \"data\": {\"mympdVersion\": \"" MYMPD_VERSION "\"}}";
             mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, response, strlen(response));
             break;
        }
        case MG_EV_HTTP_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            LOG_VERBOSE() printf("HTTP request (%ld): %.*s\n", user_data->conn_id, hm->uri.len, hm->uri.p);
            if (mg_vcmp(&hm->uri, "/api") == 0) {
                bool rc = handle_api(user_data->conn_id, hm->body.p, hm->body.len);
                if (rc == false) {
                    printf("ERROR: Invalid API request.\n");
                    const char *response = "{\"type\": \"error\", \"data\": \"Invalid API request\"}";
                    mg_send_head(nc, 200, strlen(response), "Content-Type: application/json");
                    mg_printf(nc, "%s", response);
                }
            }
            else {
                static struct mg_serve_http_opts s_http_server_opts;
                s_http_server_opts.document_root = DOC_ROOT;
                s_http_server_opts.enable_directory_listing = "no";
                mg_serve_http(nc, hm, s_http_server_opts);
            }
            break;
        }
        case MG_EV_CLOSE: {
            LOG_VERBOSE() fprintf(stderr, "HTTP connection %ld closed.\n", user_data->conn_id);
            free(nc->user_data);
            break;
        }
        default: {
            break;
        }
    }
}

static void ev_handler_redirect(struct mg_connection *nc, int ev, void *ev_data) {
    char *host;
    char *crap;
    char host_header[1024];
    switch(ev) {
        case MG_EV_HTTP_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            struct mg_str *host_hdr = mg_get_http_header(hm, "Host");
            t_user_data *user_data = (t_user_data *) nc->user_data;
            t_config *config = (t_config *) user_data->config;
            
            snprintf(host_header, 1024, "%.*s", host_hdr->len, host_hdr->p);
            host = strtok_r(host_header, ":", &crap);
            char s_redirect[250];
            if (strcmp(config->sslport, "443") == 0)
                snprintf(s_redirect, 250, "https://%s/", host);
            else
                snprintf(s_redirect, 250, "https://%s:%s/", host, config->sslport);
            LOG_VERBOSE() printf("Redirecting to %s\n", s_redirect);
            mg_http_send_redirect(nc, 301, mg_mk_str(s_redirect), mg_mk_str(NULL));
            break;
        }
        default: {
            break;
        }
    }
}

static bool handle_api(long conn_id, const char *request_body, int request_len) {
    char *cmd;
    
    LOG_VERBOSE() printf("API request (%ld): %.*s\n", conn_id, request_len, request_body);
    const int je = json_scanf(request_body, request_len, "{cmd: %Q}", &cmd);
    if (je < 1)
        return false;

    enum mympd_cmd_ids cmd_id = get_cmd_id(cmd);
    if (cmd_id == 0)
        return false;
    
    t_work_request *request = (t_work_request*)malloc(sizeof(t_work_request));
    request->conn_id = conn_id;
    request->cmd_id = cmd_id;
    request->length = copy_string(request->data, request_body, 1000, request_len);
    
    if (strncmp(cmd, "MYMPD_API_", 10) == 0)
        tiny_queue_push(mympd_api_queue, request);
    else
        tiny_queue_push(mpd_client_queue, request);
        
    return true;
}
