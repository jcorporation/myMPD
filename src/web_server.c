/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
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

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "api.h"
#include "utility.h"
#include "log.h"
#include "list.h"
#include "tiny_queue.h"
#include "config_defs.h"
#include "global.h"
#include "web_server.h"
#include "mpd_client.h"
#include "plugins.h"
#include "../dist/src/mongoose/mongoose.h"
#include "../dist/src/frozen/frozen.h"

#define EXTRA_HEADERS_DIR "Content-Security-Policy: default-src 'none'; "\
                          "style-src 'self' 'unsafe-inline'; font-src 'self'; script-src 'self' 'unsafe-inline'; img-src 'self' data:; "\
                          "connect-src 'self' ws: wss:; manifest-src 'self'; "\
                          "media-src *; frame-ancestors 'none'; base-uri 'none';\r\n"\
                          "X-Content-Type-Options: nosniff\r\n"\
                          "X-XSS-Protection: 1; mode=block\r\n"\
                          "X-Frame-Options: deny"

#define EXTRA_HEADERS "Content-Security-Policy: default-src 'none'; "\
                      "style-src 'self'; font-src 'self'; script-src 'self'; img-src 'self' data:; "\
                      "connect-src 'self' ws: wss:; manifest-src 'self'; "\
                      "media-src *; frame-ancestors 'none'; base-uri 'none';\r\n"\
                      "X-Content-Type-Options: nosniff\r\n"\
                      "X-XSS-Protection: 1; mode=block\r\n"\
                      "X-Frame-Options: deny"

#ifndef DEBUG
//embedded files for release build
#include "web_server_embedded_files.c"
#endif

//private definitions
static bool parse_internal_message(t_work_result *response, t_mg_user_data *mg_user_data);
static int is_websocket(const struct mg_connection *nc);
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data);
static void ev_handler_redirect(struct mg_connection *nc_http, int ev, void *ev_data);
static void send_ws_notify(struct mg_mgr *mgr, t_work_result *response);
static void send_api_response(struct mg_mgr *mgr, t_work_result *response);
static bool handle_api(int conn_id, const char *request, int request_len);
static bool handle_coverextract(struct mg_connection *nc, struct http_message *hm, t_config *config, sds media_file);
static bool handle_albumart(struct mg_connection *nc, struct http_message *hm, t_mg_user_data *mg_user_data, t_config *config, int conn_id);
static void serve_na_image(struct mg_connection *nc, struct http_message *hm);
static void serve_stream_image(struct mg_connection *nc, struct http_message *hm);
static void serve_asset_image(struct mg_connection *nc, struct http_message *hm, const char *name);
static void send_error(struct mg_connection *nc, int code, const char *msg);

