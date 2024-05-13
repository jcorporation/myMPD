/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/scripts.h"

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/validate.h"
#include "src/mympd_api/scripts/scripts.h"
#include "src/web_server/utility.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/**
 * Executes the script as request handler.
 * @param nc mongoose connection
 */
bool script_execute_http(struct mg_connection *nc, struct mg_http_message *hm, struct t_config *config) {
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

    struct t_script_thread_arg *script_arg = malloc_assert(sizeof(struct t_script_thread_arg));
    script_arg->lualibs = config->lualibs;
    script_arg->localscript = true;
    script_arg->script_fullpath = script_fullpath;
    script_arg->script_name = script;
    script_arg->script_content = sdsempty();
    script_arg->partition = partition;
    script_arg->arguments = arguments;
    script_arg->start_event = SCRIPT_START_HTTP;

    int rc;
    lua_State *lua_vm = script_load(script_arg, &rc);
    if (lua_vm == NULL) {
        webserver_send_error(nc, 500, "Error executing script: Memory allocation error");
        free_t_script_thread_arg(script_arg);
        return false;
    }
    if (rc != 0) {
        lua_close(lua_vm);
        webserver_send_error(nc, 500, "Error loading script");
        free_t_script_thread_arg(script_arg);
        return false;
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
    if (nr_return == 1) {
        const char *data = lua_tostring(lua_vm, 1);
        webserver_send_raw(nc, data, strlen(data));
        lua_close(lua_vm);
        free_t_script_thread_arg(script_arg);
        return true;
    }
    //error
    webserver_send_error(nc, 500, "Error executing script");
    lua_close(lua_vm);
    free_t_script_thread_arg(script_arg);
    return false;
}
