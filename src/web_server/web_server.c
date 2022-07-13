/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "web_server.h"

#include "../lib/api.h"
#include "../lib/http_client.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mympd_pin.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "web_server_albumart.h"
#include "web_server_handler.h"
#include "web_server_proxy.h"
#include "web_server_tagart.h"

#include <sys/prctl.h>

//private definitions
static bool parse_internal_message(struct t_work_result *response, struct t_mg_user_data *mg_user_data);
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data);
#ifdef ENABLE_SSL
    static void ev_handler_redirect(struct mg_connection *nc_http, int ev, void *ev_data, void *fn_data);
#endif
static void send_ws_notify(struct mg_mgr *mgr, struct t_work_result *response);
static void send_api_response(struct mg_mgr *mgr, struct t_work_result *response);
static bool check_acl(struct mg_connection *nc, sds acl);

//public functions
bool web_server_init(struct mg_mgr *mgr, struct t_config *config, struct t_mg_user_data *mg_user_data) {
    //initialize mgr user_data, malloced in main.c
    mg_user_data->config = config;
    mg_user_data->browse_directory = sdscatfmt(sdsempty(), "%S/empty", config->workdir);
    mg_user_data->music_directory = sdsempty();
    sds default_coverimage_names = sdsnew(MYMPD_COVERIMAGE_NAMES);
    mg_user_data->coverimage_names= sds_split_comma_trim(default_coverimage_names, &mg_user_data->coverimage_names_len);
    FREE_SDS(default_coverimage_names);
    sds default_thumbnail_names = sdsnew(MYMPD_THUMBNAIL_NAMES);
    mg_user_data->thumbnail_names= sds_split_comma_trim(default_thumbnail_names, &mg_user_data->thumbnail_names_len);
    FREE_SDS(default_thumbnail_names);
    mg_user_data->publish_music = false;
    mg_user_data->publish_playlists = false;
    mg_user_data->feat_mpd_albumart = false;
    mg_user_data->connection_count = 0;
    mg_user_data->stream_uri = sdsnew("http://localhost:8000");
    mg_user_data->covercache = true;
    list_init(&mg_user_data->session_list);

    //init monogoose mgr
    mg_mgr_init(mgr);
    mgr->userdata = mg_user_data;
    mgr->product_name = "myMPD "MYMPD_VERSION;
    mgr->directory_listing_css = DIRECTORY_LISTING_CSS;
    //set dns server
    mgr->dns4.url = get_dnsserver();
    MYMPD_LOG_DEBUG("Setting dns server to %s", mgr->dns4.url);

    //bind to http_port
    struct mg_connection *nc_http = NULL;
    sds http_url = sdscatfmt(sdsempty(), "http://%S:%S", config->http_host, config->http_port);
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
    FREE_SDS(http_url);
    if (nc_http == NULL) {
        MYMPD_LOG_EMERG("Can't bind to http://%s:%s", config->http_host, config->http_port);
        return false;
    }
    MYMPD_LOG_NOTICE("Listening on http://%s:%s", config->http_host, config->http_port);

    //bind to ssl_port
    #ifdef ENABLE_SSL
    if (config->ssl == true) {
        sds https_url = sdscatfmt(sdsempty(), "https://%S:%S", config->http_host, config->ssl_port);
        struct mg_connection *nc_https = mg_http_listen(mgr, https_url, ev_handler, NULL);
        FREE_SDS(https_url);
        if (nc_https == NULL) {
            MYMPD_LOG_ERROR("Can't bind to https://%s:%s", config->http_host, config->ssl_port);
            return false;
        }
        MYMPD_LOG_NOTICE("Listening on https://%s:%s", config->http_host, config->ssl_port);
    }
    #endif
    MYMPD_LOG_NOTICE("Serving files from \"%s\"", DOC_ROOT);
    return mgr;
}

void *web_server_free(struct mg_mgr *mgr) {
    sds dns4_url = (sds)mgr->dns4.url;
    FREE_SDS(dns4_url);
    mg_mgr_free(mgr);
    FREE_PTR(mgr);
    return NULL;
}

