/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/web_server.h"

#include "src/lib/api.h"
#include "src/lib/cert.h"
#include "src/lib/filehandler.h"
#include "src/lib/http_client.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mg_str_utils.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/thread.h"
#include "src/web_server/albumart.h"
#include "src/web_server/folderart.h"
#include "src/web_server/playlistart.h"
#include "src/web_server/proxy.h"
#include "src/web_server/request_handler.h"
#include "src/web_server/tagart.h"

#ifdef MYMPD_ENABLE_LUA
    #include "src/web_server/scripts.h"
#endif

#include <inttypes.h>
#include <libgen.h>

/**
 * Private definitions
 */

static void get_placeholder_image(sds workdir, const char *name, sds *result);
static void read_queue(struct mg_mgr *mgr);
static bool parse_internal_message(struct t_work_response *response, struct t_mg_user_data *mg_user_data);
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data);
static void ev_handler_redirect(struct mg_connection *nc_http, int ev, void *ev_data);
static void send_ws_notify(struct mg_mgr *mgr, struct t_work_response *response);
static void send_ws_notify_client(struct mg_mgr *mgr, struct t_work_response *response);
static void send_raw_response(struct mg_mgr *mgr, struct t_work_response *response);
static void send_api_response(struct mg_mgr *mgr, struct t_work_response *response);
static bool enforce_acl(struct mg_connection *nc, sds acl);
static bool enforce_conn_limit(struct mg_connection *nc, int connection_count);
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
    mg_user_data->browse_directory = sdscatfmt(sdsempty(), "%S/%s", config->workdir, DIR_WORK_EMPTY);
    mg_user_data->music_directory = sdsempty();
    mg_user_data->placeholder_booklet = sdsempty();
    mg_user_data->placeholder_mympd = sdsempty();
    mg_user_data->placeholder_na = sdsempty();
    mg_user_data->placeholder_stream = sdsempty();
    mg_user_data->placeholder_playlist = sdsempty();
    mg_user_data->placeholder_smartpls = sdsempty();
    mg_user_data->placeholder_folder = sdsempty();
    sds default_coverimage_names = sdsnew(MYMPD_COVERIMAGE_NAMES);
    mg_user_data->coverimage_names= sds_split_comma_trim(default_coverimage_names, &mg_user_data->coverimage_names_len);
    FREE_SDS(default_coverimage_names);
    sds default_thumbnail_names = sdsnew(MYMPD_THUMBNAIL_NAMES);
    mg_user_data->thumbnail_names= sds_split_comma_trim(default_thumbnail_names, &mg_user_data->thumbnail_names_len);
    FREE_SDS(default_thumbnail_names);
    mg_user_data->publish_music = false;
    mg_user_data->publish_playlists = false;
    mg_user_data->feat_albumart = false;
    mg_user_data->connection_count = 2; // listening + wakup
    list_init(&mg_user_data->stream_uris);
    list_init(&mg_user_data->session_list);
    mg_user_data->mympd_api_started = false;
    mg_user_data->cert_content = sdsempty();
    mg_user_data->cert = mg_str("");
    mg_user_data->key_content = sdsempty();
    mg_user_data->key = mg_str("");

    //init monogoose mgr
    mg_mgr_init(mgr);
    mgr->userdata = mg_user_data;
    mgr->product_name = "myMPD "MYMPD_VERSION;
    mgr->directory_listing_css = DIRECTORY_LISTING_CSS;
    //set dns server
    mgr->dns4.url = get_dnsserver();
    MYMPD_LOG_DEBUG(NULL, "Setting dns server to %s", mgr->dns4.url);

    //bind to http_port
    if (config->http == true) {
        struct mg_connection *nc_http = NULL;
        sds http_url = sdscatfmt(sdsempty(), "http://%S:%i", config->http_host, config->http_port);
        if (config->ssl == true) {
            nc_http = mg_http_listen(mgr, http_url, ev_handler_redirect, NULL);
        }
        else {
            nc_http = mg_http_listen(mgr, http_url, ev_handler, NULL);
        }
        FREE_SDS(http_url);
        if (nc_http == NULL) {
            MYMPD_LOG_EMERG(NULL, "Can't bind to http://%s:%d", config->http_host, config->http_port);
            return false;
        }
        MYMPD_LOG_NOTICE(NULL, "Listening on http://%s:%d", config->http_host, config->http_port);
    }
    //bind to ssl_port
    if (config->ssl == true) {
        sds https_url = sdscatfmt(sdsempty(), "https://%S:%i", config->http_host, config->ssl_port);
        struct mg_connection *nc_https = mg_http_listen(mgr, https_url, ev_handler, NULL);
        FREE_SDS(https_url);
        if (nc_https == NULL) {
            MYMPD_LOG_ERROR(NULL, "Can't bind to https://%s:%d", config->http_host, config->ssl_port);
            return false;
        }
        MYMPD_LOG_NOTICE(NULL, "Listening on https://%s:%d", config->http_host, config->ssl_port);
    }
    else if (config->http == false) {
        MYMPD_LOG_ERROR(NULL, "Not listening on any port.");
        return false;
    }
    MYMPD_LOG_NOTICE(NULL, "Serving files from \"%s\"", MYMPD_DOC_ROOT);
    return true;
}

