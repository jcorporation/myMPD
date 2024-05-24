/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/scripts/interface_http_client.h"

#include "src/lib/filehandler.h"
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
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *method =lua_tostring(lua_vm, 1);
    if (method == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_client: method is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "method is NULL");
    }
    const char *uri = lua_tostring(lua_vm, 2);
    if (uri == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_client: uri is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "uri is NULL");
    }
    const char *extra_headers = lua_tostring(lua_vm, 3);
    if (extra_headers == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_client: extra_headers is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "extra_headers is NULL");
    }
    const char *post_data = lua_tostring(lua_vm, 4);
    if (post_data == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_client: post_data is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "post_data is NULL");
    }

    struct mg_client_request_t mg_client_request = {
        .method = method,
        .uri = uri,
        .extra_headers = extra_headers,
        .post_data = post_data
    };

    struct mg_client_response_t mg_client_response = {
        .rc = -1,
        .response_code = 0,
        .header = sdsempty(),
        .body = sdsempty()
    };

    http_client_request(&mg_client_request, &mg_client_response);
    lua_pop(lua_vm, n);
    lua_pushinteger(lua_vm, mg_client_response.rc);
    lua_pushinteger(lua_vm, mg_client_response.response_code);
    lua_pushlstring(lua_vm, mg_client_response.header, sdslen(mg_client_response.header));
    lua_pushlstring(lua_vm, mg_client_response.body, sdslen(mg_client_response.body));
    FREE_SDS(mg_client_response.header);
    FREE_SDS(mg_client_response.body);
    //return response count
    return 4;
}

/**
 * Downloads a file via http
 * @param lua_vm lua instance
 * @return number of variables on the stack with the response
 */
int lua_http_download(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 2) {
        MYMPD_LOG_ERROR(NULL, "Lua - mympd_http_download: invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *uri = lua_tostring(lua_vm, 1);
    if (uri == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_download: uri is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "uri is NULL");
    }
    const char *out = lua_tostring(lua_vm, 2);
    if (out == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_download: out is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "out is NULL");
    }

    struct mg_client_request_t mg_client_request = {
        .method = "GET",
        .uri = uri,
        .extra_headers = "",
        .post_data = ""
    };

    struct mg_client_response_t mg_client_response = {
        .rc = -1,
        .response_code = 0,
        .header = sdsempty(),
        .body = sdsempty()
    };

    http_client_request(&mg_client_request, &mg_client_response);
    if (mg_client_response.rc == 0 &&
        write_data_to_file(out, mg_client_response.body, sdslen(mg_client_response.body)) == true)
    {
        lua_pop(lua_vm, n);
        FREE_SDS(mg_client_response.header);
        FREE_SDS(mg_client_response.body);
        lua_pushinteger(lua_vm, mg_client_response.rc);
        return 1;
    }
    FREE_SDS(mg_client_response.header);
    FREE_SDS(mg_client_response.body);
    lua_pop(lua_vm, n);
    return luaL_error(lua_vm, "uri is NULL");
}
