/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <signal.h>
#include <stdlib.h>
#include <assert.h>

#include "../dist/src/sds/sds.h"
#include "../dist/src/mongoose/mongoose.h"
#include "tiny_queue.h"
#include "api.h"
#include "global.h"

t_work_result *create_result(t_work_request *request) {
    t_work_result *response = create_result_new(request->conn_id, request->id, request->cmd_id, request->method);
    return response;
}

t_work_result *create_result_new(int conn_id, int request_id, int cmd_id, const char *method) {
    t_work_result *response = (t_work_result *)malloc(sizeof(t_work_result));
    assert(response);
    response->conn_id = conn_id;
    response->id = request_id;
    response->cmd_id = cmd_id;
    response->method = sdsnew(method);
    response->data = sdsempty();
    response->binary = sdsempty();
    return response;
}

t_work_request *create_request(int conn_id, int request_id, int cmd_id, const char *method, const char *data) {
    t_work_request *request = (t_work_request *)malloc(sizeof(t_work_request));
    assert(request);
    request->conn_id = conn_id;
    request->cmd_id = cmd_id;
    request->id = request_id;
    request->method = sdsnew(method);
    request->data = sdsnew(data);
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
