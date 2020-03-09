/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <limits.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <inttypes.h>
#include <libgen.h>
#include <string.h>
#include <errno.h>

#include "../dist/src/sds/sds.h"
#include "../dist/src/mongoose/mongoose.h"
#include "../dist/src/frozen/frozen.h"

#include "sds_extras.h"
#include "api.h"
#include "log.h"
#include "list.h"
#include "config_defs.h"
#include "utility.h"
#include "tiny_queue.h"
#include "global.h"
#include "web_server/web_server_utility.h"
#include "web_server/web_server_albumart.h"
#include "web_server.h"

//private definitions
static bool parse_internal_message(t_work_result *response, t_mg_user_data *mg_user_data);
static int is_websocket(const struct mg_connection *nc);
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data);
#ifdef ENABLE_SSL
  static void ev_handler_redirect(struct mg_connection *nc_http, int ev, void *ev_data);
#endif
static void send_ws_notify(struct mg_mgr *mgr, t_work_result *response);
static void send_api_response(struct mg_mgr *mgr, t_work_result *response);
static bool handle_api(int conn_id, struct http_message *hm);

//public functions
bool web_server_init(void *arg_mgr, t_config *config, t_mg_user_data *mg_user_data) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;

    //initialize mgr user_data, malloced in main.c
    mg_user_data->config = config;
    mg_user_data->browse_document_root = sdscatfmt(sdsempty(), "%s/empty", config->varlibdir);
    mg_user_data->music_directory = sdsempty();
    mg_user_data->playlist_directory = sdsempty();
    mg_user_data->rewrite_patterns = sdsempty();
    mg_user_data->coverimage_names= split_coverimage_names(config->coverimage_name, mg_user_data->coverimage_names, &mg_user_data->coverimage_names_len);
    mg_user_data->conn_id = 1;
    mg_user_data->feat_library = false;
    mg_user_data->feat_mpd_albumart = false;
    
    //init monogoose mgr with mg_user_data
    mg_mgr_init(mgr, mg_user_data);
    
    //bind to webport
    struct mg_connection *nc_http;
    struct mg_bind_opts bind_opts_http;
    const char *err_http;
    memset(&bind_opts_http, 0, sizeof(bind_opts_http));
    bind_opts_http.error_string = &err_http;
    #ifdef ENABLE_SSL
    if (config->ssl == true) {
        nc_http = mg_bind_opt(mgr, config->webport, ev_handler_redirect, bind_opts_http);
    }
    else {
    #endif
        nc_http = mg_bind_opt(mgr, config->webport, ev_handler, bind_opts_http);
    #ifdef ENABLE_SSL
    }
    #endif
    if (nc_http == NULL) {
        LOG_ERROR("Can't bind to port %s: %s", config->webport, err_http);
        mg_mgr_free(mgr);
        return false;
    }
    mg_set_protocol_http_websocket(nc_http);
    LOG_INFO("Listening on http port %s", config->webport);

    //bind to sslport
    #ifdef ENABLE_SSL
    const char *err_https;
    struct mg_connection *nc_https;
    struct mg_bind_opts bind_opts_https;
    if (config->ssl == true) {
        memset(&bind_opts_https, 0, sizeof(bind_opts_https));
        bind_opts_https.error_string = &err_https;
        bind_opts_https.ssl_cert = config->ssl_cert;
        bind_opts_https.ssl_key = config->ssl_key;
        nc_https = mg_bind_opt(mgr, config->ssl_port, ev_handler, bind_opts_https);
        if (nc_https == NULL) {
            LOG_ERROR("Can't bind to port %s: %s", config->ssl_port, err_https);
            mg_mgr_free(mgr);
            return false;
        } 
        LOG_INFO("Listening on ssl port %s", config->ssl_port);
        LOG_DEBUG("Using certificate: %s", config->ssl_cert);
        LOG_DEBUG("Using private key: %s", config->ssl_key);
        mg_set_protocol_http_websocket(nc_https);
    }
    #endif
    return mgr;
}

void web_server_free(void *arg_mgr) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    mg_mgr_free(mgr);
    mgr = NULL;
}

void *web_server_loop(void *arg_mgr) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    t_mg_user_data *mg_user_data = (t_mg_user_data *) mgr->user_data;
    while (s_signal_received == 0) {
        unsigned web_server_queue_length = tiny_queue_length(web_server_queue, 50);
        if (web_server_queue_length > 0) {
            t_work_result *response = tiny_queue_shift(web_server_queue, 50);
            if (response != NULL) {
                if (response->conn_id == -1) {
                    //internal message
                    parse_internal_message(response, mg_user_data);
                }
                else if (response->conn_id == 0) {
                    //websocket notify from mpd idle
                    send_ws_notify(mgr, response);
                } 
                else {
                    //api response
                    send_api_response(mgr, response);
                }
            }
        }
        //webserver polling
        mg_mgr_poll(mgr, 50);
    }
    return NULL;
}