void *web_server_loop(void *arg_mgr) {
    thread_logname = sds_replace(thread_logname, "webserver");
    prctl(PR_SET_NAME, thread_logname, 0, 0, 0);
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) mgr->userdata;

    //set mongoose loglevel
    #ifdef DEBUG
    mg_log_set("1");
    #endif

    #ifdef ENABLE_SSL
    MYMPD_LOG_DEBUG("Using certificate: %s", mg_user_data->config->ssl_cert);
    MYMPD_LOG_DEBUG("Using private key: %s", mg_user_data->config->ssl_key);
    #endif

    sds last_notify = sdsempty();
    time_t last_time = 0;
    while (s_signal_received == 0) {
        struct t_work_result *response = mympd_queue_shift(web_server_queue, 50, 0);
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
                if (strcmp(response->data, last_notify) != 0 ||
                    last_time < now - 1)
                {
                    last_notify = sds_replace(last_notify, response->data);
                    last_time = now;
                    send_ws_notify(mgr, response);
                }
                else {
                    free_result(response);
                }
            }
            else {
                MYMPD_LOG_DEBUG("Got API response for id \"%lld\"", response->conn_id);
                //api response
                send_api_response(mgr, response);
            }
        }
        //webserver polling
        mg_mgr_poll(mgr, 50);
    }
    FREE_SDS(thread_logname);
    FREE_SDS(last_notify);
    return NULL;
}

//private functions
static bool parse_internal_message(struct t_work_result *response, struct t_mg_user_data *mg_user_data) {
    bool rc = false;
    if (response->extra != NULL) {
        struct set_mg_user_data_request *new_mg_user_data = (struct set_mg_user_data_request *)response->extra;
        struct t_config *config = mg_user_data->config;

        sdsclear(mg_user_data->browse_directory);
        mg_user_data->browse_directory = sdscatfmt(mg_user_data->browse_directory, "%S/empty", config->workdir);
        mg_user_data->browse_directory = sdscatfmt(mg_user_data->browse_directory, ",/browse/pics=%S/pics", config->workdir);
        mg_user_data->browse_directory = sdscatfmt(mg_user_data->browse_directory, ",/browse/smartplaylists=%S/smartpls", config->workdir);
        mg_user_data->browse_directory = sdscatfmt(mg_user_data->browse_directory, ",/browse/webradios=%S/webradios", config->workdir);
        if (sdslen(new_mg_user_data->playlist_directory) > 0) {
            mg_user_data->browse_directory = sdscatfmt(mg_user_data->browse_directory, ",/browse/playlists=%S", new_mg_user_data->playlist_directory);
            mg_user_data->publish_playlists = true;
        }
        else {
            mg_user_data->publish_playlists = false;
        }
        FREE_SDS(new_mg_user_data->playlist_directory);

        if (sdslen(new_mg_user_data->music_directory) > 0)
        {
            mg_user_data->browse_directory = sdscatfmt(mg_user_data->browse_directory, ",/browse/music=%S", new_mg_user_data->music_directory);
            mg_user_data->publish_music = true;
        }
        else {
            mg_user_data->publish_music = false;
        }
        mg_user_data->music_directory = sds_replace(mg_user_data->music_directory, new_mg_user_data->music_directory);
        FREE_SDS(new_mg_user_data->music_directory);
        MYMPD_LOG_DEBUG("Document root: \"%s\"", mg_user_data->browse_directory);

        sdsfreesplitres(mg_user_data->coverimage_names, mg_user_data->coverimage_names_len);
        mg_user_data->coverimage_names = sds_split_comma_trim(new_mg_user_data->coverimage_names, &mg_user_data->coverimage_names_len);
        FREE_SDS(new_mg_user_data->coverimage_names);

        sdsfreesplitres(mg_user_data->thumbnail_names, mg_user_data->thumbnail_names_len);
        mg_user_data->thumbnail_names = sds_split_comma_trim(new_mg_user_data->thumbnail_names, &mg_user_data->thumbnail_names_len);
        FREE_SDS(new_mg_user_data->thumbnail_names);

        mg_user_data->feat_mpd_albumart = new_mg_user_data->feat_mpd_albumart;
        mg_user_data->covercache = new_mg_user_data->covercache;

        sdsclear(mg_user_data->stream_uri);
        if (new_mg_user_data->mpd_stream_port != 0) {
            mg_user_data->stream_uri = sdscatfmt(mg_user_data->stream_uri, "http://%s:%u",
                (strncmp(new_mg_user_data->mpd_host, "/", 1) == 0 ? "127.0.0.1" : new_mg_user_data->mpd_host),
                new_mg_user_data->mpd_stream_port);
        }
        FREE_SDS(new_mg_user_data->mpd_host);
	    FREE_PTR(response->extra);
        rc = true;
    }
    else {
        MYMPD_LOG_WARN("Invalid internal message: %s", response->data);
    }
    free_result(response);
    return rc;
}

