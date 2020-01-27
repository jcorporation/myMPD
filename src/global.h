/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

//signal handler
sig_atomic_t s_signal_received;

//message queue
tiny_queue_t *web_server_queue;
tiny_queue_t *mpd_client_queue;
tiny_queue_t *mympd_api_queue;

typedef struct t_work_request {
    int conn_id; // needed to identify the connection where to send the reply
    int id; //the jsonrpc id
    sds method; //the jsonrpc method
    enum mympd_cmd_ids cmd_id;
    sds data;
} t_work_request;

typedef struct t_work_result {
    int conn_id; // needed to identify the connection where to send the reply
    int id; //the jsonrpc id
    sds method; //the jsonrpc method
    enum mympd_cmd_ids cmd_id;
    sds data;
    sds binary;
} t_work_result;

t_work_result *create_result(t_work_request *request);
t_work_result *create_result_new(int conn_id, int request_id, int cmd_id, const char *method);
t_work_request *create_request(int conn_id, int request_id, int cmd_id, const char *method, const char *data);

void free_request(t_work_request *request);
void free_result(t_work_result *result);

#endif
