/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
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
#include <time.h>

#include "../dist/src/sds/sds.h"
#include "../dist/src/mongoose/mongoose.h"
#include "../dist/src/frozen/frozen.h"

#include "sds_extras.h"
#include "api.h"
#include "log.h"
#include "list.h"
#include "mympd_config_defs.h"
#include "utility.h"
#include "tiny_queue.h"
#include "global.h"
#include "web_server/web_server_utility.h"
#include "web_server/web_server_albumart.h"
#include "web_server.h"

//private definitions
static bool parse_internal_message(t_work_result *response, t_mg_user_data *mg_user_data);
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data);
#ifdef ENABLE_SSL
  static void ev_handler_redirect(struct mg_connection *nc_http, int ev, void *ev_data, void *fn_data);
#endif
static void send_ws_notify(struct mg_mgr *mgr, t_work_result *response);
static void send_api_response(struct mg_mgr *mgr, t_work_result *response);
static bool handle_api(long long conn_id, struct mg_http_message *hm);
static bool handle_script_api(long long conn_id, struct mg_http_message *hm);
static void mpd_stream_proxy_forward(struct mg_http_message *hm, struct mg_connection *nc);
static void mpd_stream_proxy_ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data);

//public functions
bool web_server_init(void *arg_mgr, t_config *config, t_mg_user_data *mg_user_data) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;

    //initialize mgr user_data, malloced in main.c
    mg_user_data->config = config;
    mg_user_data->browse_document_root = sdscatfmt(sdsempty(), "%s/empty", config->varlibdir);
    mg_user_data->pics_document_root = sdscatfmt(sdsempty(), "%s/pics", config->varlibdir);
    mg_user_data->smartpls_document_root = sdscatfmt(sdsempty(), "%s/smartpls", config->varlibdir);
    mg_user_data->music_directory = sdsempty();
    mg_user_data->playlist_directory = sdsempty();
    mg_user_data->coverimage_names= split_coverimage_names(config->coverimage_name, mg_user_data->coverimage_names, &mg_user_data->coverimage_names_len);
    mg_user_data->feat_library = false;
    mg_user_data->feat_mpd_albumart = false;
    mg_user_data->connection_count = 0;
	mg_user_data->stream_uri = sdsnew("http://localhost:8000");

    //init monogoose mgr
    mg_mgr_init(mgr);
    mgr->userdata = mg_user_data;
    mgr->product_name = "myMPD "MYMPD_VERSION;
    
    //bind to http_port
    struct mg_connection *nc_http;
    sds http_url = sdscatfmt(sdsempty(), "http://%s:%s", config->http_host, config->http_port);
    #ifdef ENABLE_SSL
    if (config->ssl == true) {
        nc_http = mg_http_listen(mgr, http_url, ev_handler_redirect, NULL);
    }
    else {
    #endif
        nc_http = mg_http_listen(mgr, http_url, ev_handler, NULL);
    #ifdef ENABLE_SSL
    }
    #endif
    sdsfree(http_url);
    if (nc_http == NULL) {
        MYMPD_LOG_ERROR("Can't bind to http://%s:%s", config->http_host, config->http_port);
        mg_mgr_free(mgr);
        return false;
    }
    MYMPD_LOG_INFO("Listening on http://%s:%s", config->http_host, config->http_port);

    //bind to ssl_port
    #ifdef ENABLE_SSL
    if (config->ssl == true) {
        sds https_url = sdscatfmt(sdsempty(), "https://%s:%s", config->http_host, config->ssl_port);
        struct mg_connection *nc_https = mg_http_listen(mgr, https_url, ev_handler, NULL);
        sdsfree(https_url);
        if (nc_https == NULL) {
            MYMPD_LOG_ERROR("Can't bind to https://%s:%s", config->http_host, config->ssl_port);
            mg_mgr_free(mgr);
            return false;
        } 
        MYMPD_LOG_NOTICE("Listening on https://%s:%s", config->http_host, config->ssl_port);
        MYMPD_LOG_DEBUG("Using certificate: %s", config->ssl_cert);
        MYMPD_LOG_DEBUG("Using private key: %s", config->ssl_key);
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
    thread_logname = sdsreplace(thread_logname, "webserver");
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    
    //set mongoose loglevel
    mg_log_set("1");
    
    t_mg_user_data *mg_user_data = (t_mg_user_data *) mgr->userdata;
    sds last_notify = sdsempty();
    time_t last_time = 0;
    while (s_signal_received == 0) {
        t_work_result *response = tiny_queue_shift(web_server_queue, 50, 0);
        if (response != NULL) {
            if (response->conn_id == -1) {
                //internal message
                MYMPD_LOG_DEBUG("Got internal message");
                parse_internal_message(response, mg_user_data);
            }
            else if (response->conn_id == 0) {
                MYMPD_LOG_DEBUG("Got websocket notify");
                //websocket notify from mpd idle
                time_t now = time(NULL);
                if (strcmp(response->data, last_notify) != 0 || last_time < now - 1) {
                    last_notify = sdsreplace(last_notify, response->data);
                    last_time = now;
                    send_ws_notify(mgr, response);
                } 
                else {
                    free_result(response);                    
                }
            } 
            else {
                MYMPD_LOG_DEBUG("Got API response for id \"%ld\"", response->conn_id);
                //api response
                send_api_response(mgr, response);
            }
        }
        //webserver polling
        mg_mgr_poll(mgr, 50);
    }
    sdsfree(thread_logname);
    sdsfree(last_notify);
    return NULL;
}

