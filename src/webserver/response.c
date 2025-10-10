/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief HTTP response handler
 */

#include "compile_time.h"
#include "src/webserver/response.h"

#include "src/lib/api.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/webserver/albumart.h"
#include "src/webserver/mg_user_data.h"
#include "src/webserver/placeholder.h"
#include "src/webserver/utility.h"

/**
 * Sends a raw http response message
 * @param mgr mongoose mgr
 * @param response jsonrpc response
 */
void webserver_send_raw_response(struct mg_mgr *mgr, struct t_work_response *response) {
    struct mg_connection *nc = get_nc_by_id(mgr, response->conn_id);
    if (nc != NULL) {
        webserver_send_raw(nc, response->data, sdslen(response->data));
    }
    free_response(response);
}

/**
 * Sends a redirect http response message
 * @param mgr mongoose mgr
 * @param response jsonrpc response
 */
void webserver_send_redirect(struct mg_mgr *mgr, struct t_work_response *response) {
    struct mg_connection *nc = get_nc_by_id(mgr, response->conn_id);
    if (nc != NULL) {
        webserver_send_header_redirect(nc, response->data, NULL);
    }
    else {
        MYMPD_LOG_ERROR(NULL, "Connection for id \"%lu\" not found", response->conn_id);
    }
    free_response(response);
}

/**
 * Sends an api response
 * @param mgr mongoose mgr
 * @param response jsonrpc response
 */
void webserver_send_api_response(struct mg_mgr *mgr, struct t_work_response *response) {
    struct mg_connection *nc = get_nc_by_id(mgr, response->conn_id);
    if (nc != NULL) {
        switch(response->cmd_id) {
            case INTERNAL_API_ALBUMART_BY_URI:
                webserver_send_albumart(nc, response->data, response->extra);
                break;
            case INTERNAL_API_ALBUMART_BY_ALBUMID:
                webserver_send_albumart_redirect(nc, response->data);
                break;
            case INTERNAL_API_FOLDERART:
                webserver_redirect_placeholder_image(nc, PLACEHOLDER_FOLDER);
                break;
            case INTERNAL_API_TAGART:
                webserver_redirect_placeholder_image(nc, PLACEHOLDER_NA);
                break;
            case INTERNAL_API_PLAYLISTART:
                if (strcmp(response->data, "smartpls") == 0) {
                    webserver_redirect_placeholder_image(nc, PLACEHOLDER_SMARTPLS);
                }
                else {
                    webserver_redirect_placeholder_image(nc, PLACEHOLDER_PLAYLIST);
                }
                break;
            default:
                MYMPD_LOG_DEBUG(response->partition, "Sending response to conn_id \"%lu\" (length: %lu): %s", nc->id, (unsigned long)sdslen(response->data), response->data);
                webserver_send_data(nc, response->data, sdslen(response->data), EXTRA_HEADERS_JSON_CONTENT);
        }
    }
    else {
        MYMPD_LOG_ERROR(NULL, "Connection for id \"%lu\" not found", response->conn_id);
    }
    free_response(response);
}

/**
 * Sends a http error response
 * @param nc mongoose connection
 * @param code http error code
 * @param msg the error message
 */
void webserver_send_error(struct mg_connection *nc, int code, const char *msg) {
    mg_http_reply(nc, code, "Content-Type: text/html\r\n",
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
          "<head>"
            "<meta charset=\"utf-8\">"
            "<title>myMPD error</title>"
          "</head>"
          "<body>"
            "<h1>myMPD error</h1>"
            "<p>%s</p>"
          "</body>"
        "</html>",
        msg);
    if (code >= 400) {
        MYMPD_LOG_ERROR(NULL, "HTTP %d: %s", code, msg);
    }
    webserver_handle_connection_close(nc);
}

/**
 * Sends a http OK reply with content-length header
 * @param nc mongoose connection
 * @param len length for the content-length header
 * @param headers extra headers to add
 */
void webserver_send_header_ok(struct mg_connection *nc, size_t len, const char *headers) {
    mg_printf(nc, "HTTP/1.1 200 OK\r\n"
        "%s"
        "Content-Length: %lu\r\n\r\n",
        headers, (unsigned long)len);
}

/**
 * Sends binary data
 * @param nc mongoose connection
 * @param data data to send
 * @param len length of the data to send
 * @param headers extra headers to add
 */
void webserver_send_data(struct mg_connection *nc, const char *data, size_t len, const char *headers) {
    MYMPD_LOG_DEBUG(NULL, "Sending %lu bytes to %lu", (unsigned long)len, nc->id);
    webserver_send_header_ok(nc, len, headers);
    mg_send(nc, data, len);
    webserver_handle_connection_close(nc);
}

/**
 * Sends a raw reply
 * @param nc mongoose connection
 * @param data data to send
 * @param len length of the data to send
 */
