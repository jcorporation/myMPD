/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/scripts/scripts_worker.h"

#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/thread.h"
#include "src/scripts/util.h"

/**
 * Main function for the scripts_worker thread.
 * @param script_thread_arg pointer to t_script_thread_arg struct
 */
void *script_run(void *script_thread_arg) {
    thread_logname = sds_replace(thread_logname, "scripts_worker");
    set_threadname(thread_logname);
    struct t_script_thread_arg *script_arg = (struct t_script_thread_arg *) script_thread_arg;

    MYMPD_LOG_DEBUG(NULL, "Start script");
    bool rc = lua_pcall(script_arg->lua_vm, 0, 1, 0);
    MYMPD_LOG_DEBUG(NULL, "End script");

    sds result = script_get_result(script_arg->lua_vm, rc);
    if (rc == 0) {
        if (script_arg->start_event == SCRIPT_START_HTTP) {
            if (sdslen(result) == 0) {
                send_script_raw_error(script_arg->conn_id, script_arg->partition, "Empty http response from script");
            }
            else {
                send_script_raw_response(script_arg->conn_id, script_arg->partition, result);
            }
        }
        else {
            if (sdslen(result) == 0) {
                sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_SCRIPT,
                    JSONRPC_SEVERITY_INFO, "Script %{script} executed successfully",
                    2, "script", script_arg->script_name);
                ws_notify(buffer, script_arg->partition);
                FREE_SDS(buffer);
            }
            else {
                //send script return string
                send_jsonrpc_notify(JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_INFO, script_arg->partition, result);
            }
        }
    }
    else {
        if (script_arg->start_event == SCRIPT_START_HTTP) {
            send_script_raw_error(script_arg->conn_id, script_arg->partition, "Error executing script");
        }
        else {
            sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_SCRIPT,
                JSONRPC_SEVERITY_ERROR, "Error executing script %{script}: %{msg}",
                4, "script", script_arg->script_name, "msg", result);
            ws_notify(buffer, script_arg->partition);
            FREE_SDS(buffer);
        }
        MYMPD_LOG_ERROR(script_arg->partition, "Error executing script %s: %s", script_arg->script_name, result);
    }
    FREE_SDS(result);
    free_t_script_thread_arg(script_arg);
    FREE_SDS(thread_logname);
    return NULL;
}