//private functions
static bool parse_internal_message(t_work_result *response, t_mg_user_data *mg_user_data) {
    bool rc = false;
    if (response->extra != NULL) {	
	    struct set_mg_user_data_request *new_mg_user_data = (struct set_mg_user_data_request *)response->extra;
        
        mg_user_data->music_directory = sdsreplace(mg_user_data->music_directory, new_mg_user_data->music_directory);
        sdsfree(new_mg_user_data->music_directory);
        
        mg_user_data->playlist_directory = sdsreplace(mg_user_data->playlist_directory, new_mg_user_data->playlist_directory);
        sdsfree(new_mg_user_data->playlist_directory);
        
        sdsfreesplitres(mg_user_data->coverimage_names, mg_user_data->coverimage_names_len);
        mg_user_data->coverimage_names = split_coverimage_names(new_mg_user_data->coverimage_names, mg_user_data->coverimage_names, &mg_user_data->coverimage_names_len);
        sdsfree(new_mg_user_data->coverimage_names);
        
        mg_user_data->feat_library = new_mg_user_data->feat_library;
        mg_user_data->feat_mpd_albumart = new_mg_user_data->feat_mpd_albumart;

        sdsclear(mg_user_data->stream_uri);
        if (new_mg_user_data->mpd_stream_port != 0) {
            mg_user_data->stream_uri = sdscatprintf(mg_user_data->stream_uri, "http://%s:%u", 
                (strncmp(new_mg_user_data->mpd_host, "/", 1) == 0 ? "127.0.0.1" : new_mg_user_data->mpd_host),
                new_mg_user_data->mpd_stream_port);
        }
        sdsfree(new_mg_user_data->mpd_host);
        
		FREE_PTR(response->extra);
        rc = true;
    }
    else {
        MYMPD_LOG_WARN("Invalid internal message: %s", response->data);
    }
    free_result(response);
    return rc;
}

static void send_ws_notify(struct mg_mgr *mgr, t_work_result *response) {
    struct mg_connection *nc = mgr->conns;
    int i = 0;
    while (nc != NULL) {
        if ((int)nc->is_websocket == 1) {
            MYMPD_LOG_DEBUG("Sending notify to conn_id %lu: %s", nc->id, response->data);
            mg_ws_send(nc, response->data, sdslen(response->data), WEBSOCKET_OP_TEXT);
            i++;
        }
        nc = nc->next;
    }
    if (i == 0) {
        MYMPD_LOG_DEBUG("No websocket client connected, discarding message: %s", response->data);
    }
    free_result(response);
}

