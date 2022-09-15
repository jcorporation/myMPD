/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/list.h"
#include "web_server.h"

#include "../lib/api.h"
#include "../lib/http_client.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/msg_queue.h"
#include "../lib/sds_extras.h"
#include "albumart.h"
#include "proxy.h"
#include "request_handler.h"
#include "tagart.h"

#include <sys/prctl.h>

/**
 * Private definitions
 */

static bool parse_internal_message(struct t_work_response *response, struct t_mg_user_data *mg_user_data);
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data);
#ifdef ENABLE_SSL
    static void ev_handler_redirect(struct mg_connection *nc_http, int ev, void *ev_data, void *fn_data);
#endif
static void send_ws_notify(struct mg_mgr *mgr, struct t_work_response *response);
static void send_api_response(struct mg_mgr *mgr, struct t_work_response *response);
static bool check_acl(struct mg_connection *nc, sds acl);
static void mongoose_log(char ch, void *param);

/**
 * Public functions
 */

/**
 * Initializes the webserver
 * @param mgr mongoose mgr
 * @param config pointer to myMPD config
 * @param mg_user_data already allocated t_mg_user_data to populate
 * @return true on success, else false
 */
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
    mg_user_data->feat_albumart = false;
    mg_user_data->connection_count = 0;
    list_init(&mg_user_data->stream_uris);
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

/**
 * Frees the mongoose mgr
 * @param mgr mongoose mgr to free
 * @return NULL
 */
void *web_server_free(struct mg_mgr *mgr) {
    sds dns4_url = (sds)mgr->dns4.url;
    FREE_SDS(dns4_url);
    mg_mgr_free(mgr);
    FREE_PTR(mgr);
    return NULL;
}

/**
 * Main function for the webserver thread
 * @param arg_mgr void pointer to mongoose mgr
 * @return NULL
 */
void *web_server_loop(void *arg_mgr) {
    thread_logname = sds_replace(thread_logname, "webserver");
    prctl(PR_SET_NAME, thread_logname, 0, 0, 0);
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) mgr->userdata;

    //set mongoose loglevel
    mg_log_set(1);
    mg_log_set_fn(mongoose_log, NULL);

    #ifdef ENABLE_SSL
    MYMPD_LOG_DEBUG("Using certificate: %s", mg_user_data->config->ssl_cert);
    MYMPD_LOG_DEBUG("Using private key: %s", mg_user_data->config->ssl_key);
    #endif

    sds last_notify = sdsempty();
    time_t last_time = 0;
    while (s_signal_received == 0) {
        struct t_work_response *response = mympd_queue_shift(web_server_queue, 50, 0);
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
                    MYMPD_LOG_DEBUG("Discarding repeated notify");
                    free_response(response);
                }
            }
            else {
                MYMPD_LOG_DEBUG("\"%s\": Got API response for id \"%lld\"", response->partition, response->conn_id);
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

/**
 * Private functions
 */

/**
 * Sets the mg_user_data values from set_mg_user_data_request.
 * Message is sent by the feature detection function in the mympd_api thread.
 * @param response 
 * @param mg_user_data t_mg_user_data to configure
 * @return true on success, else false
 */
static bool parse_internal_message(struct t_work_response *response, struct t_mg_user_data *mg_user_data) {
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

        mg_user_data->feat_albumart = new_mg_user_data->feat_albumart;

        list_clear(&mg_user_data->stream_uris);
        struct t_list_node *current = new_mg_user_data->partitions.head;
        sds uri = sdsempty();
        while (current != NULL) {
            if (current->value_i > 0) {
                uri = sdscatfmt(uri, "http://%s:%I",
                    (new_mg_user_data->mpd_host[0] == '/' ? "127.0.0.1" : new_mg_user_data->mpd_host),
                    current->value_i);
                MYMPD_LOG_DEBUG("\"%s\": Setting stream uri to \"%s\"", current->key, uri);
                list_push(&mg_user_data->stream_uris, current->key, 0, uri, NULL);
            }
            sdsclear(uri);
            current = current->next;
        }
        FREE_SDS(uri);
        FREE_SDS(new_mg_user_data->mpd_host);
        list_clear(&new_mg_user_data->partitions);
	    FREE_PTR(response->extra);
        rc = true;
    }
    else {
        MYMPD_LOG_WARN("Invalid internal message: %s", response->data);
    }
    free_response(response);
    return rc;
}

