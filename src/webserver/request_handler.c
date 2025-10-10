/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief HTTP request handler
 */

#include "compile_time.h"
#include "src/webserver/request_handler.h"

#include "src/lib/api.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_query.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/webserver/proxy.h"
#include "src/webserver/response.h"
#include "src/webserver/sessions.h"
#include "src/webserver/utility.h"
#include "src/webserver/webradio.h"

/**
 * Request handler for api requests /api
 * @param nc mongoose connection
 * @param body http body (jsonrpc request)
 * @param auth_header Authentication header (myMPD session)
 * @param mg_user_data webserver configuration
  * @return true on success, else false
 */
bool request_handler_api(struct mg_connection *nc, sds body, struct mg_str *auth_header,
        struct t_mg_user_data *mg_user_data)
{
    struct t_frontend_nc_data *frontend_nc_data = (struct t_frontend_nc_data *)nc->fn_data;
    MYMPD_LOG_DEBUG(frontend_nc_data->partition, "API request (%lu): %s", nc->id, body);

    //first check if request is valid json string
    if (validate_json_object(body) == false) {
        return false;
    }

    sds cmd = NULL;
    sds jsonrpc = NULL;
    unsigned request_id = 0;

    if (json_get_string_cmp(body, "$.jsonrpc", 3, 3, "2.0", &jsonrpc, NULL) == false ||
        json_get_string_max(body, "$.method", &cmd, vcb_isalnum, NULL) == false ||
        json_get_uint_max(body, "$.id", &request_id, NULL) == false)
    {
        MYMPD_LOG_ERROR(frontend_nc_data->partition, "Invalid jsonrpc2 request");
        FREE_SDS(cmd);
        FREE_SDS(jsonrpc);
        return false;
    }

    MYMPD_LOG_INFO(frontend_nc_data->partition, "API request (%lu): %s", nc->id, cmd);

    enum mympd_cmd_ids cmd_id = get_cmd_id(cmd);
    if (cmd_id == GENERAL_API_UNKNOWN) {
        MYMPD_LOG_ERROR(frontend_nc_data->partition, "Unknown API method");
        FREE_SDS(cmd);
        FREE_SDS(jsonrpc);
        return false;
    }

    if (check_cmd_acl(cmd_id, API_INTERNAL) == true) {
        MYMPD_LOG_ERROR(frontend_nc_data->partition, "API method %s is for internal use only", cmd);
        FREE_SDS(cmd);
        FREE_SDS(jsonrpc);
        return false;
    }

    sds session = sdsempty();
    if (sdslen(mg_user_data->config->pin_hash) > 0 &&
        check_cmd_acl(cmd_id, API_PROTECTED) == true)
    {
        bool rc = false;
        if (auth_header != NULL &&
            auth_header->len == 20)
        {
            session = sdscatlen(session, auth_header->buf, auth_header->len);
            rc = webserver_session_validate(&mg_user_data->session_list, session);
        }
        else {
            MYMPD_LOG_ERROR(frontend_nc_data->partition, "No valid Authorization header found");
        }
        if (rc == false) {
            MYMPD_LOG_ERROR(frontend_nc_data->partition, "API method %s is protected", cmd);
            sds response = jsonrpc_respond_message(sdsempty(), cmd_id, 0,
                JSONRPC_FACILITY_SESSION, JSONRPC_SEVERITY_ERROR,
                (cmd_id == MYMPD_API_SESSION_VALIDATE ? "Invalid session" : "Authentication required"));
            mg_printf(nc, "HTTP/1.1 403 Forbidden\r\n"
                EXTRA_HEADERS_JSON_CONTENT
                "Content-Length: %d\r\n\r\n",
                (int)sdslen(response));
            mg_send(nc, response, sdslen(response));
            webserver_handle_connection_close(nc);
            FREE_SDS(cmd);
            FREE_SDS(jsonrpc);
            FREE_SDS(session);
            FREE_SDS(response);
            return true;
        }
        MYMPD_LOG_INFO(frontend_nc_data->partition, "API request is authorized");
    }
    switch(cmd_id) {
        case MYMPD_API_SESSION_LOGIN:
        case MYMPD_API_SESSION_LOGOUT:
        case MYMPD_API_SESSION_VALIDATE:
            webserver_session_api(nc, cmd_id, body, request_id, session, mg_user_data);
            break;
        default: {
            //forward API request to another thread
            struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, nc->id, request_id, cmd_id, body, frontend_nc_data->partition);
            if (push_request(request, 0) == false) {
                MYMPD_LOG_ERROR(frontend_nc_data->partition, "Failure pushing request to api handler for %s", cmd);
                FREE_SDS(session);
                FREE_SDS(cmd);
                FREE_SDS(jsonrpc);
                free_request(request);
                return false;
            }
        }
    }
    FREE_SDS(session);
    FREE_SDS(cmd);
    FREE_SDS(jsonrpc);
    return true;
}