static void send_ws_notify(struct mg_mgr *mgr, struct t_work_result *response) {
    struct mg_connection *nc = mgr->conns;
    int send_count = 0;
    int conn_count = 0;
    while (nc != NULL) {
        if ((int)nc->is_websocket == 1) {
            MYMPD_LOG_DEBUG("Sending notify to conn_id %lu: %s", nc->id, response->data);
            mg_ws_send(nc, response->data, sdslen(response->data), WEBSOCKET_OP_TEXT);
            send_count++;
        }
        nc = nc->next;
        conn_count++;
    }
    if (send_count == 0) {
        MYMPD_LOG_DEBUG("No websocket client connected, discarding message: %s", response->data);
    }
    free_result(response);
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) mgr->userdata;
    if (conn_count != mg_user_data->connection_count) {
        MYMPD_LOG_DEBUG("Correcting connection count from %d to %d", mg_user_data->connection_count, conn_count);
        mg_user_data->connection_count = conn_count;
    }
}

static void send_api_response(struct mg_mgr *mgr, struct t_work_result *response) {
    struct mg_connection *nc = mgr->conns;
    while (nc != NULL) {
        if ((int)nc->is_websocket == 0 && nc->id == (long unsigned)response->conn_id) {
            if (response->cmd_id == INTERNAL_API_ALBUMART) {
                webserver_albumart_send(nc, response->data, response->binary);
            }
            else {
                MYMPD_LOG_DEBUG("Sending response to conn_id %lu (length: %lu): %s", nc->id, (unsigned long)sdslen(response->data), response->data);
                webserver_send_data(nc, response->data, sdslen(response->data), "Content-Type: application/json\r\n");
            }
            break;
        }
        nc = nc->next;
    }
    free_result(response);
}

static bool check_acl(struct mg_connection *nc, sds acl) {
    if (sdslen(acl) == 0) {
        return true;
    }
    if (nc->rem.is_ip6 == true) {
        //acls for ipv6 is not implemented in mongoose
        return true;
    }
    int acl_result = mg_check_ip_acl(mg_str(acl), nc->rem.ip);
    MYMPD_LOG_DEBUG("Check against acl \"%s\": %d", acl, acl_result);
    if (acl_result == 1) {
        return true;
    }
    if (acl_result < 0) {
        MYMPD_LOG_ERROR("Malformed acl \"%s\"", acl);
        return false;
    }

    char buf[INET6_ADDRSTRLEN];
    mg_straddr(&nc->rem, buf, INET6_ADDRSTRLEN);
    MYMPD_LOG_ERROR("Connection from \"%s\" blocked by ACL", buf);
    webserver_send_error(nc, 403, "Request blocked by ACL");
    nc->is_draining = 1;
    return false;
}

//nc->label usage
//0 - connection type: F = frontend connection, B = backend connection
//1 - http method: G = GET, H = HEAD, P = POST
//2 - connection header: C = close, K = keepalive