/**
 * Broadcasts a websocket connections to all clients
 * @param mgr mongoose mgr
 * @param response jsonrpc notification
 */
static void send_ws_notify(struct mg_mgr *mgr, struct t_work_response *response) {
    struct mg_connection *nc = mgr->conns;
    int send_count = 0;
    int conn_count = 0;
    while (nc != NULL) {
        if ((int)nc->is_websocket == 1) {
            struct t_frontend_nc_data *frontend_nc_data = (struct t_frontend_nc_data *)nc->fn_data;
            if (strcmp(response->partition, frontend_nc_data->partition) == 0 ||
                strcmp(response->partition, MPD_PARTITION_ALL) == 0)
            {
                MYMPD_LOG_DEBUG("\"%s\": Sending notify to conn_id %lu: %s", response->partition, nc->id, response->data);
                mg_ws_send(nc, response->data, sdslen(response->data), WEBSOCKET_OP_TEXT);
                send_count++;
            }
        }
        nc = nc->next;
        conn_count++;
    }
    if (send_count == 0) {
        MYMPD_LOG_DEBUG("No websocket client connected, discarding message: %s", response->data);
    }
    free_response(response);
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) mgr->userdata;
    if (conn_count != mg_user_data->connection_count) {
        MYMPD_LOG_DEBUG("Correcting connection count from %d to %d", mg_user_data->connection_count, conn_count);
        mg_user_data->connection_count = conn_count;
    }
}

/**
 * Sends a api response
 * @param mgr mongoose mgr
 * @param response jsonrpc response
 */
static void send_api_response(struct mg_mgr *mgr, struct t_work_response *response) {
    struct mg_connection *nc = mgr->conns;
    while (nc != NULL) {
        if ((int)nc->is_websocket == 0 && nc->id == (long unsigned)response->conn_id) {
            if (response->cmd_id == INTERNAL_API_ALBUMART) {
                webserver_send_albumart(nc, response->data, response->binary);
            }
            else {
                MYMPD_LOG_DEBUG("\"%s\": Sending response to conn_id %lu (length: %lu): %s", response->partition, nc->id, (unsigned long)sdslen(response->data), response->data);
                webserver_send_data(nc, response->data, sdslen(response->data), "Content-Type: application/json\r\n");
            }
            break;
        }
        nc = nc->next;
    }
    free_response(response);
}

/**
 * Matches the acl against the client ip and
 * sends an error repsonse / drains the connection if acl is not matched
 * @param nc mongoose connection
 * @param acl acl string to check
 * @return true if acl matches, else false
 */
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

