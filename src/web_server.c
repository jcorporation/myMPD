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
#include "../dist/src/mongoose/mongoose_compat.h"
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
static bool handle_api(int conn_id, struct mg_http_message *hm);
static bool handle_script_api(int conn_id, struct mg_http_message *hm);

//public functions
bool web_server_init(void *arg_mgr, t_config *config, t_mg_user_data *mg_user_data) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;

    //initialize mgr user_data, malloced in main.c
    mg_user_data->config = config;
    mg_user_data->browse_document_root = sdscatfmt(sdsempty(), "%s/empty", config->varlibdir);
    mg_user_data->pics_document_root = sdscatfmt(sdsempty(), "%s/pics", config->varlibdir);
    mg_user_data->music_directory = sdsempty();
    mg_user_data->playlist_directory = sdsempty();
    mg_user_data->rewrite_patterns = sdsempty();
    mg_user_data->coverimage_names= split_coverimage_names(config->coverimage_name, mg_user_data->coverimage_names, &mg_user_data->coverimage_names_len);
    mg_user_data->conn_id = 1;
    mg_user_data->feat_library = false;
    mg_user_data->feat_mpd_albumart = false;

    //init monogoose mgr
    
    mg_mgr_init(mgr);
    mgr->userdata = mg_user_data;
    
    //bind to http_port
    struct mg_connection *nc_http;
    sds http_url = sdscatfmt(sdsempty(), "http://%s:%s", config->http_host, config->http_port);
    #ifdef ENABLE_SSL
    if (config->ssl == true && config->redirect == true) {
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
        struct mg_connection *nc_https = mg_http_listen(mgr, https_url, ev_handler_redirect, NULL);
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
    switch(loglevel) {
        case 7:
            mg_log_set("3");
            break;
        case 6:
        case 5:
            mg_log_set("2");
            break;
        default:
            mg_log_set("1");
    }
    t_mg_user_data *mg_user_data = (t_mg_user_data *) mgr->userdata;
    sds last_notify = sdsempty();
    time_t last_time = 0;
    while (s_signal_received == 0) {
        t_work_result *response = tiny_queue_shift(web_server_queue, 50, 0);
        if (response != NULL) {
            if (response->conn_id == -1) {
                //internal message
                parse_internal_message(response, mg_user_data);
            }
            else if (response->conn_id == 0) {
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
        MYMPD_LOG_DEBUG("Setting rewrite_patterns to %s", mg_user_data->rewrite_patterns);
        rc = true;
    }
    else {
        MYMPD_LOG_WARN("Unknown internal message: %s", response->data);
        rc = false;
    }
    FREE_PTR(p_charbuf1);
    FREE_PTR(p_charbuf2);
    FREE_PTR(p_charbuf3);
    free_result(response);
    return rc;
}

static void send_ws_notify(struct mg_mgr *mgr, t_work_result *response) {
    struct mg_connection *nc = mgr->conns;
    int i = 0;
    while (nc != NULL) {
        if (nc->is_websocket == 0) {
            continue;
        }
        i++;
        if (nc->fn_data != NULL) {
            MYMPD_LOG_DEBUG("Sending notify to conn_id %d: %s", (intptr_t)nc->fn_data, response->data);
        }
        else {
            MYMPD_LOG_WARN("Sending notify to unknown connection: %s", response->data);
        }
        mg_ws_send(nc, response->data, sdslen(response->data), WEBSOCKET_OP_TEXT);
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
        if (nc->is_websocket == 1) {
            continue;
        }
        if (nc->fn_data != NULL) {
            if ((intptr_t)nc->fn_data == response->conn_id) {
                MYMPD_LOG_DEBUG("Sending response to conn_id %d: %s", (intptr_t)nc->fn_data, response->data);
                if (response->cmd_id == MPD_API_ALBUMART) {
                    send_albumart(nc, response->data, response->binary);
                }
                else {
                    mg_http_reply(nc, 200, "Content-Type: application/json\r\n", response->data);
                }
                break;
            }
        }
        else {
            MYMPD_LOG_WARN("Unknown connection");
        }
        nc = nc->next;
    }
    free_result(response);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    t_mg_user_data *mg_user_data = (t_mg_user_data *) nc->mgr->userdata;
    t_config *config = (t_config *) mg_user_data->config;
    (void)fn_data;    
    switch(ev) {
        case MG_EV_ACCEPT: {
            //check acl
            if (sdslen(config->acl) > 0) {
                uint32_t remote_ip = ntohl(nc->peer.ip);
                int allowed = mg6_check_ip_acl(config->acl, remote_ip);
                if (allowed == -1) {
                    MYMPD_LOG_ERROR("ACL malformed");
                }
                if (allowed != 1) {
                    nc->is_draining = 1;
                    send_error(nc, 403, "Request blocked by ACL");
                    break;
                }
            }
            //ssl support
            if (config->ssl == true) {
                struct mg_tls_opts tls_opts;
                tls_opts.cert = config->ssl_cert;
                tls_opts.certkey = config->ssl_key;
                if (mg_tls_init(nc, &tls_opts) == 0) {
                    MYMPD_LOG_ERROR("Can't init tls with cert %s and key %s", config->ssl_cert, config->ssl_key);
                    return;
                }
            }

            //increment conn_id
            if (mg_user_data->conn_id < INT_MAX) {
                mg_user_data->conn_id++;
            }
            else {
                mg_user_data->conn_id = 1;
            }
            //set conn_id
            nc->fn_data = (void *)(intptr_t)mg_user_data->conn_id;
            MYMPD_LOG_DEBUG("New connection id %d", (intptr_t)nc->fn_data);
            break;
        }
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;

            MYMPD_LOG_INFO("HTTP request (%d): %.*s", (intptr_t)nc->fn_data, (int)hm->uri.len, hm->uri.ptr);

            if (mg_http_match_uri(hm, "/ws/")) {
                mg_ws_upgrade(nc, hm, NULL);
                MYMPD_LOG_INFO("New Websocket connection established (%d)", (intptr_t)nc->fn_data);
                sds response = jsonrpc_notify(sdsempty(), "webserver", "info", "welcome "MYMPD_VERSION);
                mg_ws_send(nc, response, sdslen(response), WEBSOCKET_OP_TEXT);
                sdsfree(response);
            }
            else if (mg_http_match_uri(hm, "/api/script")) {
                if (config->remotescripting == false) {
                    nc->is_draining = 1;
                    send_error(nc, 403, "Remote scripting is disabled");
                    break;
                }
                if (sdslen(config->scriptacl) > 0) {
                    uint32_t remote_ip = ntohl(nc->peer.ip);
                    int allowed = mg6_check_ip_acl(config->scriptacl, remote_ip);
                    if (allowed == -1) {
                        MYMPD_LOG_ERROR("ACL malformed");
                    }
                    if (allowed != 1) {
                        nc->is_draining = 1;
                        send_error(nc, 403, "Request blocked by ACL");
                        break;
                    }
                }
                bool rc = handle_script_api((intptr_t)nc->fn_data, hm);
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
                if (getsockname((long)nc->fd, (struct sockaddr *)&localip, &len) == 0) {
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
                bool rc = handle_api((intptr_t)nc->fn_data, hm);
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
            else if (mg_http_match_uri(hm, "/albumart")) {
                handle_albumart(nc, hm, mg_user_data, config, (intptr_t)nc->fn_data);
            }
            else if (mg_http_match_uri(hm, "/pics")) {
                //serve directory
                static struct mg_http_serve_opts s_http_server_opts;
                s_http_server_opts.root_dir = mg_user_data->browse_document_root;
                s_http_server_opts.extra_headers = EXTRA_HEADERS_DIR;
                /*
                s_http_server_opts.url_rewrites = mg_user_data->rewrite_patterns;
                s_http_server_opts.custom_mime_types = CUSTOM_MIME_TYPES;
                */
                mg_http_serve_dir(nc, hm, &s_http_server_opts);
            }
            else if (mg_http_match_uri(hm, "/browse")) {
                if (config->publish == false) {
                    send_error(nc, 403, "Publishing of directories is disabled");
                }
                else if (config->webdav == false && mg_vcmp(&hm->method, "GET") == 1 && mg_vcmp(&hm->method, "HEAD") == 1) {
                    send_error(nc, 405, "Method not allowed (webdav is disabled)");
                }
                else {
                    //serve directory
                    static struct mg_http_serve_opts s_http_server_opts;
                    s_http_server_opts.root_dir = mg_user_data->browse_document_root;
                    /*
                    if (config->webdav == true) {
                        s_http_server_opts.dav_document_root = mg_user_data->browse_document_root;
                        s_http_server_opts.dav_auth_file = "-";
                    }
                    s_http_server_opts.url_rewrites = mg_user_data->rewrite_patterns;
                    
                    s_http_server_opts.enable_directory_listing = "yes";
                    s_http_server_opts.custom_mime_types = CUSTOM_MIME_TYPES;
                    */
                    s_http_server_opts.extra_headers = EXTRA_HEADERS_DIR;
                    mg_http_serve_dir(nc, hm, &s_http_server_opts);
                }
            }
            else if (mg_vcmp(&hm->uri, "/index.html") == 0) {
                mg_http_reply(nc, 301, "Location: /\r\n", NULL);
            }
            else if (mg_vcmp(&hm->uri, "/favicon.ico") == 0) {
                mg_http_reply(nc, 301, "Location: /assets/favicon.ico\r\n", NULL);
            }
            else {
                //all other uris
                #ifdef DEBUG
                //serve all files from filesystem
                static struct mg_http_serve_opts s_http_server_opts;
                s_http_server_opts.root_dir = DOC_ROOT;
                //s_http_server_opts.enable_directory_listing = "no";
                s_http_server_opts.extra_headers = EXTRA_HEADERS;
                //s_http_server_opts.custom_mime_types = CUSTOM_MIME_TYPES;
                mg_http_serve_dir(nc, hm, &s_http_server_opts);
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
            if (nc->fn_data != NULL) {
                MYMPD_LOG_INFO("HTTP connection %ld closed", (intptr_t)nc->fn_data);
                nc->fn_data = NULL;
            }
            break;
        }
        default: {
            break;
        }
    }
}

#ifdef ENABLE_SSL
static void ev_handler_redirect(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    (void)fn_data;
    if (ev == MG_EV_ACCEPT) {
        t_mg_user_data *mg_user_data = (t_mg_user_data *) nc->mgr->userdata;
        t_config *config = (t_config *) mg_user_data->config;
        //check acl
        if (sdslen(config->acl) > 0) {
            uint32_t remote_ip = ntohl(nc->peer.ip);
            int allowed = mg6_check_ip_acl(config->acl, remote_ip);
            if (allowed == -1) {
                MYMPD_LOG_ERROR("ACL malformed");
            }
            if (allowed != 1) {
                nc->is_draining = 1;
                send_error(nc, 403, "Request blocked by ACL");
                return;
            }
        }
        //increment conn_id
        if (mg_user_data->conn_id < INT_MAX) {
            mg_user_data->conn_id++;
        }
        else {
            mg_user_data->conn_id = 1;
        }
        //set conn_id
        nc->fn_data = (void *)(intptr_t)mg_user_data->conn_id;
        MYMPD_LOG_DEBUG("New connection id %d", (intptr_t)nc->fn_data);
    }
    else if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        struct mg_str *host_hdr = mg_http_get_header(hm, "Host");
        t_mg_user_data *mg_user_data = (t_mg_user_data *) nc->mgr->userdata;
        t_config *config = (t_config *) mg_user_data->config;
            
        sds host_header = sdscatlen(sdsempty(), host_hdr->ptr, (int)host_hdr->len);
        int count;
        sds *tokens = sdssplitlen(host_header, sdslen(host_header), ":", 1, &count);
            
        sds s_redirect = sdscatfmt(sdsempty(), "https://%s", tokens[0]);
        if (strcmp(config->ssl_port, "443") != 0) {
            s_redirect = sdscatfmt(s_redirect, ":%s", config->ssl_port);
        }
        MYMPD_LOG_INFO("Redirecting to %s", s_redirect);
        mg_http_reply(nc, 301, "Location: /", NULL);
        sdsfreesplitres(tokens, count);
        sdsfree(host_header);
    }
}
#endif

static bool handle_api(int conn_id, struct mg_http_message *hm) {
    if (hm->body.len > 2048) {
        MYMPD_LOG_ERROR("Request length of %d exceeds max request size, discarding request)", hm->body.len);
        return false;
    }
    
    MYMPD_LOG_DEBUG("API request (%d): %.*s", conn_id, hm->body.len, hm->body.ptr);
    char *cmd = NULL;
    char *jsonrpc = NULL;
    int id = 0;
    const int je = json_scanf(hm->body.ptr, hm->body.len, "{jsonrpc: %Q, method: %Q, id: %d}", &jsonrpc, &cmd, &id);
    if (je < 3) {
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }
    MYMPD_LOG_INFO("API request (%d): %s", conn_id, cmd);

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

static bool handle_script_api(int conn_id, struct mg_http_message *hm) {
    if (hm->body.len > 4096) {
        MYMPD_LOG_ERROR("Request length of %d exceeds max request size, discarding request)", hm->body.len);
        return false;
    }
    
    MYMPD_LOG_DEBUG("Script API request (%d): %.*s", conn_id, hm->body.len, hm->body.ptr);
    char *cmd = NULL;
    char *jsonrpc = NULL;
    long id = 0;
    const int je = json_scanf(hm->body.ptr, hm->body.len, "{jsonrpc: %Q, method: %Q, id: %ld}", &jsonrpc, &cmd, &id);
    if (je < 3) {
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }
    MYMPD_LOG_INFO("Script API request (%d): %s", conn_id, cmd);

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