// Event handler
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    //initial connection specific data structure
    struct mg_connection *backend_nc = fn_data;
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_config *config = mg_user_data->config;
    switch(ev) {
        case MG_EV_ACCEPT: {
            mg_user_data->connection_count++;
            //ssl support
            #ifdef ENABLE_SSL
            if (config->ssl == true) {
                MYMPD_LOG_DEBUG("Init tls with cert \"%s\" and key \"%s\" for connection \"%lu\"", config->ssl_cert, config->ssl_key, nc->id);
                struct mg_tls_opts tls_opts = {
                    .cert = config->ssl_cert,
                    .certkey = config->ssl_key
                };
                mg_tls_init(nc, &tls_opts);
            }
            #endif
            //check connection count
            if (mg_user_data->connection_count > HTTP_CONNECTIONS_MAX) {
                MYMPD_LOG_DEBUG("Connections: %d", mg_user_data->connection_count);
                MYMPD_LOG_ERROR("Concurrent connections limit exceeded: %d", mg_user_data->connection_count);
                webserver_send_error(nc, 429, "Concurrent connections limit exceeded");
                nc->is_draining = 1;
                break;
            }
            //check acl
            if (check_acl(nc, config->acl) == false) {
                break;
            }
            if (loglevel == LOG_DEBUG) {
                char buf[INET6_ADDRSTRLEN];
                mg_straddr(&nc->rem, buf, INET6_ADDRSTRLEN);
                MYMPD_LOG_DEBUG("New connection id \"%lu\" from %s, connections: %d", nc->id, buf, mg_user_data->connection_count);
            }
            //set labels
            nc->label[0] = 'F';
            nc->label[1] = '-';
            nc->label[2] = '-';
            break;
        }
        case MG_EV_WS_MSG: {
            struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
            MYMPD_LOG_DEBUG("WS message (%lu): %.*s", nc->id, (int)wm->data.len, wm->data.ptr);
            if (strncmp(wm->data.ptr, "ping", wm->data.len) == 0) {
                size_t sent = mg_ws_send(nc, "pong", 4, WEBSOCKET_OP_TEXT);
                if (sent != 6) {
                    MYMPD_LOG_WARN("WS could not reply with pong, closing connection");
                    nc->is_closing = 1;
                }
            }
            break;
        }
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;
            MYMPD_LOG_INFO("HTTP request (%lu): %.*s %.*s", nc->id, (int)hm->method.len, hm->method.ptr, (int)hm->uri.len, hm->uri.ptr);
            //limit allowed http methods
            if (strncmp(hm->method.ptr, "GET", hm->method.len) == 0) {
                nc->label[1] = 'G';
            }
            else if (strncmp(hm->method.ptr, "HEAD", hm->method.len) == 0) {
                nc->label[1] = 'H';
            }
            else if (strncmp(hm->method.ptr, "POST", hm->method.len) == 0) {
                nc->label[1] = 'P';
            }
            else {
                MYMPD_LOG_ERROR("Invalid http method \"%.*s\" (%lu)", (int)hm->method.len, hm->method.ptr, nc->id);
                webserver_send_error(nc, 405, "Invalid http method");
                nc->is_draining = 1;
                return;
            }
            //check uri length
            if (hm->uri.len > URI_LENGTH_MAX) {
                MYMPD_LOG_ERROR("Uri is too long, length is %lu, maximum length is %d (%lu)", (unsigned long)hm->uri.len, URI_LENGTH_MAX, nc->id);
                webserver_send_error(nc, 414, "Uri is too long");
                nc->is_draining = 1;
                return;
            }
            //check post requests length
            if (nc->label[1] == 'P' && (hm->body.len == 0 || hm->body.len > BODY_SIZE_MAX)) {
                MYMPD_LOG_ERROR("POST request with body of size %lu is out of bounds (%lu)", (unsigned long)hm->body.len, nc->id);
                webserver_send_error(nc, 413, "Post request is too large");
                nc->is_draining = 1;
                return;
            }
            //respect connection close header
            struct mg_str *connection_hdr = mg_http_get_header(hm, "Connection");
            if (connection_hdr != NULL) {
                if (strncmp(connection_hdr->ptr, "close", connection_hdr->len) == 0) {
                    MYMPD_LOG_DEBUG("Connection: close header found (%lu)", nc->id);
                    nc->label[2] = 'C';
                }
                else if (strncmp(connection_hdr->ptr, "keep-alive", connection_hdr->len) == 0) {
                    MYMPD_LOG_DEBUG("Connection: keepalive header found (%lu)", nc->id);
                    nc->label[2] = 'K';
                }
            }
            //handle uris
            if (mg_http_match_uri(hm, "/api/")) {
                //api request
                sds body = sdsnewlen(hm->body.ptr, hm->body.len);
                struct mg_str *auth_header = mg_http_get_header(hm, "Authorization");
                bool rc = webserver_api_handler(nc, body, auth_header, mg_user_data, backend_nc);
                FREE_SDS(body);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Invalid API request");
                    sds response = jsonrpc_respond_message(sdsempty(), "", 0, true,
                        "general", "error", "Invalid API request");
                    webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
                    FREE_SDS(response);
                }
            }
            else if (mg_http_match_uri(hm, "/albumart-thumb")) {
                webserver_albumart_handler(nc, hm, mg_user_data, config, (long long)nc->id, ALBUMART_THUMBNAIL);
            }
            else if (mg_http_match_uri(hm, "/albumart")) {
                webserver_albumart_handler(nc, hm, mg_user_data, config, (long long)nc->id, ALBUMART_FULL);
            }
            else if (mg_http_match_uri(hm, "/tagart")) {
                webserver_tagart_handler(nc, hm, mg_user_data);
            }
            else if (mg_http_match_uri(hm, "/browse/#")) {
                webserver_browse_handler(nc, hm, mg_user_data);
            }
            else if (mg_http_match_uri(hm, "/ws/")) {
                mg_ws_upgrade(nc, hm, NULL);
                MYMPD_LOG_INFO("New Websocket connection established (%lu)", nc->id);
                sds response = jsonrpc_event(sdsempty(), "welcome");
                mg_ws_send(nc, response, sdslen(response), WEBSOCKET_OP_TEXT);
                FREE_SDS(response);
            }
            else if (mg_http_match_uri(hm, "/stream/")) {
                if (sdslen(mg_user_data->stream_uri) == 0) {
                    webserver_send_error(nc, 404, "MPD stream port not configured");
                    nc->is_draining = 1;
                    break;
                }
                create_backend_connection(nc, backend_nc, mg_user_data->stream_uri, forward_backend_to_frontend);
            }
            else if (mg_http_match_uri(hm, "/proxy")) {
                //Makes a get request to the defined uri and returns the response
                webserver_proxy_handler(nc, hm, backend_nc);
            }
            else if (mg_http_match_uri(hm, "/api/serverinfo")) {
                webserver_serverinfo_handler(nc);
            }
            else if (mg_http_match_uri(hm, "/api/script")) {
                //check acl
                if (check_acl(nc, config->scriptacl) == false) {
                    break;
                }
                sds body = sdsnewlen(hm->body.ptr, hm->body.len);
                bool rc = webserver_script_api_handler((long long)nc->id, body);
                FREE_SDS(body);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Invalid script API request");
                    sds response = jsonrpc_respond_message(sdsempty(), "", 0, true,
                        "script", "error", "Invalid script API request");
                    webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
                    FREE_SDS(response);
                }
            }
            else if (mg_vcmp(&hm->uri, "/index.html") == 0) {
                webserver_send_header_redirect(nc, "/");
            }
            else if (mg_vcmp(&hm->uri, "/favicon.ico") == 0) {
                webserver_send_header_redirect(nc, "/assets/favicon.ico");
            }
            #ifdef ENABLE_SSL
            else if (mg_http_match_uri(hm, "/ca.crt")) {
                webserver_ca_handler(nc, hm, mg_user_data, config);
            }
            #endif
            else {
                //all other uris
                #ifndef EMBEDDED_ASSETS
                    //serve all files from filesystem
                    static struct mg_http_serve_opts s_http_server_opts;
                    s_http_server_opts.root_dir = DOC_ROOT;
                    if (mg_http_match_uri(hm, "/test/#")) {
                        //test suite uses innerHTML
                        s_http_server_opts.extra_headers = EXTRA_HEADERS_UNSAFE;
                    }
                    else {
                        s_http_server_opts.extra_headers = EXTRA_HEADERS_SAFE;
                    }
                    s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
                    mg_http_serve_dir(nc, hm, &s_http_server_opts);
                #else
                    //serve embedded files
                    sds uri = sdsnewlen(hm->uri.ptr, hm->uri.len);
                    webserver_serve_embedded_files(nc, uri);
                    FREE_SDS(uri);
                #endif
            }
            break;
        }
        case MG_EV_CLOSE: {
            MYMPD_LOG_INFO("HTTP connection %lu closed", nc->id);
            mg_user_data->connection_count--;
            if (backend_nc != NULL) {
                MYMPD_LOG_INFO("Closing backend connection \"%lu\"", backend_nc->id);
                //remove pointer to frontend connection
                struct backend_nc_data_t *backend_nc_data = (struct backend_nc_data_t *)backend_nc->fn_data;
                backend_nc_data->frontend_nc = NULL;
                //close backend connection
                backend_nc->is_closing = 1;
            }
            break;
        }
    }
}

