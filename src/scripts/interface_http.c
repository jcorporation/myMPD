/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lua interface for http
 */

#include "compile_time.h"
#include "src/scripts/interface_http.h"

#include "src/lib/config_def.h"
#include "src/lib/filehandler.h"
#include "src/lib/http_client.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"
#include "src/lib/validate.h"
#include "src/scripts/interface.h"
#include <string.h>

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

    struct mg_client_response_t mg_client_response;
    http_client_response_init(&mg_client_response);

    http_client_request(&mg_client_request, &mg_client_response);
    lua_pop(lua_vm, n);
    lua_pushinteger(lua_vm, mg_client_response.rc);
    lua_pushinteger(lua_vm, mg_client_response.response_code);
    lua_newtable(lua_vm);
    struct t_list_node *current = mg_client_response.header.head;
    while (current != NULL) {
        populate_lua_table_field_p(lua_vm, current->key, current->value_p);
        current = current->next;
    }
    lua_pushlstring(lua_vm, mg_client_response.body, sdslen(mg_client_response.body));
    http_client_response_clear(&mg_client_response);
    //return response count
    return 4;
}

/**
 * Downloads a file via http
 * @param lua_vm lua instance
 * @return number of variables on the stack with the response
 */
int lua_http_download(lua_State *lua_vm) {
    struct t_config *config = get_lua_global_config(lua_vm);
    int n = lua_gettop(lua_vm);
    if (n != 3) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_download: invalid number of arguments: %d", n);
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *uri = lua_tostring(lua_vm, 1);
    if (uri == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_download: uri is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "uri is NULL");
    }
    const char *extra_headers = lua_tostring(lua_vm, 2);
    if (extra_headers == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_client: extra_headers is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "extra_headers is NULL");
    }
    const char *out = lua_tostring(lua_vm, 3);
    if (out == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_download: out is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "out is NULL");
    }

    if (strncmp(out, config->cachedir, sdslen(config->cachedir)) != 0 ||
        check_dir_traversal(out) == false)
    {
        MYMPD_LOG_ERROR(NULL, "Lua - http_download: invalid filename");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "invalid filename");
    }

    struct mg_client_request_t mg_client_request = {
        .method = "GET",
        .uri = uri,
        .extra_headers = extra_headers,
        .post_data = ""
    };

    struct mg_client_response_t mg_client_response;
    http_client_response_init(&mg_client_response);

    http_client_request(&mg_client_request, &mg_client_response);
    int rc = 1;
    if (mg_client_response.rc == 0 &&
        write_data_to_file(out, mg_client_response.body, sdslen(mg_client_response.body)) == true)
    {
        rc = 0;
    }
    lua_pop(lua_vm, n);
    lua_pushinteger(lua_vm, rc);
    lua_pushinteger(lua_vm, mg_client_response.response_code);
    lua_newtable(lua_vm);
    struct t_list_node *current = mg_client_response.header.head;
    while (current != NULL) {
        populate_lua_table_field_p(lua_vm, current->key, current->value_p);
        current = current->next;
    }
    http_client_response_clear(&mg_client_response);
    return 3;
}

/**
 * Function that creates a http reply to send a file
 * @param lua_vm lua instance
 * @return number of elements pushed to lua stack
 */
int lua_http_serve_file(lua_State *lua_vm) {
    struct t_config *config = get_lua_global_config(lua_vm);
    int n = lua_gettop(lua_vm);
    if (n != 1) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_serve_file: Invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *filename = lua_tostring(lua_vm, 1);
    if (filename == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - http_serve_file: filename is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "filename is NULL");
    }

    if (strncmp(filename, config->cachedir, sdslen(config->cachedir)) != 0 ||
        check_dir_traversal(filename) == false)
    {
        MYMPD_LOG_ERROR(NULL, "Lua - http_serve_file: invalid filename");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "invalid filename");
    }

    int nread;
    sds file = sds_getfile(sdsempty(), filename, MPD_BINARY_SIZE_MAX, false, true, &nread);
    if (nread == -1) {
        FREE_SDS(file);
        MYMPD_LOG_ERROR(NULL, "Lua - http_serve_file: error reading file");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "error reading file");
    }
    const char *mime_type = get_mime_type_by_magic_stream(file);
    sds reply = sdscatfmt(sdsempty(), "HTTP/1.1 200 OK\r\n"
        "Content-Length: %L\r\n"
        "Content-Type: %s\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%S",
        sdslen(file), mime_type, file);
    FREE_SDS(file);
    lua_pop(lua_vm, n);
    lua_pushlightuserdata(lua_vm, reply);
    //return response count
    return 1;
}