/**
 * Request handler for script api requests /api/script
 * @param nc mongoose connection
 * @param body http body (jsonrpc request)
 * @return true on success, else false
 */
bool request_handler_script_api(struct mg_connection *nc, sds body) {
    struct t_frontend_nc_data *frontend_nc_data = (struct t_frontend_nc_data *)nc->fn_data;

    MYMPD_LOG_DEBUG(frontend_nc_data->partition, "Script API request (%lu): %s", nc->id, body);

    //first check if request is valid json string
    if (validate_json_object(body) == false) {
        return false;
    }

    sds cmd = NULL;
    sds jsonrpc = NULL;
    unsigned request_id = 0;

    if (json_get_string_cmp(body, "$.jsonrpc", 3, 3, "2.0", &jsonrpc, NULL) == false ||
        json_get_string_max(body, "$.method", &cmd, vcb_isalnum, NULL) == false ||
        json_get_uint_max(body, "$.id", &request_id, NULL) == false)
    {
        MYMPD_LOG_ERROR(frontend_nc_data->partition, "Invalid jsonrpc2 request");
        FREE_SDS(cmd);
        FREE_SDS(jsonrpc);
        return false;
    }

    MYMPD_LOG_INFO(frontend_nc_data->partition, "Script API request (%lu): %s", nc->id, cmd);

    enum mympd_cmd_ids cmd_id = get_cmd_id(cmd);
    if (cmd_id != INTERNAL_API_SCRIPT_POST_EXECUTE) {
        MYMPD_LOG_ERROR(frontend_nc_data->partition, "API method %s is invalid for this uri", cmd);
        FREE_SDS(cmd);
        FREE_SDS(jsonrpc);
        return false;
    }
    struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, nc->id, request_id, cmd_id, body, frontend_nc_data->partition);
    if (push_request(request, 0) == false) {
        MYMPD_LOG_ERROR(frontend_nc_data->partition, "Failure pushing request to script api handler for %s", cmd);
        FREE_SDS(cmd);
        FREE_SDS(jsonrpc);
        free_request(request);
        return false;
    }

    FREE_SDS(cmd);
    FREE_SDS(jsonrpc);
    return true;
}

/**
 * Request handler for /browse
 * @param nc mongoose connection
 * @param hm http message
 * @param mg_user_data webserver configuration
 */
void request_handler_browse(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data)
{
    static struct mg_http_serve_opts s_http_server_opts;
    s_http_server_opts.extra_headers = EXTRA_HEADERS_UNSAFE;
    s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
    if (mg_match(hm->uri, mg_str("/browse/"), NULL)) {
        sds dirs = sdsempty();
        if (mg_user_data->publish_music == true) {
            dirs = sdscat(dirs, "<tr><td><a href=\"music/\">music/</a></td><td>MPD music directory</td><td></td></tr>");
        }
        dirs = sdscat(dirs, "<tr><td><a href=\"pics/\">pics/</a></td><td>myMPD pics directory</td><td></td></tr>");
        if (mg_user_data->publish_playlists == true) {
            dirs = sdscat(dirs, "<tr><td><a href=\"playlists/\">playlists/</a></td><td>MPD playlists directory</td><td></td></tr>");
        }
        dirs = sdscat(dirs, "<tr><td><a href=\"smartplaylists/\">smartplaylists/</a></td><td>myMPD smart playlists directory</td><td></td></tr>");
        mg_http_reply(nc, 200, "Content-Type: text/html\r\n"EXTRA_HEADERS_UNSAFE, "<!DOCTYPE html>"
            "<html><head>"
            "<meta charset=\"utf-8\">"
            "<title>Index of /browse/</title>"
            "<style>%s</style></style></head>"
            "<body>"
            "<h1>Index of /browse/</h1>"
            "<table cellpadding=\"0\"><thead>"
            "<tr><th>Name</th><th>Description</th><th></th></tr>"
            "</thead>"
            "<tbody id=\"tb\">%s</tbody>"
            "</table>"
            "<address>%s</address>"
            "</body></html>",
            nc->mgr->directory_listing_css,
            dirs,
            nc->mgr->product_name
        );
        FREE_SDS(dirs);
    }
    else {
        s_http_server_opts.root_dir = mg_user_data->browse_directory;
        MYMPD_LOG_INFO(NULL, "Serving uri \"%.*s\"", (int)hm->uri.len, hm->uri.buf);
        mg_http_serve_dir(nc, hm, &s_http_server_opts);
    }
}

