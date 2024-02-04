/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/scripts/interface_http_client.h"

#include "src/lib/http_client.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

/**
 * Simple HTTP client for lua
 * @param lua_vm lua instance
 * @return number of variables on the stack with the response
 */
int lua_http_client(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 4) {
        MYMPD_LOG_ERROR(NULL, "Lua - mympd_api_http_client: invalid number of arguments");
        return luaL_error(lua_vm, "Invalid number of arguments");
    }

    struct mg_client_request_t mg_client_request = {
        .method = lua_tostring(lua_vm, 1),
        .uri = lua_tostring(lua_vm, 2),
        .extra_headers = lua_tostring(lua_vm, 3),
        .post_data = lua_tostring(lua_vm, 4)
    };

    struct mg_client_response_t mg_client_response = {
        .rc = -1,
        .response_code = 0,
        .header = sdsempty(),
        .body = sdsempty()
    };

    http_client_request(&mg_client_request, &mg_client_response);

    lua_pushinteger(lua_vm, mg_client_response.rc);
    lua_pushinteger(lua_vm, mg_client_response.response_code);
    lua_pushlstring(lua_vm, mg_client_response.header, sdslen(mg_client_response.header));
    lua_pushlstring(lua_vm, mg_client_response.body, sdslen(mg_client_response.body));
    FREE_SDS(mg_client_response.header);
    FREE_SDS(mg_client_response.body);
    //return response count
    return 4;
}