#ifdef ENABLE_SSL
static void ev_handler_redirect(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    (void)fn_data;
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_config *config = mg_user_data->config;

    switch(ev) {
        case MG_EV_ACCEPT:
            mg_user_data->connection_count++;
            //check connection count
            if (mg_user_data->connection_count > HTTP_CONNECTIONS_MAX) {
                MYMPD_LOG_DEBUG("Connections: %d", mg_user_data->connection_count);
                MYMPD_LOG_ERROR("Concurrent connections limit exceeded: %d", mg_user_data->connection_count);
                webserver_send_error(nc, 429, "Concurrent connections limit exceeded");
                nc->is_draining = 1;
                break;
            }
            //check acl
            if (check_acl(nc, config->acl) == false) {
                break;
            }
            break;
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;
            if (mg_http_match_uri(hm, "/browse/webradios/*")) {
                //we serve the webradio directory without https to avoid ssl configuration for the mpd curl plugin
                static struct mg_http_serve_opts s_http_server_opts;
                s_http_server_opts.extra_headers = EXTRA_HEADERS_UNSAFE;
                s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
                s_http_server_opts.root_dir = mg_user_data->browse_directory;
                MYMPD_LOG_INFO("Serving uri \"%.*s\"", (int)hm->uri.len, hm->uri.ptr);
                mg_http_serve_dir(nc, hm, &s_http_server_opts);
                break;
            }
            //redirect to https
            struct mg_str *host_hdr = mg_http_get_header(hm, "Host");
            if (host_hdr == NULL) {
                MYMPD_LOG_ERROR("No hoster header found, closing connection");
                nc->is_closing = 1;
                break;
            }

            sds host_header = sdscatlen(sdsempty(), host_hdr->ptr, host_hdr->len);
            int count = 0;
            sds *tokens = sdssplitlen(host_header, (ssize_t)sdslen(host_header), ":", 1, &count);
            sds s_redirect = sdscatfmt(sdsempty(), "https://%S", tokens[0]);
            if (strcmp(config->ssl_port, "443") != 0) {
                s_redirect = sdscatfmt(s_redirect, ":%S", config->ssl_port);
            }
            MYMPD_LOG_INFO("Redirecting to %s", s_redirect);
            webserver_send_header_redirect(nc, s_redirect);
            nc->is_draining = 1;
            sdsfreesplitres(tokens, count);
            FREE_SDS(host_header);
            FREE_SDS(s_redirect);
            break;
        }
        case MG_EV_CLOSE:
            MYMPD_LOG_INFO("Connection %lu closed", nc->id);
            mg_user_data->connection_count--;
            break;
    }
}
#endif
