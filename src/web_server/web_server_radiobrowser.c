/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "web_server_radiobrowser.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "web_server_proxy.h"
#include "web_server_utility.h"

void radiobrowser_api(struct mg_connection *nc, struct mg_connection *backend_nc,
    enum mympd_cmd_ids cmd_id, sds body, int id)
{
    unsigned offset;
    unsigned limit;
    sds filter = NULL;
    sds searchstr = NULL;
    sds error = sdsempty();
    sds uri = sdsempty();
    const char *cmd = get_cmd_id_method_name(cmd_id);

    switch(cmd_id) {
        case MYMPD_API_CLOUD_RADIOBROWSER_SEARCH:
            if (json_get_uint(body, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &offset, &error) == true &&
                json_get_uint(body, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &limit, &error) == true &&
                json_get_string(body, "$.params.filter", 1, NAME_LEN_MAX, &filter, vcb_isprint, &error) == true &&
                json_get_string(body, "$.params.searchstr", 0, NAME_LEN_MAX, &searchstr, vcb_isname, &error) == true)
            {
                uri = sdscatprintf(uri, "/json/stations/search?offset=%ulimit=%u&%s=%s",
                    offset, limit, filter, searchstr);
            }
            break;
        default:
            error = sdscat(error, "Invalid API method for radiobrowser");
    }

    if (sdslen(error) > 0) {
        sds response = jsonrpc_respond_message(sdsempty(), cmd, id, true,
            "general", "error", error);
        MYMPD_LOG_ERROR("Error processing method \"%s\"", cmd);
        webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
        FREE_SDS(response);
    }
    else {
        bool rc = radiobrowser_send(nc, backend_nc, cmd_id, uri);
        if (rc == false) {
            sds response = jsonrpc_respond_message(sdsempty(), cmd, id, true,
                "general", "error", "Error connection to radio-browser.info");
            webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
            FREE_SDS(response);
        }
    }
    FREE_SDS(filter);
    FREE_SDS(searchstr);
    FREE_SDS(error);
    FREE_SDS(uri);
}

bool radiobrowser_send(struct mg_connection *nc, struct mg_connection *backend_nc,
        enum mympd_cmd_ids cmd_id, const char *request)
{
    const char *host = "de1.api.radio-browser.info";
    sds uri = sdscatprintf(sdsempty(), "https://%s/", host);
    backend_nc = create_backend_connection(nc, backend_nc, uri, radiobrowser_handler);
    sdsfree(uri);
    if (backend_nc != NULL) {
        switch(cmd_id) {
            case MYMPD_API_CLOUD_RADIOBROWSER_SEARCH:
                backend_nc->label[3] = 'a';
                break;
            default:
                backend_nc->label[3] = ' ';
        }
        //forward request
        MYMPD_LOG_DEBUG("Sending request \"%s\" to host \"%s\"", request, host);
        mg_printf(backend_nc, "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "User-Agent: myMPD/v9.1.0\r\n"
            "Connection: close\r\n\r\n",
            request, host);
        return true;
    }
    return false;
}

void radiobrowser_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    struct mg_connection *frontend_nc = fn_data;
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    switch(ev) {
        case MG_EV_ACCEPT: {
            mg_user_data->connection_count++;
            break;
        }
        case MG_EV_ERROR:
            MYMPD_LOG_ERROR("HTTP connection \"%lu\" failed", nc->id);
            break;
        case MG_EV_CLOSE: {
            sds method;
            switch(nc->label[3]) {
                case 'a':
                    method = sdsnew("MYMPD_API_CLOUD_RADIOBROWSER_SEARCH");
                    break;
                default:
                    method = sdsempty();
            }
            struct mg_http_message hm;
            mg_http_parse((char *)nc->recv.buf, nc->recv.len, &hm);
            sds result = jsonrpc_result_start(sdsempty(), method, 0);
            result = sdscat(result, "\"data\":");
            if (hm.body.ptr != NULL &&
                hm.body.len > 0)
            {
                result = sdscatlen(result, hm.body.ptr, hm.body.len);
            }
            else {
                result = sdscatlen(result, "[]", 2);
            }
            result = jsonrpc_result_end(result);
            webserver_send_data(frontend_nc, result, sdslen(result), "Content-Type: application/json\r\n");
            FREE_SDS(result);
            MYMPD_LOG_INFO("Backend HTTP connection %lu closed", nc->id);
            mg_user_data->connection_count--;
            if (frontend_nc != NULL) {
                //remove backend connection pointer from frontend connection
                frontend_nc->fn_data = NULL;
                //close frontend connection
                frontend_nc->is_draining = 1;
            }
            break;
        }
    }
    (void) ev_data;
}
