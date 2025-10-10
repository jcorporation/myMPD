/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief websocket handling
 */

#include "compile_time.h"
#include "src/webserver/websocket.h"

#include "src/lib/log.h"
#include "src/webserver/utility.h"

/**
 * Broadcasts a message through all websocket connections for a specific or all partitions
 * @param mgr mongoose mgr
 * @param response jsonrpc notification
 */
void send_ws_notify(struct mg_mgr *mgr, struct t_work_response *response) {
    struct mg_connection *nc = mgr->conns;
    int send_count = 0;
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
    }
    if (send_count == 0) {
        MYMPD_LOG_DEBUG(NULL, "No websocket client connected, discarding message: %s", response->data);
    }
    free_response(response);
}

/**
 * Sends a message through the websocket to a specific client
 * We use the jsonprc id to identify the websocket connection
 * @param mgr mongoose mgr
 * @param response jsonrpc notification
 */
void send_ws_notify_client(struct mg_mgr *mgr, struct t_work_response *response) {
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
        MYMPD_LOG_DEBUG(NULL, "No websocket client with id %u connected, discarding message: %s", client_id, response->data);
    }
    free_response(response);
}