//private functions
static bool parse_internal_message(t_work_result *response, t_mg_user_data *mg_user_data) {
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    bool feat_library;
    bool feat_mpd_albumart;
    bool rc = false;
    t_config *config = (t_config *) mg_user_data->config;
    
    int je = json_scanf(response->data, sdslen(response->data), "{playlistDirectory: %Q, musicDirectory: %Q, coverimageName: %Q, featLibrary: %B, featMpdAlbumart: %B}", 
        &p_charbuf3, &p_charbuf1, &p_charbuf2, &feat_library, &feat_mpd_albumart);
    if (je == 5) {
        mg_user_data->music_directory = sdsreplace(mg_user_data->music_directory, p_charbuf1);
        mg_user_data->playlist_directory = sdsreplace(mg_user_data->playlist_directory, p_charbuf3);
        sdsfreesplitres(mg_user_data->coverimage_names, mg_user_data->coverimage_names_len);
        mg_user_data->coverimage_names = split_coverimage_names(p_charbuf2, mg_user_data->coverimage_names, &mg_user_data->coverimage_names_len);
        mg_user_data->feat_library = feat_library;
        mg_user_data->feat_mpd_albumart = feat_mpd_albumart;
        
        mg_user_data->rewrite_patterns = sdscrop(mg_user_data->rewrite_patterns);
        if (config->publish == true) {
            mg_user_data->rewrite_patterns = sdscatfmt(mg_user_data->rewrite_patterns, "/browse/pics=%s/pics", config->varlibdir);
            if (config->smartpls == true) {
                mg_user_data->rewrite_patterns = sdscatfmt(mg_user_data->rewrite_patterns, ",/browse/smartplaylists=%s/smartpls", config->varlibdir);
            }
            if (feat_library == true) {
                mg_user_data->rewrite_patterns = sdscatfmt(mg_user_data->rewrite_patterns, ",/browse/music=%s", mg_user_data->music_directory);
            }
            if (sdslen(mg_user_data->playlist_directory) > 0) {
                mg_user_data->rewrite_patterns = sdscatfmt(mg_user_data->rewrite_patterns, ",/browse/playlists=%s", mg_user_data->playlist_directory);
            }
            mg_user_data->rewrite_patterns = sdscatfmt(mg_user_data->rewrite_patterns, ",/browse=%s/empty", config->varlibdir);
            if (config->readonly == false) {
                //maintain directory structure in empty directory
                manage_emptydir(config->varlibdir, true, config->smartpls, feat_library, (sdslen(mg_user_data->playlist_directory) > 0 ? true : false));
            }
        }
        LOG_DEBUG("Setting rewrite_patterns to %s", mg_user_data->rewrite_patterns);
        rc = true;
    }
    else {
        LOG_WARN("Unknown internal message: %s", response->data);
        rc = false;
    }
    FREE_PTR(p_charbuf1);
    FREE_PTR(p_charbuf2);
    FREE_PTR(p_charbuf3);
    free_result(response);
    return rc;
}

static int is_websocket(const struct mg_connection *nc) {
    return nc->flags & MG_F_IS_WEBSOCKET;
}

static void send_ws_notify(struct mg_mgr *mgr, t_work_result *response) {
    struct mg_connection *nc;
    for (nc = mg_next(mgr, NULL); nc != NULL; nc = mg_next(mgr, nc)) {
        if (!is_websocket(nc))
            continue;
        if (nc->user_data != NULL) {
            LOG_DEBUG("Sending notify to conn_id %d: %s", (intptr_t)nc->user_data, response->data);
        }
        else {
            LOG_WARN("Sending notify to unknown connection: %s", response->data);
        }
        mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, response->data, sdslen(response->data));
    }
    free_result(response);
}