//public functions
bool web_server_init(void *arg_mgr, t_config *config, t_mg_user_data *mg_user_data) {
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    struct mg_connection *nc_https;
    struct mg_connection *nc_http;
    struct mg_bind_opts bind_opts_https;
    struct mg_bind_opts bind_opts_http;
    const char *err_https;
    const char *err_http;

    //initialize mgr user_data, malloced in main.c
    mg_user_data->config = config;
    mg_user_data->music_directory = sdsempty();
    mg_user_data->rewrite_patterns = sdsempty();
    mg_user_data->coverimage_name = sdsdup(config->coverimage_name);
    mg_user_data->conn_id = 1;
    mg_user_data->feat_library = false;
    mg_user_data->feat_mpd_albumart = false;
    
    //init monogoose mgr with mg_user_data
    mg_mgr_init(mgr, mg_user_data);
    
    //bind to webport
    memset(&bind_opts_http, 0, sizeof(bind_opts_http));
    bind_opts_http.error_string = &err_http;
    if (config->ssl == true) {
        nc_http = mg_bind_opt(mgr, config->webport, ev_handler_redirect, bind_opts_http);
    }
    else {
        nc_http = mg_bind_opt(mgr, config->webport, ev_handler, bind_opts_http);
    }
    if (nc_http == NULL) {
        LOG_ERROR("Can't bind to port %s: %s", config->webport, err_http);
        mg_mgr_free(mgr);
        return false;
    }
    mg_set_protocol_http_websocket(nc_http);
    LOG_INFO("Listening on http port %s", config->webport);

    //bind to sslport
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
    bool feat_library;
    bool feat_mpd_albumart;
    int je = json_scanf(response->data, sdslen(response->data), "{musicDirectory: %Q, coverimageName: %Q, featLibrary: %B, featMpdAlbumart: %B}", 
        &p_charbuf1, &p_charbuf2, &feat_library, &feat_mpd_albumart);
    if (je == 4) {
        mg_user_data->music_directory = sdsreplace(mg_user_data->music_directory, p_charbuf1);
        mg_user_data->coverimage_name = sdsreplace(mg_user_data->coverimage_name, p_charbuf2);
        mg_user_data->feat_library = feat_library;
        mg_user_data->feat_mpd_albumart = feat_mpd_albumart;
        mg_user_data->rewrite_patterns = sdscrop(mg_user_data->rewrite_patterns);
        if (feat_library == true) {
            mg_user_data->rewrite_patterns = sdscatfmt(mg_user_data->rewrite_patterns, "/library/=%s", mg_user_data->music_directory);
            LOG_DEBUG("Setting music_directory to %s", mg_user_data->music_directory);
        }
        LOG_DEBUG("Setting rewrite_patterns to %s", mg_user_data->rewrite_patterns);
    }
    else {
        LOG_WARN("Unknown internal message: %s", response->data);
        return false;
    }
    FREE_PTR(p_charbuf1);
    FREE_PTR(p_charbuf2);
    sdsfree(response->data);
    FREE_PTR(response);
    return true;
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
    sdsfree(response->data);
    FREE_PTR(response);
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
                mg_send_head(nc, 200, sdslen(response->data), "Content-Type: application/json");
                mg_send(nc, response->data, sdslen(response->data));
                break;
            }
        }
        else {
            LOG_DEBUG("Unknown connection");
        }
    }
    sdsfree(response->data);
    FREE_PTR(response);
}

