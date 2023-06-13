/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/radiobrowser.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/web_server/proxy.h"
#include "src/web_server/utility.h"

/**
 * Private definitions
 */
static bool radiobrowser_send(struct mg_connection *nc, struct mg_connection *backend_nc,
        enum mympd_cmd_ids cmd_id, const char *path);
static void radiobrowser_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data);

/**
 * Public functions
 */

/**
 * Handles the radiobrowser api requests
 * @param nc mongoose connection
 * @param backend_nc mongoose backend connection
 * @param cmd_id jsonrpc method
 * @param body request body (jsonrpc request)
 * @param request_id jsonrpc request id
 */
void radiobrowser_api(struct mg_connection *nc, struct mg_connection *backend_nc,
        enum mympd_cmd_ids cmd_id, sds body, int request_id)
{
    long offset;
    long limit;
    sds tags = NULL;
    sds country = NULL;
    sds language = NULL;
    sds searchstr = NULL;
    sds uuid = NULL;
    sds error = sdsempty();
    sds uri = sdsempty();
    const char *cmd = get_cmd_id_method_name(cmd_id);

    switch(cmd_id) {
        case MYMPD_API_CLOUD_RADIOBROWSER_CLICK_COUNT:
            if (json_get_string(body, "$.params.uuid", 0, FILEPATH_LEN_MAX, &uuid, vcb_isalnum, &error) == true) {
                uri = sdscatfmt(uri, "/json/url/%S", uuid);
            }
            break;
        case MYMPD_API_CLOUD_RADIOBROWSER_NEWEST:
            if (json_get_long(body, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &offset, &error) == true &&
                json_get_long(body, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &limit, &error) == true)
            {
                uri = sdscatfmt(uri, "/json/stations/lastchange?hidebroken=true&offset=%l&limit=%l", offset, limit);
            }
            break;
        case MYMPD_API_CLOUD_RADIOBROWSER_SEARCH:
            if (json_get_long(body, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &offset, &error) == true &&
                json_get_long(body, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &limit, &error) == true &&
                json_get_string(body, "$.params.tags", 0, NAME_LEN_MAX, &tags, vcb_isname, &error) == true &&
                json_get_string(body, "$.params.country", 0, NAME_LEN_MAX, &country, vcb_isname, &error) == true &&
                json_get_string(body, "$.params.language", 0, NAME_LEN_MAX, &language, vcb_isname, &error) == true &&
                json_get_string(body, "$.params.searchstr", 0, NAME_LEN_MAX, &searchstr, vcb_isname, &error) == true)
            {
                sds searchstr_encoded = sds_urlencode(sdsempty(), searchstr, sdslen(searchstr));
                uri = sdscatfmt(uri, "/json/stations/search?hidebroken=true&offset=%l&limit=%l&name=%S&tag=%S&country=%S&language=%S",
                    offset, limit, searchstr_encoded, tags, country, language);
                FREE_SDS(searchstr_encoded);
            }
            break;
        case MYMPD_API_CLOUD_RADIOBROWSER_SERVERLIST:
            uri = sdscat(uri, "/json/servers");
            break;
        case MYMPD_API_CLOUD_RADIOBROWSER_STATION_DETAIL:
            if (json_get_string(body, "$.params.uuid", 0, FILEPATH_LEN_MAX, &uuid, vcb_isalnum, &error) == true) {
                uri = sdscatfmt(uri, "/json/stations/byuuid?uuids=%S", uuid);
            }
            break;
        default:
            error = sdscat(error, "Invalid API request");
    }

    if (sdslen(error) > 0) {
        sds response = jsonrpc_respond_message(sdsempty(), cmd_id, request_id,
            JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, error);
        MYMPD_LOG_ERROR(NULL, "Error processing method \"%s\"", cmd);
        webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
        FREE_SDS(response);
    }
    else {
        bool rc = radiobrowser_send(nc, backend_nc, cmd_id, uri);
        if (rc == false) {
            sds response = jsonrpc_respond_message(sdsempty(), cmd_id, request_id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Error connecting to radio-browser.info");
            webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
            FREE_SDS(response);
        }
    }
    FREE_SDS(tags);
    FREE_SDS(country);
    FREE_SDS(language);
    FREE_SDS(searchstr);
    FREE_SDS(uuid);
    FREE_SDS(error);
    FREE_SDS(uri);
}

/**
 * Private functions
 */

/**
 * Creates the backend connection to the radiobrowser server and assigns the webradiodb_handler.
 * @param nc mongoose connection
 * @param backend_nc mongoose backend connection
 * @param cmd_id jsonrpc method
 * @param path path and query to send 
 * @return true on success, else false
 */
static bool radiobrowser_send(struct mg_connection *nc, struct mg_connection *backend_nc,
        enum mympd_cmd_ids cmd_id, const char *path)
{
    const char *host = RADIOBROWSER_HOST;
    sds uri = sdscatfmt(sdsempty(), "https://%s%s", host, path);
    backend_nc = create_backend_connection(nc, backend_nc, uri, radiobrowser_handler);
    FREE_SDS(uri);
    if (backend_nc != NULL) {
        struct t_backend_nc_data *backend_nc_data = (struct t_backend_nc_data *)backend_nc->fn_data;
        backend_nc_data->cmd_id = cmd_id;
        return true;
    }
    return false;
}

/**
 * Handler for the radiobrowser backend connection
 * @param nc mongoose backend connection
 * @param ev mongoose event
 * @param ev_data mongoose ev_data (http response)
 * @param fn_data mongoose fn_data (t_backend_nc_data)
 */
static void radiobrowser_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    struct t_backend_nc_data *backend_nc_data = (struct t_backend_nc_data *)fn_data;
    switch(ev) {
        case MG_EV_CONNECT: {
            send_backend_request(nc, fn_data);
            break;
        }
        case MG_EV_ERROR:
            MYMPD_LOG_ERROR(NULL, "HTTP connection to \"%s\", connection %lu failed", backend_nc_data->uri, nc->id);
            if (backend_nc_data->frontend_nc != NULL) {
                sds response = jsonrpc_respond_message_phrase(sdsempty(), backend_nc_data->cmd_id, 0,
                        JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Could not connect to %{host}", 2, "host", RADIOBROWSER_HOST);
                webserver_send_data(backend_nc_data->frontend_nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
                FREE_SDS(response);
            }
            break;
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;
            MYMPD_LOG_DEBUG(NULL, "Got response from connection \"%lu\": %lu bytes", nc->id, (unsigned long)hm->body.len);
            sds response = sdsempty();
            if (hm->body.len > 0) {
                response = jsonrpc_respond_start(response, backend_nc_data->cmd_id, 0);
                response = sdscat(response, "\"data\":");
                response = sdscatlen(response, hm->body.ptr, hm->body.len);
                response = jsonrpc_end(response);
            }
            else {
                response = jsonrpc_respond_message(response, backend_nc_data->cmd_id, 0,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Empty response from radio-browser.info");
            }
            if (backend_nc_data->frontend_nc != NULL) {
                webserver_send_data(backend_nc_data->frontend_nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
            }
            FREE_SDS(response);
            break;
        }
        case MG_EV_CLOSE: {
            handle_backend_close(nc, backend_nc_data);
            break;
        }
    }
}