static void send_api_response(struct mg_mgr *mgr, t_work_result *response) {
    struct mg_connection *nc;
    for (nc = mg_next(mgr, NULL); nc != NULL; nc = mg_next(mgr, nc)) {
        if (is_websocket(nc)) {
            continue;
        }
        if (nc->user_data != NULL) {
            if ((intptr_t)nc->user_data == response->conn_id) {
                LOG_DEBUG("Sending response to conn_id %d: %s", (intptr_t)nc->user_data, response->data);
                if (response->cmd_id == MPD_API_ALBUMART) {
                    send_albumart(nc, response->data, response->binary);
                }
                else {
                    mg_send_head(nc, 200, sdslen(response->data), "Content-Type: application/json");
                    mg_send(nc, response->data, sdslen(response->data));
                }
                break;
            }
        }
        else {
            LOG_WARN("Unknown connection");
        }
    }
    free_result(response);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    t_mg_user_data *mg_user_data = (t_mg_user_data *) nc->mgr->user_data;
    t_config *config = (t_config *) mg_user_data->config;
    
    switch(ev) {
        case MG_EV_ACCEPT: {
            //increment conn_id
            if (mg_user_data->conn_id < INT_MAX) {
                mg_user_data->conn_id++;
            }
            else {
                mg_user_data->conn_id = 1;
            }
            //set conn_id
            nc->user_data = (void *)(intptr_t)mg_user_data->conn_id;
            LOG_DEBUG("New connection id %d", (intptr_t)nc->user_data);

            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            LOG_VERBOSE("New websocket request (%d): %.*s", (intptr_t)nc->user_data, (int)hm->uri.len, hm->uri.p);
            if (mg_vcmp(&hm->uri, "/ws/") != 0) {
                nc->flags |= MG_F_SEND_AND_CLOSE;
                send_error(nc, 403, "Websocket request not to /ws/, closing connection");
            }
            break;
        }
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
             LOG_VERBOSE("New Websocket connection established (%d)", (intptr_t)nc->user_data);
             sds response = jsonrpc_start_notify(sdsempty(), "welcome");
             response = tojson_char(response, "mympdVersion", MYMPD_VERSION, false);
             response = jsonrpc_end_notify(response);
             mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, response, sdslen(response));
             sdsfree(response);
             break;
        }
        case MG_EV_HTTP_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            static const struct mg_str browse_prefix = MG_MK_STR("/browse");
            static const struct mg_str albumart_prefix = MG_MK_STR("/albumart");
            LOG_VERBOSE("HTTP request (%d): %.*s", (intptr_t)nc->user_data, (int)hm->uri.len, hm->uri.p);
            if (mg_vcmp(&hm->uri, "/api/serverinfo") == 0) {
                struct sockaddr_in localip;
                socklen_t len = sizeof(localip);
                if (getsockname(nc->sock, (struct sockaddr *)&localip, &len) == 0) {
                    sds method = sdsempty();
                    sds response = jsonrpc_start_result(sdsempty(), method, 0);
                    response = sdscat(response, ",");
                    response = tojson_char(response, "version", MG_VERSION, true);
                    response = tojson_char(response, "ip", inet_ntoa(localip.sin_addr), false);
                    response = jsonrpc_end_result(response);
                    mg_send_head(nc, 200, sdslen(response), "Content-Type: application/json");
                    mg_send(nc, response, sdslen(response));
                    sdsfree(response);
                    sdsfree(method);
                }
            }
            else if (mg_vcmp(&hm->uri, "/api") == 0) {
                //api request
                bool rc = handle_api((intptr_t)nc->user_data, hm);
                if (rc == false) {
                    LOG_ERROR("Invalid API request");
                    sds method = sdsempty();
                    sds response = jsonrpc_respond_message(sdsempty(), method, 0, "Invalid API request", true);
                    mg_send_head(nc, 200, sdslen(response), "Content-Type: application/json");
                    mg_send(nc, response, sdslen(response));
                    sdsfree(response);
                    sdsfree(method);
                }
            }
            #ifdef ENABLE_SSL
            else if (mg_vcmp(&hm->uri, "/ca.crt") == 0) { 
                if (config->custom_cert == false) {
                    //deliver ca certificate
                    sds ca_file = sdscatfmt(sdsempty(), "%s/ssl/ca.pem", config->varlibdir);
                    mg_http_serve_file(nc, hm, ca_file, mg_mk_str("application/x-x509-ca-cert"), mg_mk_str(""));
                    sdsfree(ca_file);
                }
                else {
                    send_error(nc, 404, "Custom cert enabled, don't deliver myMPD ca");
                }
            }
            #endif
            else if (mg_str_starts_with(hm->uri, albumart_prefix) == 1) {
                handle_albumart(nc, hm, mg_user_data, config, (intptr_t)nc->user_data);
            }
            else if (mg_str_starts_with(hm->uri, browse_prefix) == 1) {
                if (config->publish == false) {
                    send_error(nc, 403, "Publishing of directories is disabled");
                }
                if (config->webdav == false && mg_vcmp(&hm->method, "GET") == 1) {
                    send_error(nc, 405, "Method not allowed (webdav is disabled)");
                }
                else {
                    //serve directory
                    static struct mg_serve_http_opts s_http_server_opts;
                    s_http_server_opts.document_root = mg_user_data->browse_document_root;
                    if (config->webdav == true) {
                        s_http_server_opts.dav_document_root = mg_user_data->browse_document_root;
                        s_http_server_opts.dav_auth_file = "-";
                    }
                    s_http_server_opts.url_rewrites = mg_user_data->rewrite_patterns;
                    s_http_server_opts.enable_directory_listing = "yes";
                    s_http_server_opts.extra_headers = EXTRA_HEADERS_DIR;
                    mg_serve_http(nc, hm, s_http_server_opts);
                }
            }
            else if (mg_vcmp(&hm->uri, "/index.html") == 0) {
                mg_http_send_redirect(nc, 301, mg_mk_str("/"), mg_mk_str(NULL));
            }
            else if (mg_vcmp(&hm->uri, "/favicon.ico") == 0) {
                mg_http_send_redirect(nc, 301, mg_mk_str("/assets/favicon.ico"), mg_mk_str(NULL));
            }
            else {
                //all other uris
                #ifdef DEBUG
                //serve all files from filesystem
                static struct mg_serve_http_opts s_http_server_opts;
                s_http_server_opts.document_root = DOC_ROOT;
                s_http_server_opts.enable_directory_listing = "no";
                s_http_server_opts.extra_headers = EXTRA_HEADERS;
                s_http_server_opts.custom_mime_types = CUSTOM_MIME_TYPES;
                mg_serve_http(nc, hm, s_http_server_opts);
                #else
                //serve embedded files
                sds uri = sdsnewlen(hm->uri.p, hm->uri.len);
                serve_embedded_files(nc, uri, hm);
                sdsfree(uri);
                #endif
            }
            break;
        }
        case MG_EV_CLOSE: {
            if (nc->user_data != NULL) {
                LOG_VERBOSE("HTTP connection %ld closed", (intptr_t)nc->user_data);
                nc->user_data = NULL;
            }
            break;
        }
        default: {
            break;
        }
    }
}

