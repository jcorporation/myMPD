/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

//signal handler
extern sig_atomic_t s_signal_received;

//message queue
extern tiny_queue_t *web_server_queue;
extern tiny_queue_t *mympd_api_queue;
extern tiny_queue_t *mpd_worker_queue;
extern tiny_queue_t *mympd_script_queue;

typedef struct t_work_request {
    long long conn_id; // needed to identify the connection where to send the reply
    long id; //the jsonrpc id
    sds method; //the jsonrpc method
    enum mympd_cmd_ids cmd_id;
    sds data;
    void *extra;
} t_work_request;

typedef struct t_work_result {
    long long conn_id; // needed to identify the connection where to send the reply
    long id; //the jsonrpc id
    sds method; //the jsonrpc method
    enum mympd_cmd_ids cmd_id;
    sds data;
    sds binary;
    void *extra;
} t_work_result;

struct set_mg_user_data_request {
    sds music_directory;
    sds playlist_directory;
    sds coverimage_names;
    bool feat_library;
    bool feat_mpd_albumart;
    sds mpd_host;
    unsigned mpd_stream_port;
};

t_work_result *create_result(t_work_request *request);
t_work_result *create_result_new(long long conn_id, long request_id, int cmd_id, const char *method);
t_work_request *create_request(long long conn_id, long request_id, int cmd_id, const char *method, const char *data);
int expire_request_queue(tiny_queue_t *queue, time_t age);
int expire_result_queue(tiny_queue_t *queue, time_t age);
void free_request(t_work_request *request);
void free_result(t_work_result *result);

#endif