/**
 * Reads the ssl key and certificate from disc
 * @param mg_user_data pointer to mongoose user data
 * @param config pointer to myMPD config
 * @return true on success, else false
 */
bool webserver_read_certs(struct t_mg_user_data *mg_user_data, struct t_config *config) {
    if (config->ssl == false) {
        return true;
    }
    int nread = 0;
    mg_user_data->cert_content = sds_getfile(mg_user_data->cert_content, config->ssl_cert, SSL_FILE_MAX, false, true, &nread);
    if (nread <= 0) {
        MYMPD_LOG_ERROR(NULL, "Failure reading ssl certificate from disc");
        return false;
    }
    nread = 0;
    mg_user_data->key_content = sds_getfile(mg_user_data->key_content, config->ssl_key, SSL_FILE_MAX, false, true, &nread);
    if (nread <= 0) {
        MYMPD_LOG_ERROR(NULL, "Failure reading ssl key from disc");
        return false;
    }
    sds cert_details = certificate_get_detail(mg_user_data->cert_content);
    if (sdslen(cert_details) > 0) {
        MYMPD_LOG_INFO(NULL, "Certificate: %s", cert_details);
    }
    FREE_SDS(cert_details);
    mg_user_data->cert = mg_str(mg_user_data->cert_content);
    mg_user_data->key = mg_str(mg_user_data->key_content);
    return true;
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
    set_threadname(thread_logname);
    struct mg_mgr *mgr = (struct mg_mgr *) arg_mgr;
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) mgr->userdata;

    //set mongoose loglevel to error
    mg_log_set(1);
    //debug logging
    //mg_log_set(4);
    mg_log_set_fn(mongoose_log, NULL);
    // Initialise wakeup socket pair
    mg_wakeup_init(mgr);
    if (mg_user_data->config->ssl == true) {
        MYMPD_LOG_DEBUG(NULL, "Using certificate: %s", mg_user_data->config->ssl_cert);
        MYMPD_LOG_DEBUG(NULL, "Using private key: %s", mg_user_data->config->ssl_key);
    }
    while (s_signal_received == 0) {
        //webserver polling
        mg_mgr_poll(mgr, -1);
    }
    MYMPD_LOG_DEBUG(NULL, "Stopping web_server thread");
    FREE_SDS(thread_logname);
    return NULL;
}

/**
 * Private functions
 */

/**
 * Reads and processes all messages from the queue
 * @param mgr pointer to mongoose mgr
 */
static void read_queue(struct mg_mgr *mgr) {
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) mgr->userdata;
    struct t_work_response *response;
    while ((response = mympd_queue_shift(web_server_queue, -1, 0)) != NULL) {
        switch(response->type) {
            case RESPONSE_TYPE_NOTIFY_CLIENT:
                //websocket notify for specific clients
                send_ws_notify_client(mgr, response);
                break;
            case RESPONSE_TYPE_NOTIFY_PARTITION:
                //websocket notify for all clients
                send_ws_notify(mgr, response);
                break;
            case RESPONSE_TYPE_PUSH_CONFIG:
                //internal message
                if (response->cmd_id == INTERNAL_API_WEBSERVER_READY) {
                    mg_user_data->mympd_api_started = true;
                    free_response(response);
                }
                else if (response->cmd_id == INTERNAL_API_WEBSERVER_SETTINGS){
                    parse_internal_message(response, mg_user_data);
                }
                else {
                    MYMPD_LOG_ERROR(response->partition, "Invalid API method: %s", get_cmd_id_method_name(response->cmd_id));
                }
                break;
            case RESPONSE_TYPE_DEFAULT:
                //api response
                MYMPD_LOG_DEBUG(response->partition, "Got API response for id \"%lu\"", response->conn_id);
                send_api_response(mgr, response);
                break;
            case RESPONSE_TYPE_RAW:
                MYMPD_LOG_DEBUG(response->partition, "Got raw response for id \"%lu\" with %lu bytes", response->conn_id, (unsigned long)sdslen(response->data));
                send_raw_response(mgr, response);
                break;
            case RESPONSE_TYPE_SCRIPT:
            case RESPONSE_TYPE_DISCARD:
                //ignore
                break;
        }
    }
}