#ifdef ENABLE_SSL
static void ev_handler_redirect(struct mg_connection *nc, int ev, void *ev_data) {
    switch(ev) {
        case MG_EV_HTTP_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            struct mg_str *host_hdr = mg_get_http_header(hm, "Host");
            t_mg_user_data *mg_user_data = (t_mg_user_data *) nc->mgr->user_data;
            t_config *config = (t_config *) mg_user_data->config;
            
            sds host_header = sdscatlen(sdsempty(), host_hdr->p, (int)host_hdr->len);
            
            int count;
            sds *tokens = sdssplitlen(host_header, sdslen(host_header), ":", 1, &count);
            
            sds s_redirect = sdscatfmt(sdsempty(), "https://%s", tokens[0]);
            if (strcmp(config->ssl_port, "443") != 0) {
                s_redirect = sdscatfmt(s_redirect, ":%s", config->ssl_port);
            }
            s_redirect = sdscat(s_redirect, "/");
            LOG_VERBOSE("Redirecting to %s", s_redirect);
            mg_http_send_redirect(nc, 301, mg_mk_str(s_redirect), mg_mk_str(NULL));
            sdsfreesplitres(tokens, count);
            sdsfree(host_header);
            sdsfree(s_redirect);
            break;
        }
        default: {
            break;
        }
    }
}
#endif

static bool handle_api(int conn_id, struct http_message *hm) {
    if (hm->body.len > 2048) {
        LOG_ERROR("Request length of %d exceeds max request size, discarding request)", hm->body.len);
        return false;
    }
    
    LOG_DEBUG("API request (%d): %.*s", conn_id, hm->body.len, hm->body.p);
    char *cmd = NULL;
    char *jsonrpc = NULL;
    int id = 0;
    const int je = json_scanf(hm->body.p, hm->body.len, "{jsonrpc: %Q, method: %Q, id: %d}", &jsonrpc, &cmd, &id);
    if (je < 3) {
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }
    LOG_VERBOSE("API request (%d): %s", conn_id, cmd);

    enum mympd_cmd_ids cmd_id = get_cmd_id(cmd);
    if (cmd_id == 0 || strncmp(jsonrpc, "2.0", 3) != 0) {
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }
    
    sds data = sdscatlen(sdsempty(), hm->body.p, hm->body.len);
    t_work_request *request = create_request(conn_id, id, cmd_id, cmd, data);
    sdsfree(data);
    
    if (strncmp(cmd, "MYMPD_API_", 10) == 0) {
        tiny_queue_push(mympd_api_queue, request);
    }
    else {
        tiny_queue_push(mpd_client_queue, request);
    }

    FREE_PTR(cmd);
    FREE_PTR(jsonrpc);    
    return true;
}