static void send_api_response(struct mg_mgr *mgr, t_work_result *response) {
    struct mg_connection *nc = mgr->conns;
    while (nc != NULL) {
        if ((int)nc->is_websocket == 0 && nc->id == (long unsigned)response->conn_id) {
            MYMPD_LOG_DEBUG("Sending response to conn_id %lu: %s", nc->id, response->data);
            if (response->cmd_id == MPD_API_ALBUMART) {
                send_albumart(nc, response->data, response->binary);
            }
            else {
                mg_http_reply(nc, 200, "Content-Type: application/json\r\n", response->data);
            }
            break;
        }
        nc = nc->next;
    }
    free_result(response);
}

// Reverse proxy
static void mpd_stream_proxy_forward(struct mg_http_message *hm, struct mg_connection *nc) {
    mg_printf(nc, "%.*s\r\n", (int) (hm->proto.ptr + hm->proto.len - hm->message.ptr), hm->message.ptr);
    mg_send(nc, "\r\n", 2);
    mg_send(nc, hm->body.ptr, hm->body.len);
}

static void mpd_stream_proxy_ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    struct mg_connection *frontend_nc = fn_data;
    switch(ev) {
        case MG_EV_READ:
            //forward incoming data from backend to frontend
            mg_send(frontend_nc, nc->recv.buf, nc->recv.len);
            mg_iobuf_delete(&nc->recv, nc->recv.len);
            break;
        case MG_EV_CLOSE: {
            MYMPD_LOG_INFO("Backend HTTP connection %lu closed", nc->id);
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

// Event handler
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    struct mg_connection *backend_nc = fn_data;
    t_mg_user_data *mg_user_data = (t_mg_user_data *) nc->mgr->userdata;
    t_config *config = (t_config *) mg_user_data->config;
    switch(ev) {
        case MG_EV_ACCEPT: {
            //check connection count
            if (mg_user_data->connection_count > 100) {
                nc->is_draining = 1;
                send_error(nc, 429, "Concurrent connections limit exceeded");
                break;
            }
            //check acl
            if (sdslen(config->acl) > 0 && check_ip_acl(config->acl, &nc->peer) == false) {
                nc->is_draining = 1;
                send_error(nc, 403, "Request blocked by ACL");
                break;
            }
            //ssl support
            if (config->ssl == true) {
                MYMPD_LOG_DEBUG("Init tls with cert %s and key %s", config->ssl_cert, config->ssl_key);
                struct mg_tls_opts tls_opts = {
                    .cert = config->ssl_cert,
                    .certkey = config->ssl_key
                };
                if (mg_tls_init(nc, &tls_opts) == 0) {
                    MYMPD_LOG_ERROR("Can not init tls with cert %s and key %s", config->ssl_cert, config->ssl_key);
                    nc->is_closing = 1;
                    break;
                }
            }
            mg_user_data->connection_count++;
            MYMPD_LOG_DEBUG("New connection id %lu, connections %d", nc->id, mg_user_data->connection_count);
            break;
        }
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;
            MYMPD_LOG_INFO("HTTP request (%lu): %.*s", nc->id, (int)hm->uri.len, hm->uri.ptr);
            
            if (mg_http_match_uri(hm, "/stream/")) {
                if (sdslen(mg_user_data->stream_uri) == 0) {
                    nc->is_draining = 1;
                    send_error(nc, 404, "MPD stream port not configured");
                    break;
                }
                if (backend_nc == NULL) {
                    // Client request, create backend connection Note that we're passing
                    // client connection `c` as fn_data for the created backend connection.
                    // This way, we tie together these two connections via `fn_data` pointer:
                    // client's c->fn_data points to the backend connection, and backend's
                    // c->fn_data points to the client connection
					MYMPD_LOG_INFO("Creating new mpd stream proxy backend connection to %s", mg_user_data->stream_uri);
                    backend_nc = mg_connect(nc->mgr, mg_user_data->stream_uri, mpd_stream_proxy_ev_handler, nc);
                    nc->fn_data = backend_nc;
                    if (backend_nc == NULL) {
						//no backend connection, close frontend connection
						MYMPD_LOG_WARN("Can not create backend connection");
                        nc->is_closing = 1;
                    }
                }
                if (backend_nc != NULL) {
                    //strip path
                    hm->uri.ptr = "/";
                    hm->uri.len = 1;
                    //forward request
					MYMPD_LOG_INFO("Forwarding client connection %lu to backend connection %lu", nc->id, backend_nc->id);
                    mpd_stream_proxy_forward(hm, backend_nc);
                }
            }
            else if (mg_http_match_uri(hm, "/ws/")) {
                mg_ws_upgrade(nc, hm, NULL);
                MYMPD_LOG_INFO("New Websocket connection established (%lu)", nc->id);
                sds response = jsonrpc_event(sdsempty(), "welcome");
                mg_ws_send(nc, response, sdslen(response), WEBSOCKET_OP_TEXT);
                sdsfree(response);
            }
            else if (mg_http_match_uri(hm, "/api/script")) {
                if (config->remotescripting == false) {
                    nc->is_draining = 1;
                    send_error(nc, 403, "Remote scripting is disabled");
                    break;
                }
                if (sdslen(config->scriptacl) > 0 && check_ip_acl(config->scriptacl, &nc->peer) == false) {
                    nc->is_draining = 1;
                    send_error(nc, 403, "Request blocked by ACL");
                    break;
                }
                bool rc = handle_script_api((long long)nc->id, hm);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Invalid script API request");
                    sds response = jsonrpc_respond_message(sdsempty(), "", 0, true,
                        "script", "error", "Invalid script API request");
                    mg_http_reply(nc, 200, "Content-Type: application/json\r\n", response);
                    sdsfree(response);
                }
            }
            else if (mg_http_match_uri(hm, "/api/serverinfo")) {
                struct sockaddr_in localip;
                socklen_t len = sizeof(localip);
                if (getsockname((int)(long)nc->fd, (struct sockaddr *)&localip, &len) == 0) {
                    sds response = jsonrpc_result_start(sdsempty(), "", 0);
                    response = tojson_char(response, "version", MG_VERSION, true);
                    response = tojson_char(response, "ip", inet_ntoa(localip.sin_addr), false);
                    response = jsonrpc_result_end(response);
                    mg_http_reply(nc, 200, "Content-Type: application/json\r\n", response);
                    sdsfree(response);
                }
            }
            else if (mg_http_match_uri(hm, "/api")) {
                //api request
                bool rc = handle_api((long long)nc->id, hm);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Invalid API request");
                    sds response = jsonrpc_respond_message(sdsempty(), "", 0, true,
                        "general", "error", "Invalid API request");
                    mg_http_reply(nc, 200, "Content-Type: application/json\r\n", response);
                    sdsfree(response);
                }
            }
            #ifdef ENABLE_SSL
            else if (mg_http_match_uri(hm, "/ca.crt")) { 
                if (config->custom_cert == false) {
                    //deliver ca certificate
                    sds ca_file = sdscatfmt(sdsempty(), "%s/ssl/ca.pem", config->varlibdir);
                    mg_http_serve_file(nc, hm, ca_file, "application/x-x509-ca-cert", NULL);
                    sdsfree(ca_file);
                }
                else {
                    send_error(nc, 404, "Custom cert enabled, don't deliver myMPD ca");
                }
            }
            #endif
            else if (mg_http_match_uri(hm, "/albumart/#")) {
                handle_albumart(nc, hm, mg_user_data, config, (long long)nc->id);
            }
            else if (mg_http_match_uri(hm, "/pics/#")) {
                //serve directory
                MYMPD_LOG_DEBUG("Setting document root to \"%s\"", mg_user_data->pics_document_root);
                static struct mg_http_serve_opts s_http_server_opts;
                s_http_server_opts.root_dir = mg_user_data->pics_document_root;
                s_http_server_opts.enable_directory_listing = 0;
                s_http_server_opts.extra_headers = EXTRA_HEADERS_CACHE;
                hm->uri = mg_str_strip_parent(&hm->uri, 1);
                mg_http_serve_dir(nc, hm, &s_http_server_opts);
            }
            else if (mg_http_match_uri(hm, "/browse/#")) {
                if (config->publish == false) {
                    send_error(nc, 403, "Publishing of directories is disabled");
                    break;
                }
                static struct mg_http_serve_opts s_http_server_opts;
                s_http_server_opts.extra_headers = EXTRA_HEADERS_DIR;
                s_http_server_opts.enable_directory_listing = 1;
                s_http_server_opts.directory_listing_css = DIRECTORY_LISTING_CSS;
                if (mg_http_match_uri(hm, "/browse/")) {
                    s_http_server_opts.root_dir = mg_user_data->browse_document_root;
                    hm->uri = mg_str_strip_parent(&hm->uri, 1);
                }
                else if (mg_http_match_uri(hm, "/browse/pics/#")) {
                    s_http_server_opts.root_dir = mg_user_data->pics_document_root;
                    hm->uri = mg_str_strip_parent(&hm->uri, 2);
                }
                else if (mg_http_match_uri(hm, "/browse/smartplaylists/#")) {
                    s_http_server_opts.root_dir = mg_user_data->smartpls_document_root;
                    hm->uri = mg_str_strip_parent(&hm->uri, 2);
                }
                else if (mg_http_match_uri(hm, "/browse/playlists/#")) {
                    s_http_server_opts.root_dir = mg_user_data->playlist_directory;
                    hm->uri = mg_str_strip_parent(&hm->uri, 2);
                }
                else if (mg_http_match_uri(hm, "/browse/music/#")) {
                    s_http_server_opts.root_dir = mg_user_data->music_directory;
                    hm->uri = mg_str_strip_parent(&hm->uri, 2);
                }
                mg_http_serve_dir(nc, hm, &s_http_server_opts);
            }
            else if (mg_vcmp(&hm->uri, "/index.html") == 0) {
                http_send_header_redirect(nc, "/");
            }
            else if (mg_vcmp(&hm->uri, "/favicon.ico") == 0) {
                http_send_header_redirect(nc, "/assets/favicon.ico");
            }
            else {
                //all other uris
                #ifdef DEBUG
                //serve all files from filesystem
                static struct mg_http_serve_opts s_http_server_opts;
                s_http_server_opts.root_dir = DOC_ROOT;
                s_http_server_opts.enable_directory_listing = 0;
                s_http_server_opts.extra_headers = EXTRA_HEADERS;
                mg_http_serve_dir(nc, hm, &s_http_server_opts);
                #else
                //serve embedded files
                sds uri = sdsnewlen(hm->uri.ptr, hm->uri.len);
                serve_embedded_files(nc, uri, hm);
                sdsfree(uri);
                #endif
            }
            break;
        }
        case MG_EV_CLOSE: {
            MYMPD_LOG_INFO("HTTP connection %lu closed", nc->id);
            mg_user_data->connection_count--;
            if (backend_nc != NULL) {
                //remove pointer to frontend connection
                backend_nc->fn_data = NULL;
                //close reverse proxy connection
                backend_nc->is_closing = 1;
            }
            break;
        }
    }
}