/**
 * Sets the mg_user_data values from set_mg_user_data_request.
 * Message is sent from the mympd_api thread.
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
        mg_user_data->browse_directory = sdscatfmt(mg_user_data->browse_directory, "%S/%s", config->workdir, DIR_WORK_EMPTY);
        mg_user_data->browse_directory = sdscatfmt(mg_user_data->browse_directory, ",/browse/pics=%S/%s", config->workdir, DIR_WORK_PICS);
        mg_user_data->browse_directory = sdscatfmt(mg_user_data->browse_directory, ",/browse/smartplaylists=%S/%s", config->workdir, DIR_WORK_SMARTPLS);
        mg_user_data->browse_directory = sdscatfmt(mg_user_data->browse_directory, ",/browse/webradios=%S/%s", config->workdir, DIR_WORK_WEBRADIOS);
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
        MYMPD_LOG_DEBUG(NULL, "Document root: \"%s\"", mg_user_data->browse_directory);

        //coverimage names
        sdsfreesplitres(mg_user_data->coverimage_names, mg_user_data->coverimage_names_len);
        mg_user_data->coverimage_names = sds_split_comma_trim(new_mg_user_data->coverimage_names, &mg_user_data->coverimage_names_len);
        FREE_SDS(new_mg_user_data->coverimage_names);

        sdsfreesplitres(mg_user_data->thumbnail_names, mg_user_data->thumbnail_names_len);
        mg_user_data->thumbnail_names = sds_split_comma_trim(new_mg_user_data->thumbnail_names, &mg_user_data->thumbnail_names_len);
        FREE_SDS(new_mg_user_data->thumbnail_names);

        mg_user_data->feat_albumart = new_mg_user_data->feat_albumart;

        //set per partition stream uris
        list_clear(&mg_user_data->stream_uris);
        struct t_list_node *current = new_mg_user_data->partitions.head;
        sds uri = sdsempty();
        while (current != NULL) {
            if (current->value_i > 0) {
                uri = sdscatfmt(uri, "http://%s:%I",
                    (new_mg_user_data->mpd_host[0] == '/' ? "127.0.0.1" : new_mg_user_data->mpd_host),
                    current->value_i);
                MYMPD_LOG_DEBUG(current->key, "Setting stream uri to \"%s\"", uri);
                list_push(&mg_user_data->stream_uris, current->key, 0, uri, NULL);
            }
            sdsclear(uri);
            current = current->next;
        }
        FREE_SDS(uri);

        //custom placeholder images
        get_placeholder_image(config->workdir, "coverimage-booklet", &mg_user_data->placeholder_booklet);
        get_placeholder_image(config->workdir, "coverimage-folder", &mg_user_data->placeholder_folder);
        get_placeholder_image(config->workdir, "coverimage-mympd", &mg_user_data->placeholder_mympd);
        get_placeholder_image(config->workdir, "coverimage-notavailable", &mg_user_data->placeholder_na);
        get_placeholder_image(config->workdir, "coverimage-stream", &mg_user_data->placeholder_stream);
        get_placeholder_image(config->workdir, "coverimage-playlist", &mg_user_data->placeholder_playlist);
        get_placeholder_image(config->workdir, "coverimage-smartpls", &mg_user_data->placeholder_smartpls);

        //cleanup
        FREE_SDS(new_mg_user_data->mpd_host);
        list_clear(&new_mg_user_data->partitions);
        FREE_PTR(response->extra);
        rc = true;
    }
    else {
        MYMPD_LOG_WARN(NULL, "Invalid internal message: %s", response->data);
    }
    free_response(response);
    return rc;
}

/**
 * Finds and sets the placeholder images
 * @param workdir myMPD working directory
 * @param name basename to search for
 * @param result pointer to sds result
 */
static void get_placeholder_image(sds workdir, const char *name, sds *result) {
    sds file = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_PICS_THUMBS, name);
    MYMPD_LOG_DEBUG(NULL, "Check for custom placeholder image \"%s\"", file);
    file = webserver_find_image_file(file);
    sdsclear(*result);
    if (sdslen(file) > 0) {
        file = sds_basename(file);
        MYMPD_LOG_INFO(NULL, "Setting custom placeholder image for %s to \"%s\"", name, file);
        *result = sdscatfmt(*result, "/browse/%s/%S", DIR_WORK_PICS_THUMBS, file);
    }
    else {
        *result = sdscatfmt(*result, "/assets/%s.svg", name);
    }
    FREE_SDS(file);
}

/**
 * Broadcasts a message through all websocket connections for a specific or all partitions
 * @param mgr mongoose mgr
 * @param response jsonrpc notification
 */
