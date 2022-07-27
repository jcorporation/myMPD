/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "request_handler.h"

#include "../lib/api.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "sessions.h"
#include "proxy.h"
#include "radiobrowser.h"
#include "webradiodb.h"

bool request_handler_api(struct mg_connection *nc, sds body, struct mg_str *auth_header,
        struct t_mg_user_data *mg_user_data, struct mg_connection *backend_nc)
{
    MYMPD_LOG_DEBUG("API request (%lld): %s", (long long)nc->id, body);

    //first check if request is valid json string
    if (validate_json(body) == false) {
        return false;
    }

    sds cmd = NULL;
    sds jsonrpc = NULL;
    int id = 0;

    if (json_get_string_cmp(body, "$.jsonrpc", 3, 3, "2.0", &jsonrpc, NULL) == false ||
        json_get_string_max(body, "$.method", &cmd, vcb_isalnum, NULL) == false ||
        json_get_int(body, "$.id", 0, 0, &id, NULL) == false)
    {
        MYMPD_LOG_ERROR("Invalid jsonrpc2 request");
        FREE_SDS(cmd);
        FREE_SDS(jsonrpc);
        return false;
    }

    MYMPD_LOG_INFO("API request (%lld): %s", (long long)nc->id, cmd);

    enum mympd_cmd_ids cmd_id = get_cmd_id(cmd);
    if (cmd_id == GENERAL_API_UNKNOWN) {
        MYMPD_LOG_ERROR("Unknown API method");
        FREE_SDS(cmd);
        FREE_SDS(jsonrpc);
        return false;
    }

    if (is_public_api_method(cmd_id) == false) {
        MYMPD_LOG_ERROR("API method %s is for internal use only", cmd);
        FREE_SDS(cmd);
        FREE_SDS(jsonrpc);
        return false;
    }

    sds session = sdsempty();
    #ifdef ENABLE_SSL
    if (sdslen(mg_user_data->config->pin_hash) > 0 &&
        is_protected_api_method(cmd_id) == true)
    {
        bool rc = false;
        if (auth_header != NULL &&
            auth_header->len == 20)
        {
            session = sdscatlen(session, auth_header->ptr, auth_header->len);
            rc = webserver_session_validate(&mg_user_data->session_list, session);
        }
        else {
            MYMPD_LOG_ERROR("No valid Authorization header found");
        }
        if (rc == false) {
            MYMPD_LOG_ERROR("API method %s is protected", cmd);
            sds response = jsonrpc_respond_message(sdsempty(), cmd_id, 0,
                JSONRPC_FACILITY_SESSION, JSONRPC_SEVERITY_ERROR,
                (cmd_id == MYMPD_API_SESSION_VALIDATE ? "Invalid session" : "Authentication required"));
            mg_printf(nc, "HTTP/1.1 401 Unauthorized\r\n"
                "WWW-Authenticate: Bearer realm=\"myMPD\"\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %d\r\n\r\n",
                (int)sdslen(response));
            mg_send(nc, response, sdslen(response));
            FREE_SDS(cmd);
            FREE_SDS(jsonrpc);
            FREE_SDS(session);
            FREE_SDS(response);
            return true;
        }
        MYMPD_LOG_INFO("API request is authorized");
    }
    #else
    (void) auth_header;
    #endif
    switch(cmd_id) {
        case MYMPD_API_SESSION_LOGIN:
        case MYMPD_API_SESSION_LOGOUT:
        case MYMPD_API_SESSION_VALIDATE:
            webserver_session_api(nc, cmd_id, body, id, session, mg_user_data);
            break;
        case MYMPD_API_CLOUD_RADIOBROWSER_CLICK_COUNT:
        case MYMPD_API_CLOUD_RADIOBROWSER_NEWEST:
        case MYMPD_API_CLOUD_RADIOBROWSER_SERVERLIST:
        case MYMPD_API_CLOUD_RADIOBROWSER_SEARCH:
        case MYMPD_API_CLOUD_RADIOBROWSER_STATION_DETAIL:
            radiobrowser_api(nc, backend_nc, cmd_id, body, id);
            break;
        case MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET:
            webradiodb_api(nc, backend_nc, cmd_id, body, id);
            break;
        default: {
            //forward API request to mympd_api_handler
            struct t_work_request *request = create_request((long long)nc->id, id, cmd_id, body);
            mympd_queue_push(mympd_api_queue, request, 0);
        }
    }
    FREE_SDS(session);
    FREE_SDS(cmd);
    FREE_SDS(jsonrpc);
    return true;
}