#ifdef ENABLE_SSL
static void ev_handler_redirect(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    (void)fn_data;
    t_mg_user_data *mg_user_data = (t_mg_user_data *) nc->mgr->userdata;
    t_config *config = (t_config *) mg_user_data->config;
    if (ev == MG_EV_ACCEPT) {
        //check connection count
        if (mg_user_data->connection_count > 100) {
            nc->is_draining = 1;
            send_error(nc, 429, "Concurrent connections limit exceeded");
            return;
        }
        //check acl
        if (sdslen(config->acl) > 0 && check_ip_acl(config->acl, &nc->peer) == false) {
            nc->is_draining = 1;
            send_error(nc, 403, "Request blocked by ACL");
            return;
        }
        mg_user_data->connection_count++;
    }
    else if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        struct mg_str *host_hdr = mg_http_get_header(hm, "Host");
        sds host_header = sdscatlen(sdsempty(), host_hdr->ptr, (int)host_hdr->len);
        int count;
        sds *tokens = sdssplitlen(host_header, sdslen(host_header), ":", 1, &count);
        sds s_redirect = sdscatfmt(sdsempty(), "https://%s", tokens[0]);
        if (strcmp(config->ssl_port, "443") != 0) {
            s_redirect = sdscatfmt(s_redirect, ":%s", config->ssl_port);
        }
        MYMPD_LOG_INFO("Redirecting to %s", s_redirect);
        http_send_header_redirect(nc, s_redirect);
        sdsfreesplitres(tokens, count);
        sdsfree(host_header);
        sdsfree(s_redirect);
    }
}
#endif