/**
 * Request handler for proxy connections /proxy
 * @param nc mongoose connection
 * @param hm http body
 * @param backend_nc mongoose backend connection
 */
void request_handler_proxy(struct mg_connection *nc, struct mg_http_message *hm,
        struct mg_connection *backend_nc)
{
    sds uri = get_uri_param(&hm->query, "uri=");
    if (uri != NULL) {
        if (is_allowed_proxy_uri(uri) == true) {
            create_backend_connection(nc, backend_nc, uri, forward_backend_to_frontend_stream, true);
        }
        else {
            webserver_send_error(nc, 403, "Host is not allowed");
            nc->is_draining = 1;
        }
    }
    else {
        webserver_send_error(nc, 400, "Invalid query parameter");
        nc->is_draining = 1;
    }
    FREE_SDS(uri);
}

/**
 * Request handler for proxy connections /proxy-covercache
 * @param nc mongoose connection
 * @param hm http body
 * @param backend_nc mongoose backend connection
 */
void request_handler_proxy_covercache(struct mg_connection *nc, struct mg_http_message *hm,
        struct mg_connection *backend_nc)
{
    sds uri = get_uri_param(&hm->query, "uri=");
    if (uri != NULL) {
        struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *)nc->mgr->userdata;
        if (check_imagescache(nc, hm, mg_user_data, DIR_CACHE_COVER, uri, 0) == false) {
            create_backend_connection(nc, backend_nc, uri, forward_backend_to_frontend_covercache, false);
        }
    }
    else {
        webserver_send_error(nc, 400, "Invalid query parameter");
        nc->is_draining = 1;
    }
    FREE_SDS(uri);
}

/**
 * Request handler for /serverinfo
 * @param nc mongoose connection
 */
void request_handler_serverinfo(struct mg_connection *nc) {
    struct sockaddr_storage localip;
    socklen_t len = sizeof(localip);
    if (getsockname((int)(long)nc->fd, (struct sockaddr *)(&localip), &len) == 0) {
        sds response = jsonrpc_respond_start(sdsempty(), GENERAL_API_UNKNOWN, 0);
        char addr_str[INET6_ADDRSTRLEN];
        const char *addr_str_ptr = nc->loc.is_ip6 == true ?
            inet_ntop(AF_INET6, &(((struct sockaddr_in6*)&localip)->sin6_addr), addr_str, INET6_ADDRSTRLEN) :
            inet_ntop(AF_INET, &(((struct sockaddr_in*)&localip)->sin_addr), addr_str, INET6_ADDRSTRLEN);
        if (addr_str_ptr != NULL) {
            response = tojson_char(response, "ip", addr_str_ptr, false);
        }
        else {
            MYMPD_LOG_ERROR(NULL, "Could not convert peer ip to string");
            response = tojson_char_len(response, "ip", "", 0, false);
        }
        response = jsonrpc_end(response);
        webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
        FREE_SDS(response);
    }
    else {
        webserver_send_jsonrpc_response(nc, GENERAL_API_UNKNOWN, 0, JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Could not get local ip");
    }
}

/**
 * Request handler for /ca.crt
 * @param nc mongoose connection
 * @param hm http message
 * @param mg_user_data webserver configuration
 */
void request_handler_ca(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data)
{
    if (mg_user_data->config->custom_cert == false) {
        //deliver ca certificate
        sds ca_file = sdscatfmt(sdsempty(), "%S/ssl/ca.pem", mg_user_data->config->workdir);
        webserver_serve_file(nc, hm, EXTRA_HEADERS_SAFE_CACHE, ca_file);
        FREE_SDS(ca_file);
    }
    else {
        webserver_send_error(nc, 404, "Custom cert enabled, don't deliver myMPD ca");
    }
}

/**
 * Request handler for /webradio to retrieve an extm3u for a webradio.
 * Sends the request to the mympd_api thread.
 * @param nc mongoose connection
 * @param hm http message
 * @param mg_user_data webserver configuration
 */
void request_handler_extm3u(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data)
{
    sds uri = get_uri_param(&hm->query, "uri=");
    MYMPD_LOG_INFO(NULL, "EXTM3U for %s requested", uri);
    if (uri != NULL) {
        sds buffer = webserver_webradio_get_extm3u(mg_user_data->webradio_favorites, mg_user_data->webradiodb, sdsempty(), uri);
        webserver_send_data(nc, buffer, sdslen(buffer), "Content-Type: audio/mpegurl\r\n");
        FREE_SDS(buffer);
    }
    else {
        webserver_send_error(nc, 400, "Invalid uri");
        nc->is_draining = 1;
    }
    FREE_SDS(uri);
}
