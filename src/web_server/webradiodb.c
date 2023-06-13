/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/webradiodb.h"

#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/web_server/proxy.h"
#include "src/web_server/utility.h"

/**
 * Private definitions
 */

static bool webradiodb_send(struct mg_connection *nc, struct mg_connection *backend_nc,
        enum mympd_cmd_ids cmd_id, const char *path);
static void webradiodb_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data);
static sds webradiodb_cache_check(sds cachedir, const char *cache_file);
static bool webradiodb_cache_write(sds cachedir, const char *cache_file, const char *data, size_t data_len);

/**
 * Public functions
 */

/**
 * Handles the webradiodb api requests
 * @param nc mongoose connection
 * @param backend_nc mongoose backend connection
 * @param cmd_id jsonrpc method
 * @param request_id jsonrpc request id
 */
void webradiodb_api(struct mg_connection *nc, struct mg_connection *backend_nc,
    enum mympd_cmd_ids cmd_id, int request_id)
{
    sds error = sdsempty();
    sds uri = sdsempty();
    sds data = NULL;
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_config *config = mg_user_data->config;

    switch(cmd_id) {
        case MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET:
            data = webradiodb_cache_check(config->cachedir, FILENAME_WEBRADIODB);
            uri = sdscatfmt(uri, "/webradiodb/db/index/%s", FILENAME_WEBRADIODB);
            break;
        default:
            error = sdscat(error, "Invalid API request");
    }

    if (sdslen(error) > 0) {
        sds response = jsonrpc_respond_message(sdsempty(), cmd_id, request_id,
            JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, error);
        MYMPD_LOG_ERROR(NULL, "Error processing method \"%s\"", get_cmd_id_method_name(cmd_id));
        webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
        FREE_SDS(response);
    }
    else if (data != NULL) {
        sds response = sdsempty();
        response = jsonrpc_respond_start(response, cmd_id, request_id);
        response = sdscat(response, "\"data\":");
        response = sdscatsds(response, data);
        response = jsonrpc_end(response);
        webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
        FREE_SDS(response);
    }
    else {
        bool rc = webradiodb_send(nc, backend_nc, cmd_id, uri);
        if (rc == false) {
            sds response = jsonrpc_respond_message(sdsempty(), cmd_id, request_id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Error connecting to radio-browser.info");
            webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
            FREE_SDS(response);
        }
    }
    FREE_SDS(error);
    FREE_SDS(uri);
    FREE_SDS(data);
}

/**
 * Private functions
 */

/**
 * Checks the webradiodb cache
 * @param cachedir cache directory
 * @param cache_file file to check
 * @return file content on success, else NULL
 */
static sds webradiodb_cache_check(sds cachedir, const char *cache_file) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", cachedir, DIR_CACHE_WEBRADIODB, cache_file);
    time_t mtime = get_mtime(filepath);
    if (mtime > 0) {
        //cache it one day
        time_t expire_time = time(NULL) - (long)(24 * 60 * 60);
        if (mtime < expire_time) {
            MYMPD_LOG_DEBUG(NULL, "Expiring cache file \"%s\": %lld", filepath, (long long)mtime);
            rm_file(filepath);
        }
        else {
            sds data = sdsempty();
            int rc = sds_getfile(&data, filepath, WEBRADIODB_SIZE_MAX, true, true);
            if (rc <= 0) {
                FREE_SDS(data);
                FREE_SDS(filepath);
                return NULL;
            }
            MYMPD_LOG_DEBUG(NULL, "Found cached file \"%s\"", filepath);
            FREE_SDS(filepath);
            return data;
        }
    }
    FREE_SDS(filepath);
    return NULL;
}

/**
 * Writes the webradiodb cache file
 * @param cachedir cache directory
 * @param cache_file file to write
 * @param data data to write
 * @param data_len data length to write
 * @return true on success, else false
 */
static bool webradiodb_cache_write(sds cachedir, const char *cache_file, const char *data, size_t data_len) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", cachedir, DIR_CACHE_WEBRADIODB, cache_file);
    bool rc = write_data_to_file(filepath, data, data_len);
    FREE_SDS(filepath);
    return rc;
}

/**
 * Creates the backend connection to the webradiodb host and assigns the webradiodb_handler.
 * @param nc mongoose connection
 * @param backend_nc backend connection to create
 * @param cmd_id jsonrpc method
 * @param path path and query to send 
 * @return true on success, else false
 */
static bool webradiodb_send(struct mg_connection *nc, struct mg_connection *backend_nc,
        enum mympd_cmd_ids cmd_id, const char *path)
{
    sds uri = sdscatfmt(sdsempty(), "https://%s%s", WEBRADIODB_HOST, path);
    backend_nc = create_backend_connection(nc, backend_nc, uri, webradiodb_handler);
    FREE_SDS(uri);
    if (backend_nc != NULL) {
        struct t_backend_nc_data *backend_nc_data = (struct t_backend_nc_data *)backend_nc->fn_data;
        backend_nc_data->cmd_id = cmd_id;
        return true;
    }
    return false;
}

/**
 * Handler for the webradiodb backend connection
 * @param nc mongoose backend connection
 * @param ev mongoose event
 * @param ev_data mongoose ev_data (http response)
 * @param fn_data mongoose fn_data (t_backend_nc_data)
 */
static void webradiodb_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_backend_nc_data *backend_nc_data = (struct t_backend_nc_data *)fn_data;
    struct t_config *config = mg_user_data->config;
    switch(ev) {
        case MG_EV_CONNECT: {
            send_backend_request(nc, fn_data);
            break;
        }
        case MG_EV_ERROR:
            MYMPD_LOG_ERROR(NULL, "HTTP connection to \"%s\", connection %lu failed", backend_nc_data->uri, nc->id);
            if (backend_nc_data->frontend_nc != NULL) {
                sds response = jsonrpc_respond_message_phrase(sdsempty(), backend_nc_data->cmd_id, 0,
                        JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Could not connect to %{host}", 2, "host", WEBRADIODB_HOST);
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
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Empty response from webradiodb backend");
            }
            if (backend_nc_data->frontend_nc != NULL) {
                webserver_send_data(backend_nc_data->frontend_nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
            }
            FREE_SDS(response);
            if (backend_nc_data->cmd_id == MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET) {
                webradiodb_cache_write(config->cachedir, FILENAME_WEBRADIODB, hm->body.ptr, hm->body.len);
            }
            break;
        }
        case MG_EV_CLOSE: {
            handle_backend_close(nc, backend_nc_data);
            break;
        }
    }
}