static bool handle_api(long long conn_id, struct mg_http_message *hm) {
    if (hm->body.len > 2048) {
        MYMPD_LOG_ERROR("Request length of %d exceeds max request size, discarding request)", hm->body.len);
        return false;
    }
    
    MYMPD_LOG_DEBUG("API request (%lld): %.*s", conn_id, hm->body.len, hm->body.ptr);
    char *cmd = NULL;
    char *jsonrpc = NULL;
    int id = 0;
    const int je = json_scanf(hm->body.ptr, hm->body.len, "{jsonrpc: %Q, method: %Q, id: %d}", &jsonrpc, &cmd, &id);
    if (je < 3) {
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }
    MYMPD_LOG_INFO("API request (%lld): %s", conn_id, cmd);

    enum mympd_cmd_ids cmd_id = get_cmd_id(cmd);
    if (cmd_id == 0 || strncmp(jsonrpc, "2.0", 3) != 0) {
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }
    
    if (is_public_api_method(cmd_id) == false) {
        MYMPD_LOG_ERROR("API method %s is privat", cmd);
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }
    
    sds data = sdscatlen(sdsempty(), hm->body.ptr, hm->body.len);
    t_work_request *request = create_request(conn_id, id, cmd_id, cmd, data);
    sdsfree(data);
    
    if (strncmp(cmd, "MYMPD_API_", 10) == 0) {
        tiny_queue_push(mympd_api_queue, request, 0);
    }
    else if (strncmp(cmd, "MPDWORKER_API_", 14) == 0) {
        tiny_queue_push(mpd_worker_queue, request, 0);
        
    }
    else {
        tiny_queue_push(mpd_client_queue, request, 0);
    }

    FREE_PTR(cmd);
    FREE_PTR(jsonrpc);
    return true;
}

