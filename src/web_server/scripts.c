/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/scripts.h"

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/thread.h"
#include "src/lib/validate.h"
#include "src/scripts/scripts.h"
#include "src/scripts/scripts_lua.h"
#include "src/web_server/utility.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <pthread.h>

// private definitions
static void send_script_raw_response(unsigned long conn_id, const char *partition, const char *data);
static void send_script_raw_error(unsigned long conn_id, const char *partition, const char *data);
static void *script_execute_async_http(void *script_thread_arg);

// public functions

/**
 * Executes the script as request handler.
 * @param nc mongoose connection
 */
bool script_execute_http(struct mg_connection *nc, struct mg_http_message *hm, struct t_config *config) {
    if (http_script_threads > MAX_HTTP_SCRIPT_THREADS) {
        webserver_send_error(nc, 500, "Too many http script threads already running");
        return false;
    }
    sds partition = sdsnewlen(hm->uri.buf, hm->uri.len);
    sds script = sdsdup(partition);
    partition = sds_dirname(partition);
    partition = sds_basename(partition);
    script = sds_basename(script);
    if (vcb_isfilepath(script) == false) {
        FREE_SDS(script);
        FREE_SDS(partition);
        webserver_send_error(nc, 500, "Invalid script name");
        return false;
    }
    sds script_fullpath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", config->workdir, DIR_WORK_SCRIPTS, script);
    if (testfile_read(script_fullpath) == false) {
        FREE_SDS(script_fullpath);
        FREE_SDS(script);
        FREE_SDS(partition);
        webserver_send_error(nc, 500, "Script not found");
        return false;
    }

    //Extract arguments from query parameters
    struct t_list *arguments = list_new();
    int params_count;
    sds *params = sdssplitlen(hm->query.buf, (ssize_t)hm->query.len, "&", 1, &params_count);
    for (int i = 0; i < params_count; i++) {
        int kv_count;
        sds *kv = sdssplitlen(params[i], (ssize_t)sdslen(params[i]), "=", 1, &kv_count);
        if (kv_count == 2) {
            sds decoded = sds_urldecode(sdsempty(), kv[1], sdslen(kv[1]), false);
            list_push(arguments, kv[0], 0, decoded, NULL);
            FREE_SDS(decoded);
        }
        sdsfreesplitres(kv, kv_count);
    }
    sdsfreesplitres(params, params_count);

    struct t_script_thread_arg *script_thread_arg = malloc_assert(sizeof(struct t_script_thread_arg));
    script_thread_arg->lualibs = config->lualibs;
    script_thread_arg->localscript = true;
    script_thread_arg->script_fullpath = script_fullpath;
    script_thread_arg->script_name = script;
    script_thread_arg->script_content = sdsempty();
    script_thread_arg->partition = partition;
    script_thread_arg->arguments = arguments;
    script_thread_arg->start_event = SCRIPT_START_HTTP;
    script_thread_arg->conn_id = nc->id;

    pthread_t mympd_script_http_thread;
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0 ||
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 ||
        pthread_create(&mympd_script_http_thread, &attr, script_execute_async_http, script_thread_arg) != 0)
    {
        MYMPD_LOG_ERROR(NULL, "Can not create mympd_http script thread");
        free_t_script_thread_arg(script_thread_arg);
        return false;
    }
    http_script_threads++;
    mympd_queue_expire(mympd_script_queue, 120);
    return true;
}

// private functions

/**
 * Sends the script raw response to the webserver queue
 * @param conn_id mongoose connection id
 * @param partition MPD partition
 * @param data raw http response to send
 */
static void send_script_raw_response(unsigned long conn_id, const char *partition, const char *data) {
    struct t_work_response *response = create_response_new(RESPONSE_TYPE_RAW, conn_id, 0, INTERNAL_API_RAW, partition);
    response->data = sdscat(response->data, data);
    push_response(response);
}

/**
 * Sends the script raw error to the webserver queue
 * @param conn_id mongoose connection id
 * @param partition MPD partition
 * @param data raw http response to send
 */
static void send_script_raw_error(unsigned long conn_id, const char *partition, const char *data) {
    struct t_work_response *response = create_response_new(RESPONSE_TYPE_RAW, conn_id, 0, INTERNAL_API_RAW, partition);
    sds body = sdscatfmt(sdsempty(), "<!DOCTYPE html><html><head><title>myMPD error</title></head><body>"
        "<h1>myMPD error</h1>"
        "<p>%s</p>"
        "</body></html>",
        data);
    response->data = sdscatfmt(response->data, "HTTP/1.1 500 Internal server error\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %L\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s", sdslen(body), body);
    FREE_SDS(body);
    push_response(response);
}

/**
 * Executes the http script.
 * This is the main function of the new thread.
 * @param script_thread_arg pointer to t_script_thread_arg struct
 */
static void *script_execute_async_http(void *script_thread_arg) {
    thread_logname = sds_replace(thread_logname, "http_script");
    set_threadname(thread_logname);
    struct t_script_thread_arg *script_arg = (struct t_script_thread_arg *) script_thread_arg;
    
    int rc;
    lua_State *lua_vm = script_load(script_arg, &rc);
    if (lua_vm == NULL) {
        send_script_raw_error(script_arg->conn_id, script_arg->partition, "Error executing script: Memory allocation error");
        free_t_script_thread_arg(script_arg);
        http_script_threads--;
        FREE_SDS(thread_logname);
        return NULL;
    }
    if (rc != 0) {
        lua_close(lua_vm);
        send_script_raw_error(script_arg->conn_id, script_arg->partition, "Error loading script");
        free_t_script_thread_arg(script_arg);
        http_script_threads--;
        FREE_SDS(thread_logname);
        return NULL;
    }

    populate_lua_global_vars(lua_vm, script_arg);
    //execute script
    MYMPD_LOG_DEBUG(NULL, "Start script");
    rc = lua_pcall(lua_vm, 0, 1, 0);
    MYMPD_LOG_DEBUG(NULL, "End script");

    //it should be one value on the stack
    int nr_return = lua_gettop(lua_vm);
    MYMPD_LOG_DEBUG(NULL, "Lua script returns %d values", nr_return);
    for (int i = 1; i <= nr_return; i++) {
        MYMPD_LOG_DEBUG(NULL, "Lua script return value %d: %s", i, lua_tostring(lua_vm, i));
    }
    if (nr_return == 1 && rc == 0) {
        const char *data = lua_tostring(lua_vm, 1);
        if (data != NULL) {
            send_script_raw_response(script_arg->conn_id, script_arg->partition, data);
        }
        else {
            send_script_raw_error(script_arg->conn_id, script_arg->partition, "Empty http response from script");
        }
        lua_close(lua_vm);
        free_t_script_thread_arg(script_arg);
        http_script_threads--;
        FREE_SDS(thread_logname);
        return NULL;
    }
    //error
    MYMPD_LOG_ERROR(script_arg->partition, "Lua error: %s", lua_err_to_str(rc));
    send_script_raw_error(script_arg->conn_id, script_arg->partition, "Error executing script");
    lua_close(lua_vm);
    free_t_script_thread_arg(script_arg);
    FREE_SDS(thread_logname);
    http_script_threads--;
    return NULL;
}