static void send_ws_notify(struct mg_mgr *mgr, struct t_work_response *response) {
    struct mg_connection *nc = mgr->conns;
    int send_count = 0;
    int conn_count = 0;
    time_t last_ping = time(NULL) - WS_PING_TIMEOUT;
    while (nc != NULL) {
        if (nc->is_websocket == 1U) {
            struct t_frontend_nc_data *frontend_nc_data = (struct t_frontend_nc_data *)nc->fn_data;
            if (frontend_nc_data->last_ws_ping < last_ping) {
                MYMPD_LOG_INFO(NULL, "Closing stale websocket connection \"%lu\"", nc->id);
                nc->is_closing = 1;
            }
            else if (strcmp(response->partition, frontend_nc_data->partition) == 0 ||
                strcmp(response->partition, MPD_PARTITION_ALL) == 0)
            {
                MYMPD_LOG_DEBUG(response->partition, "Sending notify to conn_id \"%lu\": %s", nc->id, response->data);
                mg_ws_send(nc, response->data, sdslen(response->data), WEBSOCKET_OP_TEXT);
                send_count++;
            }
        }
        nc = nc->next;
        conn_count++;
    }
    if (send_count == 0) {
        MYMPD_LOG_DEBUG(NULL, "No websocket client connected, discarding message: %s", response->data);
    }
    free_response(response);
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) mgr->userdata;
    if (conn_count != mg_user_data->connection_count) {
        MYMPD_LOG_DEBUG(NULL, "Correcting connection count from %d to %d", mg_user_data->connection_count, conn_count);
        mg_user_data->connection_count = conn_count;
    }
}

/**
 * Sends a message through the websocket to a specific client
 * We use the jsonprc id to identify the websocket connection
 * @param mgr mongoose mgr
 * @param response jsonrpc notification
 */
static void send_ws_notify_client(struct mg_mgr *mgr, struct t_work_response *response) {
    struct mg_connection *nc = mgr->conns;
    int send_count = 0;
    const unsigned client_id = response->id / 1000;
    //const unsigned request_id = response->id % 1000;
    while (nc != NULL) {
        if (nc->is_websocket == 1U) {
            struct t_frontend_nc_data *frontend_nc_data = (struct t_frontend_nc_data *)nc->fn_data;
            if (client_id == frontend_nc_data->id) {
                MYMPD_LOG_DEBUG(response->partition, "Sending notify to conn_id \"%lu\", jsonrpc client id %u: %s", nc->id, client_id, response->data);
                mg_ws_send(nc, response->data, sdslen(response->data), WEBSOCKET_OP_TEXT);
                send_count++;
                break;
            }
        }
        nc = nc->next;
    }
    if (send_count == 0) {
        MYMPD_LOG_DEBUG(NULL, "No websocket client connected, discarding message: %s", response->data);
    }
    free_response(response);
}

/**
 * Sends a raw http response message
 * @param mgr mongoose mgr
 * @param response jsonrpc response
 */
static void send_raw_response(struct mg_mgr *mgr, struct t_work_response *response) {
    struct mg_connection *nc = mgr->conns;
    while (nc != NULL) {
        if (nc->is_websocket == 0U &&
            nc->id == response->conn_id)
        {
            webserver_send_raw(nc, response->data, sdslen(response->data));
            break;
        }
        nc = nc->next;
    }
    free_response(response);
}

/**
 * Sends an api response
 * @param mgr mongoose mgr
 * @param response jsonrpc response
 */
static void send_api_response(struct mg_mgr *mgr, struct t_work_response *response) {
    struct mg_connection *nc = mgr->conns;
    while (nc != NULL) {
        if (nc->is_websocket == 0U &&
            nc->id == response->conn_id)
        {
            if (response->cmd_id == INTERNAL_API_ALBUMART_BY_URI) {
                webserver_send_albumart(nc, response->data, response->binary);
            }
            else if (response->cmd_id == INTERNAL_API_ALBUMART_BY_ALBUMID) {
                webserver_send_albumart_redirect(nc, response->data);
            }
            else {
                MYMPD_LOG_DEBUG(response->partition, "Sending response to conn_id \"%lu\" (length: %lu): %s", nc->id, (unsigned long)sdslen(response->data), response->data);
                webserver_send_data(nc, response->data, sdslen(response->data), EXTRA_HEADERS_JSON_CONTENT);
            }
            break;
        }
        nc = nc->next;
    }
    free_response(response);
}

/**
 * Matches the acl against the client ip and
 * sends an error response / drains the connection if acl is not matched
 * @param nc mongoose connection
 * @param acl acl string to check
 * @return true if acl matches, else false
 */
