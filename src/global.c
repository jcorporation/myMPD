/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "global.h"

#include <mpd/client.h>
#include "../dist/src/mongoose/mongoose.h"
#include "lua_mympd_state.h"

#include <assert.h>

_Atomic int worker_threads;
sig_atomic_t s_signal_received;
tiny_queue_t *web_server_queue;
tiny_queue_t *mympd_api_queue;
tiny_queue_t *mympd_script_queue;

t_work_result *create_result(t_work_request *request) {
    t_work_result *response = create_result_new(request->conn_id, request->id, request->cmd_id);
    return response;
}

t_work_result *create_result_new(long long conn_id, long request_id, unsigned cmd_id) {
    t_work_result *response = (t_work_result *)malloc(sizeof(t_work_result));
    assert(response);
    response->conn_id = conn_id;
    response->id = request_id;
    response->cmd_id = cmd_id;
    const char *method = get_cmd_id_method_name(cmd_id);
    response->method = sdsnew(method);
    response->data = sdsempty();
    response->binary = sdsempty();
    response->extra = NULL;
    return response;
}

t_work_request *create_request(long long conn_id, long request_id, unsigned cmd_id, const char *data) {
    t_work_request *request = (t_work_request *)malloc(sizeof(t_work_request));
    assert(request);
    request->conn_id = conn_id;
    request->cmd_id = cmd_id;
    request->id = request_id;
    const char *method = get_cmd_id_method_name(cmd_id);
    request->method = sdsnew(method);
    if (data == NULL) {
        request->data = sdscatfmt(sdsempty(), "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"%s\",\"params\":{", method);
    }
    else {
        request->data = sdsnew(data);
    }
    request->extra = NULL;
    return request;
}

void free_request(t_work_request *request) {
    if (request != NULL) {
        sdsfree(request->data);
        sdsfree(request->method);
        free(request);
    }
}

void free_result(t_work_result *result) {
    if (result != NULL) {
        sdsfree(result->data);
        sdsfree(result->method);
        sdsfree(result->binary);
        free(result);
    }
}

int expire_result_queue(tiny_queue_t *queue, time_t age) {
    t_work_result *response = NULL;
    int i = 0;
    while ((response = tiny_queue_expire(queue, age)) != NULL) {
        if (response->extra != NULL) {
            if (response->cmd_id == INTERNAL_API_SCRIPT_INIT) {
                free_lua_mympd_state(response->extra);
            }
            else {
                free(response->extra);
            }
        }
        free_result(response);
        response = NULL;
        i++;
    }
    return i;
}

int expire_request_queue(tiny_queue_t *queue, time_t age) {
    t_work_request *request = NULL;
    int i = 0;
    while ((request = tiny_queue_expire(queue, age)) != NULL) {
        if (request->extra != NULL) {
            if (request->cmd_id == INTERNAL_API_SCRIPT_INIT) {
                free_lua_mympd_state(request->extra);
            }
            else {
                free(request->extra);
            }
        }
        free_request(request);
        request = NULL;
        i++;
    }
    return i;
}