/**
 * Central webserver event handler
 * nc->label usage
 * 0 - connection type: F = frontend connection, B = backend connection
 * 1 - http method: G = GET, H = HEAD, P = POST
 * 2 - connection header: C = close, K = keepalive
 *
 * @param nc mongoose connection
 * @param ev connection event
 * @param ev_data event data (http / websocket message)
 * @param fn_data backend connection for proxy connections
 */
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    //connection specific data structure
    struct t_frontend_nc_data *frontend_nc_data = (struct t_frontend_nc_data *)fn_data;
    //mongoose user data
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_config *config = mg_user_data->config;
    switch(ev) {
        case MG_EV_OPEN: {
            mg_user_data->connection_count++;
            //initialize fn_data
            frontend_nc_data = malloc_assert(sizeof(struct t_frontend_nc_data));
            frontend_nc_data->partition = NULL;
            frontend_nc_data->backend_nc = NULL;
            nc->fn_data = frontend_nc_data;
            //set labels
            nc->label[0] = 'F';
            nc->label[1] = '-';
            nc->label[2] = 'C';
            break;
        }
        case MG_EV_ACCEPT:
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
            break;
        case MG_EV_WS_MSG: {
            struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
            MYMPD_LOG_DEBUG("\"%s\": Websocket message (%lu): %.*s", frontend_nc_data->partition, nc->id, (int)wm->data.len, wm->data.ptr);
            if (mg_vcmp(&wm->data, "ping") == 0) {
                size_t sent = mg_ws_send(nc, "pong", 4, WEBSOCKET_OP_TEXT);
                if (sent != 6) {
                    MYMPD_LOG_WARN("Websocket: Could not reply with pong, closing connection");
                    nc->is_closing = 1;
                }
            }
            break;
        }
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;
            if (hm->query.len > 0) {
                MYMPD_LOG_INFO("HTTP request (%lu): %.*s %.*s?%.*s", nc->id, (int)hm->method.len, hm->method.ptr,
                    (int)hm->uri.len, hm->uri.ptr, (int)hm->query.len, hm->query.ptr);
            }
            else {
                MYMPD_LOG_INFO("HTTP request (%lu): %.*s %.*s", nc->id, (int)hm->method.len, hm->method.ptr,
                    (int)hm->uri.len, hm->uri.ptr);
            }
            //limit allowed http methods
            if (mg_vcmp(&hm->method, "GET") == 0) {
                nc->label[1] = 'G';
            }
            else if (mg_vcmp(&hm->method, "HEAD") == 0) {
                nc->label[1] = 'H';
            }
            else if (mg_vcmp(&hm->method, "POST") == 0) {
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
                if (mg_vcasecmp(connection_hdr, "close") == 0) {
                    nc->label[2] = 'C';
                }
                else {
                    nc->label[2] = 'K';
                }
            }
            //handle uris
            if (mg_http_match_uri(hm, "/api/*")) {
                //api request
                //check partition
                if (get_partition_from_uri(nc, hm, frontend_nc_data) == false) {
                    break;
                }
                //body
                sds body = sdsnewlen(hm->body.ptr, hm->body.len);
                /*
                 * We use the custom header X-myMPD-Session for authorization
                 * to allow other authorization methods in reverse proxy setups
                 */
                struct mg_str *auth_header = mg_http_get_header(hm, "X-myMPD-Session");
                bool rc = request_handler_api(nc, body, auth_header, mg_user_data, frontend_nc_data->backend_nc);
                FREE_SDS(body);
                if (rc == false) {
                    MYMPD_LOG_ERROR("\"%s\": Invalid API request", frontend_nc_data->partition);
                    sds response = jsonrpc_respond_message(sdsempty(), GENERAL_API_UNKNOWN, 0,
                        JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Invalid API request");
                    webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
                    FREE_SDS(response);
                }
            }
            else if (mg_http_match_uri(hm, "/albumart-thumb")) {
                request_handler_albumart(nc, hm, mg_user_data, (long long)nc->id, ALBUMART_THUMBNAIL);
            }
            else if (mg_http_match_uri(hm, "/albumart")) {
                request_handler_albumart(nc, hm, mg_user_data, (long long)nc->id, ALBUMART_FULL);
            }
            else if (mg_http_match_uri(hm, "/tagart")) {
                request_handler_tagart(nc, hm, mg_user_data);
            }
            else if (mg_http_match_uri(hm, "/browse/#")) {
                request_handler_browse(nc, hm, mg_user_data);
            }
            else if (mg_http_match_uri(hm, "/ws/*")) {
                //check partition
                if (get_partition_from_uri(nc, hm, frontend_nc_data) == false) {
                    break;
                }
                mg_ws_upgrade(nc, hm, NULL);
                MYMPD_LOG_INFO("\"%s\": New Websocket connection established (%lu)", frontend_nc_data->partition, nc->id);
                sds response = jsonrpc_event(sdsempty(), JSONRPC_EVENT_WELCOME);
                mg_ws_send(nc, response, sdslen(response), WEBSOCKET_OP_TEXT);
                FREE_SDS(response);
            }
            else if (mg_http_match_uri(hm, "/stream/*")) {
                //check partition
                if (get_partition_from_uri(nc, hm, frontend_nc_data) == false) {
                    break;
                }
                struct t_list_node *node = list_get_node(&mg_user_data->stream_uris, frontend_nc_data->partition);
                if (node == NULL) {
                    webserver_send_error(nc, 404, "Stream uri not configured");
                    nc->is_draining = 1;
                    break;
                }
                create_backend_connection(nc, frontend_nc_data->backend_nc, node->value_p, forward_backend_to_frontend);
            }
            else if (mg_http_match_uri(hm, "/proxy")) {
                //Makes a get request to the defined uri and returns the response
                request_handler_proxy(nc, hm, frontend_nc_data->backend_nc);
            }
            else if (mg_http_match_uri(hm, "/serverinfo")) {
                request_handler_serverinfo(nc);
            }
            else if (mg_http_match_uri(hm, "/script-api/*")) {
                //check acl
                if (check_acl(nc, config->scriptacl) == false) {
                    break;
                }
                //check partition
                if (get_partition_from_uri(nc, hm, frontend_nc_data) == false) {
                    break;
                }
                sds body = sdsnewlen(hm->body.ptr, hm->body.len);
                bool rc = request_handler_script_api(nc, body);
                FREE_SDS(body);
                if (rc == false) {
                    MYMPD_LOG_ERROR("\"%s\": Invalid script API request", frontend_nc_data->partition);
                    sds response = jsonrpc_respond_message(sdsempty(), GENERAL_API_UNKNOWN, 0,
                        JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Invalid script API request");
                    webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
                    FREE_SDS(response);
                }
            }
            else if (mg_vcmp(&hm->uri, "/index.html") == 0) {
                webserver_send_header_redirect(nc, "/");
            }
            else if (mg_vcmp(&hm->uri, "/favicon.ico") == 0) {
                webserver_send_header_redirect(nc, "/assets/appicon-192.png");
            }
            #ifdef ENABLE_SSL
            else if (mg_http_match_uri(hm, "/ca.crt")) {
                request_handler_ca(nc, hm, mg_user_data);
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
            if (frontend_nc_data == NULL) {
                MYMPD_LOG_WARN("frontend_nc_data not allocated");
                break;
            }
            if (frontend_nc_data->backend_nc != NULL) {
                MYMPD_LOG_INFO("Closing backend connection \"%lu\"", frontend_nc_data->backend_nc->id);
                //remove pointer to frontend connection
                struct t_backend_nc_data *backend_nc_data = (struct t_backend_nc_data *)frontend_nc_data->backend_nc->fn_data;
                backend_nc_data->frontend_nc = NULL;
                //close backend connection
                frontend_nc_data->backend_nc->is_closing = 1;
            }
            FREE_SDS(frontend_nc_data->partition);
            FREE_PTR(frontend_nc_data);
            nc->fn_data = NULL;
            break;
        }
    }
}

#ifdef ENABLE_SSL
/**
 * Redirects the client to https if ssl is enabled.
 * Only requests to /browse/webradios are not redirected.
 * @param nc mongoose connection
 * @param ev connection event
 * @param ev_data event data (http / websocket message)
 * @param fn_data not used
 */
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

/**
 * Mongoose logging function
 * @param ch character to log
 * @param param 
 */
static void mongoose_log(char ch, void *param) {
    static char buf[256];
    static size_t len;
    buf[len++] = ch;
    if (ch == '\n' ||
        len >= sizeof(buf))
    {
        MYMPD_LOG_DEBUG("%.*s", (int) len, buf); //Send logs
        len = 0;
    }
    (void)param;
}