static bool enforce_acl(struct mg_connection *nc, sds acl) {
    if (sdslen(acl) == 0) {
        return true;
    }
    if (nc->rem.is_ip6 == true) {
        //acls for ipv6 is not implemented in mongoose
        return true;
    }
    int acl_result = mg_check_ip_acl(mg_str(acl), &nc->rem);
    MYMPD_LOG_DEBUG(NULL, "Check against acl \"%s\": %d", acl, acl_result);
    if (acl_result == 1) {
        return true;
    }
    if (acl_result < 0) {
        MYMPD_LOG_ERROR(NULL, "Malformed acl \"%s\"", acl);
        return false;
    }

    sds ip = print_ip(sdsempty(), &nc->rem);
    MYMPD_LOG_ERROR(NULL, "Connection from \"%s\" blocked by ACL", ip);
    webserver_send_error(nc, 403, "Request blocked by ACL");
    nc->is_draining = 1;
    FREE_SDS(ip);

    return false;
}

/**
 * Enforces the connection limit
 * @param nc mongoose connection
 * @param connection_count connection count
 * @return true if connection count is not exceeded, else false
 */
static bool enforce_conn_limit(struct mg_connection *nc, int connection_count) {
    if (connection_count > HTTP_CONNECTIONS_MAX) {
        MYMPD_LOG_DEBUG(NULL, "Connections: %d", connection_count);
        MYMPD_LOG_ERROR(NULL, "Concurrent connections limit exceeded: %d", connection_count);
        webserver_send_error(nc, 429, "Concurrent connections limit exceeded");
        nc->is_draining = 1;
        return false;
    }
    return true;
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
 */
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    //connection specific data structure
    struct t_frontend_nc_data *frontend_nc_data = (struct t_frontend_nc_data *) nc->fn_data;
    //mongoose user data
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_config *config = mg_user_data->config;
    switch(ev) {
        case MG_EV_OPEN: {
            if (nc->is_listening) {
                nc->fn_data = NULL;
                web_server_queue->mg_conn_id = nc->id;
                web_server_queue->mg_mgr = nc->mgr;
            }
            else {
                mg_user_data->connection_count++;
                //initialize fn_data
                frontend_nc_data = malloc_assert(sizeof(struct t_frontend_nc_data));
                frontend_nc_data->partition = NULL;           // populated on websocket handshake
                frontend_nc_data->id = 0;                     // populated through websocket message
                frontend_nc_data->last_ws_ping = time(NULL);  // websocket ping timestamp
                frontend_nc_data->backend_nc = NULL;          // used for reverse proxy function
                nc->fn_data = frontend_nc_data;
                //set labels
                nc->data[0] = 'F'; // connection type
                nc->data[1] = '-'; // http method
                nc->data[2] = 'C'; // connection header
            }
            break;
        }
        case MG_EV_WAKEUP:
            read_queue(nc->mgr);
            break;
        case MG_EV_ACCEPT:
            if (loglevel == LOG_DEBUG) {
                sds ip = print_ip(sdsempty(), &nc->rem);
                MYMPD_LOG_DEBUG(NULL, "New connection id \"%lu\" from %s, connections: %d", nc->id, ip, mg_user_data->connection_count);
                FREE_SDS(ip);
            }
            //ssl support
            if (config->ssl == true) {
                MYMPD_LOG_DEBUG(NULL, "Init tls with cert \"%s\" and key \"%s\" for connection \"%lu\"", config->ssl_cert, config->ssl_key, nc->id);
                struct mg_tls_opts tls_opts = {
                    .cert = mg_user_data->cert,
                    .key = mg_user_data->key
                };
                mg_tls_init(nc, &tls_opts);
            }
            //enforce connection limit
            if (enforce_conn_limit(nc, mg_user_data->connection_count) == false) {
                break;
            }
            //enforce acl
            if (enforce_acl(nc, config->acl) == false) {
                break;
            }
            break;
        case MG_EV_WS_MSG: {
            struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
            struct mg_str matches[1];
            size_t sent = 0;
            if (wm->data.len > 9) {
                MYMPD_LOG_ERROR(frontend_nc_data->partition, "Websocket message too long: %lu", (unsigned long)wm->data.len);
                sent = mg_ws_send(nc, "too long", 8, WEBSOCKET_OP_TEXT);
            }
            else if (mg_strcmp(wm->data, mg_str("ping")) == 0) {
                sent = mg_ws_send(nc, "pong", 4, WEBSOCKET_OP_TEXT);
            }
            else if (mg_match(wm->data, mg_str("id:*"), matches)) {
                frontend_nc_data->id = mg_str_to_uint(&matches[0]);
                MYMPD_LOG_INFO(frontend_nc_data->partition, "Setting websocket id to \"%u\"", frontend_nc_data->id);
                sent = mg_ws_send(nc, "ok", 2, WEBSOCKET_OP_TEXT);
            }
            else {
                MYMPD_LOG_DEBUG(frontend_nc_data->partition, "Websocket message (%lu): %.*s", nc->id, (int)wm->data.len, wm->data.buf);
                MYMPD_LOG_ERROR(frontend_nc_data->partition, "Invalid Websocket message");
                sent = mg_ws_send(nc, "invalid", 7, WEBSOCKET_OP_TEXT);
            }
            if (sent == 0) {
                MYMPD_LOG_ERROR(frontend_nc_data->partition, "Websocket: Could not reply, closing connection");
                nc->is_closing = 1;
            }
            else {
                frontend_nc_data->last_ws_ping = time(NULL);
            }
            break;
        }
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;
            if (hm->query.len > 0) {
                MYMPD_LOG_INFO(NULL, "HTTP request (%lu): %.*s %.*s?%.*s", nc->id, (int)hm->method.len, hm->method.buf,
                    (int)hm->uri.len, hm->uri.buf, (int)hm->query.len, hm->query.buf);
            }
            else {
                MYMPD_LOG_INFO(NULL, "HTTP request (%lu): %.*s %.*s", nc->id, (int)hm->method.len, hm->method.buf,
                    (int)hm->uri.len, hm->uri.buf);
            }
            //limit allowed http methods
            if (mg_strcmp(hm->method, mg_str("GET")) == 0) {
                nc->data[1] = 'G';
            }
            else if (mg_strcmp(hm->method, mg_str("HEAD")) == 0) {
                nc->data[1] = 'H';
            }
            else if (mg_strcmp(hm->method, mg_str("POST")) == 0) {
                nc->data[1] = 'P';
            }
            else if (mg_strcmp(hm->method, mg_str("OPTIONS")) == 0) {
                nc->data[1] = 'O';
                webserver_send_cors_reply(nc);
                return;
            }
            else {
                MYMPD_LOG_ERROR(NULL, "Invalid http method \"%.*s\" (%lu)", (int)hm->method.len, hm->method.buf, nc->id);
                webserver_send_error(nc, 405, "Invalid http method");
                nc->is_draining = 1;
                return;
            }
            //check uri length
            if (hm->uri.len > URI_LENGTH_MAX) {
                MYMPD_LOG_ERROR(NULL, "Uri is too long, length is %lu, maximum length is %d (%lu)", (unsigned long)hm->uri.len, URI_LENGTH_MAX, nc->id);
                webserver_send_error(nc, 414, "Uri is too long");
                nc->is_draining = 1;
                return;
            }
            //check post requests length
            if (nc->data[1] == 'P' && (hm->body.len == 0 || hm->body.len > BODY_SIZE_MAX)) {
                MYMPD_LOG_ERROR(NULL, "POST request with body of size %lu is out of bounds (%lu)", (unsigned long)hm->body.len, nc->id);
                webserver_send_error(nc, 413, "Post request is too large");
                nc->is_draining = 1;
                return;
            }
            //respect connection close header
            struct mg_str *connection_hdr = mg_http_get_header(hm, "Connection");
            if (connection_hdr != NULL) {
                if (mg_strcasecmp(*connection_hdr, mg_str("close")) == 0) {
                    nc->data[2] = 'C';
                }
                else {
                    nc->data[2] = 'K';
                }
            }
            //handle uris
            if (mg_match(hm->uri, mg_str("/api/*"), NULL)) {
                //api request
                if (mg_user_data->mympd_api_started == false) {
                    //mympd_api thread not yet ready
                    MYMPD_LOG_WARN(frontend_nc_data->partition, "mympd_api thread not yet ready");
                    sds response = jsonrpc_respond_message(sdsempty(), GENERAL_API_NOT_READY, 0,
                        JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "myMPD not yet ready");
                    webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
                    FREE_SDS(response);
                }
                //check partition
                if (get_partition_from_uri(nc, hm, frontend_nc_data) == false) {
                    break;
                }
                //body
                sds body = sdsnewlen(hm->body.buf, hm->body.len);
                /*
                 * We use the custom header X-myMPD-Session for authorization
                 * to allow other authorization methods in reverse proxy setups
                 */
                struct mg_str *auth_header = mg_http_get_header(hm, "X-myMPD-Session");
                bool rc = request_handler_api(nc, body, auth_header, mg_user_data, frontend_nc_data->backend_nc);
                FREE_SDS(body);
                if (rc == false) {
                    MYMPD_LOG_ERROR(frontend_nc_data->partition, "Invalid API request");
                    sds response = jsonrpc_respond_message(sdsempty(), GENERAL_API_UNKNOWN, 0,
                        JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Invalid API request");
                    webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
                    FREE_SDS(response);
                }
            }
            else if (mg_match(hm->uri, mg_str("/albumart-thumb/*"), NULL)) {
                request_handler_albumart_by_album_id(hm, nc->id, ALBUMART_THUMBNAIL);
            }
            else if (mg_match(hm->uri, mg_str("/albumart/*"), NULL)) {
                request_handler_albumart_by_album_id(hm, nc->id, ALBUMART_FULL);
            }
            else if (mg_match(hm->uri, mg_str("/albumart-thumb"), NULL)) {
                request_handler_albumart_by_uri(nc, hm, mg_user_data, nc->id, ALBUMART_THUMBNAIL);
            }
            else if (mg_match(hm->uri, mg_str("/albumart"), NULL)) {
                request_handler_albumart_by_uri(nc, hm, mg_user_data, nc->id, ALBUMART_FULL);
            }
            else if (mg_match(hm->uri, mg_str("/folderart"), NULL)) {
                request_handler_folderart(nc, hm, mg_user_data);
            }
            else if (mg_match(hm->uri, mg_str("/tagart"), NULL)) {
                request_handler_tagart(nc, hm, mg_user_data, nc->id);
            }
            else if (mg_match(hm->uri, mg_str("/playlistart"), NULL)) {
                request_handler_playlistart(nc, hm, mg_user_data);
            }
            else if (mg_match(hm->uri, mg_str("/browse/#"), NULL)) {
                request_handler_browse(nc, hm, mg_user_data);
            }
            else if (mg_match(hm->uri, mg_str("/ws/*"), NULL)) {
                //check partition
                if (get_partition_from_uri(nc, hm, frontend_nc_data) == false) {
                    break;
                }
                mg_ws_upgrade(nc, hm, NULL);
                MYMPD_LOG_INFO(frontend_nc_data->partition, "New Websocket connection established (%lu)", nc->id);
                sds response = jsonrpc_event(sdsempty(), JSONRPC_EVENT_WELCOME);
                mg_ws_send(nc, response, sdslen(response), WEBSOCKET_OP_TEXT);
                FREE_SDS(response);
            }
            else if (mg_match(hm->uri, mg_str("/stream/*"), NULL)) {
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
                create_backend_connection(nc, frontend_nc_data->backend_nc, node->value_p, forward_backend_to_frontend_stream, true);
            }
            else if (mg_match(hm->uri, mg_str("/proxy"), NULL)) {
                //Makes a get request to the defined uri and returns the response
                request_handler_proxy(nc, hm, frontend_nc_data->backend_nc);
            }
            else if (mg_match(hm->uri, mg_str("/proxy-covercache"), NULL)) {
                //Makes a get request to the defined uri and caches and returns the response
                request_handler_proxy_covercache(nc, hm, frontend_nc_data->backend_nc);
            }
            else if (mg_match(hm->uri, mg_str("/serverinfo"), NULL)) {
                request_handler_serverinfo(nc);
            }
        #ifdef MYMPD_ENABLE_LUA
            else if (mg_match(hm->uri, mg_str("/script-api/*"), NULL)) {
                //enforce script acl
                if (enforce_acl(nc, config->scriptacl) == false) {
                    break;
                }
                //check partition
                if (get_partition_from_uri(nc, hm, frontend_nc_data) == false) {
                    break;
                }
                sds body = sdsnewlen(hm->body.buf, hm->body.len);
                bool rc = request_handler_script_api(nc, body);
                FREE_SDS(body);
                if (rc == false) {
                    MYMPD_LOG_ERROR(frontend_nc_data->partition, "Invalid script API request");
                    sds response = jsonrpc_respond_message(sdsempty(), GENERAL_API_UNKNOWN, 0,
                        JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Invalid script API request");
                    webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
                    FREE_SDS(response);
                }
            }
            else if (mg_match(hm->uri, mg_str("/script/*/*"), NULL)) {
                script_execute_http(nc, hm, config);
            }
        #endif
            else if (mg_match(hm->uri, mg_str("/assets/coverimage-booklet"), NULL)) {
                webserver_serve_placeholder_image(nc, PLACEHOLDER_BOOKLET);
            }
            else if (mg_match(hm->uri, mg_str("/assets/coverimage-mympd"), NULL)) {
                webserver_serve_placeholder_image(nc, PLACEHOLDER_MYMPD);
            }
            else if (mg_match(hm->uri, mg_str("/assets/coverimage-notavailable"), NULL)) {
                webserver_serve_placeholder_image(nc, PLACEHOLDER_NA);
            }
            else if (mg_match(hm->uri, mg_str("/assets/coverimage-stream"), NULL)) {
                webserver_serve_placeholder_image(nc, PLACEHOLDER_STREAM);
            }
            else if (mg_match(hm->uri, mg_str("/assets/coverimage-playlist"), NULL)) {
                webserver_serve_placeholder_image(nc, PLACEHOLDER_PLAYLIST);
            }
            else if (mg_match(hm->uri, mg_str("/assets/coverimage-smartpls"), NULL)) {
                webserver_serve_placeholder_image(nc, PLACEHOLDER_SMARTPLS);
            }
            else if (mg_match(hm->uri, mg_str("/index.html"), NULL)) {
                webserver_send_header_redirect(nc, "/");
            }
            else if (mg_match(hm->uri, mg_str("/favicon.ico"), NULL)) {
                webserver_send_header_redirect(nc, "/assets/appicon-192.png");
            }
            else if (mg_match(hm->uri, mg_str("/ca.crt"), NULL)) {
                request_handler_ca(nc, hm, mg_user_data);
            }
            else {
                //all other uris
                #ifndef MYMPD_EMBEDDED_ASSETS
                    //serve all files from filesystem
                    static struct mg_http_serve_opts s_http_server_opts;
                    s_http_server_opts.root_dir = MYMPD_DOC_ROOT;
                    if (mg_match(hm->uri, mg_str("/test/#"), NULL)) {
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
                    sds uri = sdsnewlen(hm->uri.buf, hm->uri.len);
                    webserver_serve_embedded_files(nc, uri);
                    FREE_SDS(uri);
                #endif
            }
            break;
        }
        case MG_EV_CLOSE: {
            MYMPD_LOG_INFO(NULL, "HTTP connection \"%lu\" closed", nc->id);
            mg_user_data->connection_count--;
            if (frontend_nc_data == NULL) {
                if (nc->is_listening == 0) {
                    MYMPD_LOG_WARN(NULL, "frontend_nc_data not allocated");
                }
                break;
            }
            if (frontend_nc_data->backend_nc != NULL) {
                MYMPD_LOG_INFO(NULL, "Closing backend connection \"%lu\"", frontend_nc_data->backend_nc->id);
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

/**
 * Redirects the client to https if ssl is enabled.
 * Only requests to /browse/webradios are not redirected.
 * @param nc mongoose connection
 * @param ev connection event
 * @param ev_data event data (http / websocket message)
 */
static void ev_handler_redirect(struct mg_connection *nc, int ev, void *ev_data) {
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_config *config = mg_user_data->config;

    switch(ev) {
        case MG_EV_OPEN:
            mg_user_data->connection_count++;
            break;
        case MG_EV_ACCEPT:
            //enforce connection limit
            if (enforce_conn_limit(nc, mg_user_data->connection_count) == false) {
                break;
            }
            //enforce acl
            if (enforce_acl(nc, config->acl) == false) {
                break;
            }
            break;
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;
            // Serve some directories without ssl
            if (mg_match(hm->uri, mg_str("/browse/webradios/*"), NULL)) {
                // myMPD webradio links
                static struct mg_http_serve_opts s_http_server_opts;
                s_http_server_opts.extra_headers = EXTRA_HEADERS_UNSAFE;
                s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
                s_http_server_opts.root_dir = mg_user_data->browse_directory;
                MYMPD_LOG_INFO(NULL, "Serving uri \"%.*s\"", (int)hm->uri.len, hm->uri.buf);
                mg_http_serve_dir(nc, hm, &s_http_server_opts);
                break;
            }
            #ifdef MYMPD_ENABLE_LUA
                if (mg_match(hm->uri, mg_str("/script/*/*"), NULL)) {
                    script_execute_http(nc, hm, config);
                    break;
                }
            #endif
            //redirect to https
            struct mg_str *host_hdr = mg_http_get_header(hm, "Host");
            if (host_hdr == NULL) {
                MYMPD_LOG_ERROR(NULL, "No host header found, closing connection");
                nc->is_closing = 1;
                break;
            }

            sds host_header = sdscatlen(sdsempty(), host_hdr->buf, host_hdr->len);
            int count = 0;
            sds *tokens = sdssplitlen(host_header, (ssize_t)sdslen(host_header), ":", 1, &count);
            sds s_redirect = sdscatfmt(sdsempty(), "https://%S", tokens[0]);
            if (config->ssl_port != 443) {
                s_redirect = sdscatfmt(s_redirect, ":%i", config->ssl_port);
            }
            MYMPD_LOG_INFO(NULL, "Redirecting to %s", s_redirect);
            webserver_send_header_redirect(nc, s_redirect);
            nc->is_draining = 1;
            sdsfreesplitres(tokens, count);
            FREE_SDS(host_header);
            FREE_SDS(s_redirect);
            break;
        }
        case MG_EV_CLOSE:
            MYMPD_LOG_INFO(NULL, "Connection \"%lu\" closed", nc->id);
            mg_user_data->connection_count--;
            break;
    }
}

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
        if (ch == '\n') {
            //Remove newline
            len--;
        }
        //we log only mongoose errors
        MYMPD_LOG_ERROR(NULL, "%.*s", (int) len, buf); //Send logs
        len = 0;
    }
    (void)param;
}