static void send_error(struct mg_connection *nc, int code, const char *msg) {
    sds errorpage = sdscatfmt(sdsempty(), "<html><head><title>myMPD error</title></head><body>"
        "<h1>myMPD error</h1>"
        "<p>%s</p>"
        "</body></html>",
        msg);
    mg_send_head(nc, code, sdslen(errorpage), "Content-Type: text/html");
    mg_send(nc, errorpage, sdslen(errorpage));
    sdsfree(errorpage);
    if (code >= 400) {
        LOG_ERROR(msg);
    }
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
            static const struct mg_str library_prefix = MG_MK_STR("/library");
            static const struct mg_str albumart_prefix = MG_MK_STR("/albumart");
            LOG_VERBOSE("HTTP request (%d): %.*s", (intptr_t)nc->user_data, (int)hm->uri.len, hm->uri.p);
            if (mg_vcmp(&hm->uri, "/api") == 0) {
                //api request
                bool rc = handle_api((intptr_t)nc->user_data, hm->body.p, hm->body.len);
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
            else if (mg_str_starts_with(hm->uri, albumart_prefix) == 1) {
                handle_albumart(nc, hm, mg_user_data, config, (intptr_t)nc->user_data);
            }
            else if (mg_str_starts_with(hm->uri, library_prefix) == 1) {
                if (config->publish_library == false) {
                    send_error(nc, 403, "Publishing of music directory is disabled");
                }
                else if (mg_user_data->feat_library == true) {
                    //serve directory
                    static struct mg_serve_http_opts s_http_server_opts;
                    s_http_server_opts.document_root = DOC_ROOT;
                    s_http_server_opts.url_rewrites = mg_user_data->rewrite_patterns;
                    s_http_server_opts.enable_directory_listing = "yes";
                    s_http_server_opts.extra_headers = EXTRA_HEADERS_DIR;
                    mg_serve_http(nc, hm, s_http_server_opts);
                }
                else {
                    send_error(nc, 404, "Unknown music_directory");
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

static bool handle_api(int conn_id, const char *request_body, int request_len) {
    if (request_len > 1000) {
        LOG_ERROR("Request to long, discarding)");
        return false;
    }
    
    LOG_VERBOSE("API request (%d): %.*s", conn_id, request_len, request_body);
    char *cmd = NULL;
    char *jsonrpc = NULL;
    int id = 0;
    const int je = json_scanf(request_body, request_len, "{jsonrpc: %Q, method: %Q, id: %d}", &jsonrpc, &cmd, &id);
    if (je < 3) {
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }

    enum mympd_cmd_ids cmd_id = get_cmd_id(cmd);
    if (cmd_id == 0 || strncmp(jsonrpc, "2.0", 3) != 0) {
        FREE_PTR(cmd);
        FREE_PTR(jsonrpc);
        return false;
    }
    
    t_work_request *request = (t_work_request*)malloc(sizeof(t_work_request));
    assert(request);
    request->conn_id = conn_id;
    request->cmd_id = cmd_id;
    request->id = id;
    request->method = sdscat(sdsempty(), cmd);
    request->data = sdscatlen(sdsempty(), request_body, request_len);
    
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

static void serve_na_image(struct mg_connection *nc, struct http_message *hm) {
    serve_asset_image(nc, hm, "coverimage-notavailable.svg");
}

static void serve_stream_image(struct mg_connection *nc, struct http_message *hm) {
    serve_asset_image(nc, hm, "coverimage-stream.svg");
}

static void serve_asset_image(struct mg_connection *nc, struct http_message *hm, const char *name) {
    #ifdef DEBUG
    sds na_image = sdscatfmt(sdsempty(), "%s/assets/%s", DOC_ROOT, name);
    mg_http_serve_file(nc, hm, na_image, mg_mk_str("image/svg+xml"), mg_mk_str(""));
    #else
    sds na_image = sdscatfmt(sdsempty(), "/assets/%s", name);
    serve_embedded_files(nc, na_image, hm);
    #endif
    LOG_DEBUG("Serving file %s (%s)", na_image, "image/svg+xml");
    sdsfree(na_image);
}

//returns true if an image is served
//returns false if waiting for mpd_client to handle request
static bool handle_albumart(struct mg_connection *nc, struct http_message *hm, t_mg_user_data *mg_user_data, t_config *config, int conn_id) {
    //decode uri
    sds uri_decoded = sdsurldecode(sdsempty(), hm->uri.p, (int)hm->uri.len, 0);
    if (sdslen(uri_decoded) == 0) {
        LOG_ERROR("Failed to decode uri");
        serve_na_image(nc, hm);
        sdsfree(uri_decoded);
        return true;
    }
    if (validate_uri(uri_decoded) == false) {
        LOG_ERROR("Invalid uri: %s", uri_decoded);
        serve_na_image(nc, hm);
        sdsfree(uri_decoded);
        return true;
    }
    //try image in /pics folder, if uri contains ://
    if (strstr(uri_decoded, "://") != NULL) {
        char *name = strstr(uri_decoded, "://");
        if (strlen(name) < 4) {
            LOG_ERROR("Uri to short");
            serve_na_image(nc, hm);
            sdsfree(uri_decoded);
            return true;
        }
        name += 3;
        replacechar(name, '/', '_');
        replacechar(name, '.', '_');
        replacechar(name, ':', '_');
        sds coverfile = sdscatfmt(sdsempty(), "%s/pics/%s", config->varlibdir, name);
        LOG_DEBUG("Check for stream cover %s", coverfile);
        coverfile = find_image_file(coverfile);
        
        if (sdslen(coverfile) > 0) {
            sds mime_type = get_mime_type_by_ext(coverfile);
            LOG_DEBUG("Serving file %s (%s)", coverfile, mime_type);
            mg_http_serve_file(nc, hm, coverfile, mg_mk_str(mime_type), mg_mk_str(""));
            sdsfree(mime_type);
        }
        else {
            serve_stream_image(nc, hm);
        }
        sdsfree(coverfile);
        sdsfree(uri_decoded);
        return true;
    }
    //remove /albumart/
    sdsrange(uri_decoded, 10, -1);
    //create absolute file
    sds mediafile = sdscatfmt(sdsempty(), "%s/%s", mg_user_data->music_directory, uri_decoded);
    LOG_DEBUG("Absolut media_file: %s", mediafile);
    //check covercache
    sds covercachefile = sdscatfmt(sdsempty(), "%s/covercache/%s", config->varlibdir, uri_decoded);
    replacechar(covercachefile, '/', '_');
    covercachefile = find_image_file(covercachefile);
    if (sdslen(covercachefile) > 0) {
        sds mime_type = get_mime_type_by_ext(covercachefile);
        LOG_DEBUG("Serving file %s (%s)", covercachefile, mime_type);
        mg_http_serve_file(nc, hm, covercachefile, mg_mk_str(mime_type), mg_mk_str(""));
        sdsfree(uri_decoded);
        sdsfree(covercachefile);
        sdsfree(mediafile);
        sdsfree(mime_type);
        return true;
    }
    else {
        sdsfree(covercachefile);
    }
    //check music_directory folder
    if (mg_user_data->feat_library == true && access(mediafile, F_OK) == 0) {
        //try image in folder under music_directory
        char *path = uri_decoded;
        dirname(path);
        sds coverfile = sdscatfmt(sdsempty(), "%s/%s/%s", mg_user_data->music_directory, path, mg_user_data->coverimage_name);
        LOG_DEBUG("Check for cover %s", coverfile);
        if (access(coverfile, F_OK ) == 0) { /* Flawfinder: ignore */
            //todo: get mime_type
            LOG_DEBUG("Serving file %s (%s)", coverfile, "image/jpeg");
            mg_http_serve_file(nc, hm, coverfile, mg_mk_str("image/jpeg"), mg_mk_str(""));
            sdsfree(uri_decoded);
            sdsfree(coverfile);
            sdsfree(mediafile);
            return true;
        }
        sdsfree(coverfile);
        //try coverextract plugin
        if (config->plugins_coverextract == true) {
            LOG_DEBUG("Exctracting coverimage from %s/%s", mg_user_data->music_directory, mediafile);
            bool rc = handle_coverextract(nc, hm, config, uri_decoded);
            if (rc == true) {
                sdsfree(uri_decoded);
                sdsfree(mediafile);
                return true;
            }
        }
    }
    //ask mpd
    else if (mg_user_data->feat_library == false && mg_user_data->feat_mpd_albumart == true) {
        LOG_DEBUG("Sending getalbumart to mpd_client_queue");
/*
    t_work_request *request = (t_work_request*)malloc(sizeof(t_work_request));
    assert(request);
    request->conn_id = conn_id;
    request->cmd_id = cmd_id;
    request->id = id;
    request->method = sdscat(sdsempty(), cmd);
    request->data = sdscatlen(sdsempty(), request_body, request_len);
    tiny_queue_push(mpd_client_queue, request);
    sdsfree(mediafile);
    sdsfree(uri_decoded);
    return false;
*/
    }
    (void) conn_id;

    LOG_VERBOSE("No coverimage found for %s", mediafile);
    sdsfree(mediafile);
    sdsfree(uri_decoded);
    serve_na_image(nc, hm);
    return true;
}

static bool handle_coverextract(struct mg_connection *nc, struct http_message *hm, t_config *config, sds media_file) {
    size_t image_mime_type_len = 100;
    char image_mime_type[image_mime_type_len]; /* Flawfinder: ignore */
    size_t image_file_len = 1500;
    char image_file[image_file_len]; /* Flawfinder: ignore */
    
    sds cache_dir = sdscatfmt(sdsempty(), "%s/covercache", config->varlibdir);

    bool rc = plugin_coverextract(media_file, cache_dir, image_file, image_file_len, image_mime_type, image_mime_type_len, true);
    sdsfree(cache_dir);
    if (rc == true) {
        sds path = sdscatfmt(sdsempty(), "%s/covercache/%s", config->varlibdir, image_file);
        LOG_DEBUG("Serving file %s (%s)", path, image_mime_type);
        mg_http_serve_file(nc, hm, path, mg_mk_str(image_mime_type), mg_mk_str(""));
        sdsfree(path);
    }
    return rc;
}
