/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lua interface for myMPD API
 */

#include "compile_time.h"
#include "src/scripts/interface_mympd_api.h"

#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"
#include "src/lib/random.h"
#include "src/mympd_api/lua_mympd_state.h"
#include "src/scripts/interface.h"

/**
 * Function that implements mympd_api lua function
 * @param lua_vm lua instance
 * @return return code
 */
int lua_mympd_api(lua_State *lua_vm) {
    //check arguments
    int n = lua_gettop(lua_vm);
    if (n != 3) {
        MYMPD_LOG_ERROR(NULL, "Lua - mympd_api: Invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    //get partition
    const char *partition = lua_tostring(lua_vm, 1);
    if (partition == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - mympd_api: partition is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "partition is NULL");
    }
    //get method
    const char *method = lua_tostring(lua_vm, 2);
    if (method == NULL) {
        MYMPD_LOG_ERROR(partition, "Lua - mympd_api: method is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "method is NULL");
    }
    enum mympd_cmd_ids cmd_id = get_cmd_id(method);
    if (cmd_id == GENERAL_API_UNKNOWN) {
        MYMPD_LOG_ERROR(partition, "Lua - mympd_api: Invalid method \"%s\"", method);
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid method");
    }
    if (is_script_api_method(cmd_id) == false) {
        MYMPD_LOG_ERROR(partition, "Lua - mympd_api: API method %s is for internal use only ", method);
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "API method is for internal use only");
    }
    const char *params = lua_tostring(lua_vm, 3);
    if (params == NULL) {
        MYMPD_LOG_ERROR(partition, "Lua - mympd_api: params is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "params is NULL");
    }
    //generate a request id
    unsigned request_id = randrange(0, UINT_MAX);
    //create the request
    struct t_work_request *request = create_request(REQUEST_TYPE_SCRIPT, 0, request_id, cmd_id, NULL, partition);
    if (params[0] != '{') {
        //param is invalid json, ignore it
        request->data = sdscatlen(request->data, "}", 1);
    }
    else {
        sdsrange(request->data, 0, -2); //trim opening curly bracket
        request->data = sdscat(request->data, params);
    }
    request->data = sdscatlen(request->data, "}", 1);
    push_request(request, request_id);
    lua_pop(lua_vm, n);
    int i = 0;
    while (s_signal_received == 0 && i < 60) {
        i++;
        struct t_work_response *response = mympd_queue_shift(script_worker_queue, 1000000, request_id);
        if (response != NULL) {
            MYMPD_LOG_DEBUG(NULL, "Got response: %s", response->data);
            if (response->cmd_id == INTERNAL_API_SCRIPT_INIT) {
                //this populates a lua table with some MPD and myMPD states
                MYMPD_LOG_DEBUG(partition, "Populating global lua table mympd_state");
                if (response->extra != NULL) {
                    struct t_list *lua_mympd_state = (struct t_list *)response->extra;
                    lua_newtable(lua_vm);
                    populate_lua_table(lua_vm, lua_mympd_state);
                    lua_setglobal(lua_vm, "mympd_state");
                    lua_mympd_state_free(lua_mympd_state);
                }
            }
            //push return code and jsonrpc response
            int rc = json_find_key(response->data, "$.error.message") == true ? 1 : 0;
            lua_pushinteger(lua_vm, rc);
            lua_pushlstring(lua_vm, response->data, sdslen(response->data));
            free_response(response);
            //return response count
            return 2;
        }
    }
    return luaL_error(lua_vm, "No API response, timeout after 60s");
}