void webserver_send_raw(struct mg_connection *nc, const char *data, size_t len) {
    MYMPD_LOG_DEBUG(NULL, "Sending %lu bytes to %lu", (unsigned long)len, nc->id);
    mg_send(nc, data, len);
    webserver_handle_connection_close(nc);
}

/**
 * Creates and sends a jsonrpc response
 * @param nc mongoose connection
 * @param cmd_id myMPD API method
 * @param request_id jsonrpc request id
 * @param facility jsonrpc facility
 * @param severity jsonrpc severity
 * @param message message to send
 */
void webserver_send_jsonrpc_response(struct mg_connection *nc,
        enum mympd_cmd_ids cmd_id, unsigned request_id,
        enum jsonrpc_facilities facility, enum jsonrpc_severities severity,
        const char *message)
{
    sds response = jsonrpc_respond_message(sdsempty(), cmd_id, request_id,
        facility, severity, message);
    webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
    FREE_SDS(response);
}

/**
 * Serves a file defined by file from path
 * @param nc mongoose connection
 * @param hm mongoose http message
 * @param headers extra headers to add
 * @param file absolute filepath to serve
 */
void webserver_serve_file(struct mg_connection *nc, struct mg_http_message *hm,
        const char *headers, const char *file)
{
    MYMPD_LOG_DEBUG(NULL, "Serving file %s", file);
    static struct mg_http_serve_opts s_http_server_opts;
    s_http_server_opts.extra_headers = headers;
    s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
    mg_http_serve_file(nc, hm, file, &s_http_server_opts);
    webserver_handle_connection_close(nc);
}

/**
 * Sends a 301 moved permanently header
 * @param nc mongoose connection
 * @param location destination for the redirect
 * @param headers extra headers to add
 */
void webserver_send_header_redirect(struct mg_connection *nc, const char *location,
        const char *headers)
{
    MYMPD_LOG_DEBUG(NULL, "Sending 301 Moved Permanently \"%s\" to %lu", location, nc->id);
    mg_printf(nc, "HTTP/1.1 301 Moved Permanently\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n"
        EXTRA_HEADERS_CACHE
        "%s"
        "\r\n",
        location, headers);
    webserver_handle_connection_close(nc);
}

/**
 * Sends a 302 found header
 * @param nc mongoose connection
 * @param location destination for the redirect
 * @param headers extra headers to add
 */
void webserver_send_header_found(struct mg_connection *nc, const char *location,
    const char *headers)
{
    MYMPD_LOG_DEBUG(NULL, "Sending 302 Found \"%s\" to %lu", location, nc->id);
    mg_printf(nc, "HTTP/1.1 302 Found\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n"
        "%s"
        "\r\n",
        location, headers);
    webserver_handle_connection_close(nc);
}

/**
 * Replies to preflighted requests in CORS
 * @param nc mongoose connection
 */
void webserver_send_cors_reply(struct mg_connection *nc) {
    MYMPD_LOG_DEBUG(NULL, "Sending 204 No Content to %lu", nc->id);
    mg_printf(nc, "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: POST, GET, HEAD, OPTIONS\r\n"
        "Access-Control-Allow-Credentials: true\r\n"
        "Access-Control-Allow-Headers: *\r\n"
        "Access-Control-Expose-Headers: *\r\n"
        EXTRA_HEADERS_CACHE
        "\r\n");
    webserver_handle_connection_close(nc);
}

/**
 * Serves the embedded files
 * @param nc mongoose connection
 * @param uri uri to server
 * @return true on success, else false
 */
bool webserver_serve_embedded_files(struct mg_connection *nc, sds uri) {
    //decode uri
    sds uri_decoded = sds_urldecode(sdsempty(), uri, sdslen(uri), false);
    if (sdslen(uri_decoded) == 0) {
        webserver_send_error(nc, 500, "Failed to decode uri");
        FREE_SDS(uri_decoded);
        return false;
    }
    //find fileinfo
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *)nc->mgr->userdata;
    const struct t_embedded_file *p = NULL;
    for (p = mg_user_data->embedded_files; p->uri != NULL; p++) {
        if (strcmp(p->uri, uri_decoded) == 0){
            break;
        }
    }

    if (p->uri != NULL) {
        //send header
        mg_printf(nc, "HTTP/1.1 200 OK\r\n"
            EXTRA_HEADERS_SAFE
            "%s"
            "Content-Length: %d\r\n"
            "Content-Type: %s\r\n"
            "%s\r\n",
            (p->cache == true ? EXTRA_HEADERS_CACHE : ""),
            p->size,
            p->mimetype,
            (p->compressed == true ? EXTRA_HEADER_CONTENT_ENCODING : "")
        );
        //send data
        mg_send(nc, p->data, p->size);
        webserver_handle_connection_close(nc);
        FREE_SDS(uri_decoded);
        return true;
    }
    sds errormsg = sdscatfmt(sdsempty(), "Embedded asset \"%S\" not found", uri_decoded);
    webserver_send_error(nc, 404, errormsg);
    FREE_SDS(errormsg);
    FREE_SDS(uri_decoded);
    return false;
}
