/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "webradiodb.h"

#include "../lib/filehandler.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "proxy.h"
#include "utility.h"

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
 * @param body request body (jsonrpc request)
 * @param request_id jsonrpc request id
 */
void webradiodb_api(struct mg_connection *nc, struct mg_connection *backend_nc,
    enum mympd_cmd_ids cmd_id, sds body, int request_id)
{
    (void) body;
    sds error = sdsempty();
    sds uri = sdsempty();
    sds data = NULL;
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_config *config = mg_user_data->config;

    switch(cmd_id) {
        case MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET:
            data = webradiodb_cache_check(config->cachedir, "webradiodb-combined.min.json");
            uri = sdscat(uri, "/webradiodb/db/index/webradiodb-combined.min.json");
            break;
        default:
            error = sdscat(error, "Invalid API request");
    }

    if (sdslen(error) > 0) {
        sds response = jsonrpc_respond_message(sdsempty(), cmd_id, request_id,
            JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, error);
        MYMPD_LOG_ERROR("Error processing method \"%s\"", get_cmd_id_method_name(cmd_id));
        webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
        FREE_SDS(response);
    }
    else if (data != NULL) {
        sds response = sdsempty();
        response = jsonrpc_respond_start(response, cmd_id, request_id);
        response = sdscat(response, "\"data\":");
        response = sdscatsds(response, data);
        response = jsonrpc_end(response);
        webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
        FREE_SDS(response);
    }
    else {
        bool rc = webradiodb_send(nc, backend_nc, cmd_id, uri);
        if (rc == false) {
            sds response = jsonrpc_respond_message(sdsempty(), cmd_id, request_id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Error connection to radio-browser.info");
            webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
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
    sds filepath = sdscatfmt(sdsempty(), "%S/webradiodb/%s", cachedir, cache_file);
    struct stat status;
    if (stat(filepath, &status) == 0) {
        //cache it one day
        time_t expire_time = time(NULL) - (long)(24 * 60 * 60);
        if (status.st_mtime < expire_time) {
            MYMPD_LOG_DEBUG("Expiring cache file \"%s\": %lld", filepath, (long long)status.st_mtime);
            rm_file(filepath);
        }
        else {
            FILE *fp = fopen(filepath, OPEN_FLAGS_READ);
            if (fp == NULL) {
                MYMPD_LOG_ERROR("Cant open cache file \"%s\"", filepath);
                FREE_SDS(filepath);
                return NULL;
            }
            sds data = sdsempty();
            sds_getfile(&data, fp, WEBRADIODB_SIZE_MAX, true);
            (void) fclose(fp);
            MYMPD_LOG_DEBUG("Found cached file \"%s\"", filepath);
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
    sds filepath = sdscatfmt(sdsempty(), "%S/webradiodb/%s", cachedir, cache_file);
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
        struct backend_nc_data_t *backend_nc_data = (struct backend_nc_data_t *)backend_nc->fn_data;
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
 * @param fn_data mongoose fn_data (backend_nc_data_t)
 */
static void webradiodb_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct backend_nc_data_t *backend_nc_data = (struct backend_nc_data_t *)fn_data;
    struct t_config *config = mg_user_data->config;
    switch(ev) {
        case MG_EV_CONNECT: {
            send_backend_request(nc, fn_data);
            break;
        }
        case MG_EV_ERROR:
            MYMPD_LOG_ERROR("HTTP connection \"%lu\" failed", nc->id);
            break;
        case MG_EV_HTTP_MSG: {
            struct mg_http_message *hm = (struct mg_http_message *) ev_data;
            MYMPD_LOG_DEBUG("Got response from connection \"%lu\": %lu bytes", nc->id, (unsigned long)hm->body.len);
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
                webserver_send_data(backend_nc_data->frontend_nc, response, sdslen(response), "Content-Type: application/json\r\n");
            }
            FREE_SDS(response);
            if (backend_nc_data->cmd_id == MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET) {
                webradiodb_cache_write(config->cachedir, "webradiodb-combined.min.json", hm->body.ptr, hm->body.len);
            }
            break;
        }
        case MG_EV_CLOSE: {
            handle_backend_close(nc, backend_nc_data);
            break;
        }
    }
}