static bool handle_script_api(long long conn_id, struct mg_http_message *hm) {
    if (hm->body.len > 4096) {
        MYMPD_LOG_ERROR("Request length of %d exceeds max request size, discarding request)", hm->body.len);
        return false;
    }
    
    MYMPD_LOG_DEBUG("Script API request (%lld): %.*s", conn_id, hm->body.len, hm->body.ptr);
    char *cmd = NULL;
    char *jsonrpc = NULL;
    long id = 0;
    const int je = json_scanf(hm->body.ptr, hm->body.len, "{jsonrpc: %Q, method: %Q, id: %ld}", &jsonrpc, &cmd, &id);
    if (je < 3) {
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }
    MYMPD_LOG_INFO("Script API request (%lld): %s", conn_id, cmd);

    enum mympd_cmd_ids cmd_id = get_cmd_id(cmd);
    if (cmd_id == 0 || strncmp(jsonrpc, "2.0", 3) != 0) {
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }
    
    if (cmd_id != MYMPD_API_SCRIPT_POST_EXECUTE) {
        MYMPD_LOG_ERROR("API method %s is invalid for this uri", cmd);
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }
    
    sds data = sdscatlen(sdsempty(), hm->body.ptr, hm->body.len);
    t_work_request *request = create_request(conn_id, id, cmd_id, cmd, data);
    sdsfree(data);
    tiny_queue_push(mympd_api_queue, request, 0);

    FREE_PTR(cmd);
    FREE_PTR(jsonrpc);
    return true;
}