bool request_handler_script_api(long long conn_id, sds body) {
    MYMPD_LOG_DEBUG("Script API request (%lld): %s", conn_id, body);

    //first check if request is valid json string
    if (validate_json(body) == false) {
        return false;
    }

    sds cmd = NULL;
    sds jsonrpc = NULL;
    int id = 0;

    if (json_get_string_cmp(body, "$.jsonrpc", 3, 3, "2.0", &jsonrpc, NULL) == false ||
        json_get_string_max(body, "$.method", &cmd, vcb_isalnum, NULL) == false ||
        json_get_int(body, "$.id", 0, 0, &id, NULL) == false)
    {
        MYMPD_LOG_ERROR("Invalid jsonrpc2 request");
        FREE_SDS(cmd);
        FREE_SDS(jsonrpc);
        return false;
    }

    MYMPD_LOG_INFO("Script API request (%lld): %s", conn_id, cmd);

    enum mympd_cmd_ids cmd_id = get_cmd_id(cmd);
    if (cmd_id != INTERNAL_API_SCRIPT_POST_EXECUTE) {
        MYMPD_LOG_ERROR("API method %s is invalid for this uri", cmd);
        FREE_SDS(cmd);
        FREE_SDS(jsonrpc);
        return false;
    }

    struct t_work_request *request = create_request(conn_id, id, cmd_id, body);
    mympd_queue_push(mympd_api_queue, request, 0);

    FREE_SDS(cmd);
    FREE_SDS(jsonrpc);
    return true;
}

void request_handler_browse(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data)
{
    static struct mg_http_serve_opts s_http_server_opts;
    s_http_server_opts.extra_headers = EXTRA_HEADERS_UNSAFE;
    s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
    if (mg_http_match_uri(hm, "/browse/")) {
        sds dirs = sdsempty();
        if (mg_user_data->publish_music == true) {
            dirs = sdscat(dirs, "<tr><td><a href=\"music/\">music/</a></td><td>MPD music directory</td><td></td></tr>");
        }
        dirs = sdscat(dirs, "<tr><td><a href=\"pics/\">pics/</a></td><td>myMPD pics directory</td><td></td></tr>");
        if (mg_user_data->publish_playlists == true) {
            dirs = sdscat(dirs, "<tr><td><a href=\"playlists/\">playlists/</a></td><td>MPD playlists directory</td><td></td></tr>");
        }
        dirs = sdscat(dirs, "<tr><td><a href=\"smartplaylists/\">smartplaylists/</a></td><td>myMPD smart playlists directory</td><td></td></tr>");
        dirs = sdscat(dirs, "<tr><td><a href=\"webradios/\">webradios/</a></td><td>Webradio favorites</td><td></td></tr>");
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
        MYMPD_LOG_INFO("Serving uri \"%.*s\"", (int)hm->uri.len, hm->uri.ptr);
        mg_http_serve_dir(nc, hm, &s_http_server_opts);
    }
}

void request_handler_proxy(struct mg_connection *nc, struct mg_http_message *hm,
        struct mg_connection *backend_nc)
{
    sds query = sdsnewlen(hm->query.ptr, hm->query.len);
    sds uri_decoded = sdsempty();
    if (sdslen(query) > 4 &&
        strncmp(query, "uri=", 4) == 0)
    {
        //remove &uri=
        sdsrange(query, 4, -1);
        //decode uri
        uri_decoded = sds_urldecode(uri_decoded, query, sdslen(query), false);
        if (is_allowed_proxy_uri(uri_decoded) == true) {
            create_backend_connection(nc, backend_nc, uri_decoded, forward_backend_to_frontend);
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
    FREE_SDS(query);
    FREE_SDS(uri_decoded);
}

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
            MYMPD_LOG_ERROR("Could not convert peer ip to string");
            response = tojson_char_len(response, "ip", "", 0, false);
        }
        response = jsonrpc_respond_end(response);
        webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
        FREE_SDS(response);
    }
    else {
        sds response = jsonrpc_respond_message(sdsempty(), GENERAL_API_UNKNOWN, 0,
            JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Could not get local ip");
        webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
        FREE_SDS(response);
    }
}

#ifdef ENABLE_SSL
void request_handler_ca(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data, struct t_config *config)
{
    if (config->custom_cert == false) {
        //deliver ca certificate
        sds ca_file = sdscatfmt(sdsempty(), "%S/ssl/ca.pem", config->workdir);
        static struct mg_http_serve_opts s_http_server_opts;
        s_http_server_opts.root_dir = mg_user_data->browse_directory;
        s_http_server_opts.extra_headers = EXTRA_HEADERS_SAFE_CACHE;
        s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
        mg_http_serve_file(nc, hm, ca_file, &s_http_server_opts);
        FREE_SDS(ca_file);
    }
    else {
        webserver_send_error(nc, 404, "Custom cert enabled, don't deliver myMPD ca");
    }
}
#endif
