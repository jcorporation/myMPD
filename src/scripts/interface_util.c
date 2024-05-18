/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/scripts/interface_util.h"

#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/scripts/interface.h"

#include <string.h>

/**
 * Function that notifies all clients in a partition or a specific client
 * @param lua_vm lua instance
 * @return return code
 */
int lua_util_notify(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 3) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_notify: invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    unsigned request_id = (unsigned)lua_tonumber(lua_vm, 1);
    unsigned severity = (unsigned)lua_tonumber(lua_vm, 2);
    const char *message = lua_tostring(lua_vm, 3);
    if (message == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_notify: message is a NULL string");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "NULL string");
    }
    if (severity >= JSONRPC_SEVERITY_MAX) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_notify: invalid severity");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid severity");
    }
    const char *partition = get_lua_global_partition(lua_vm);
    if (partition == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_notify: Invalid partition");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid partition");
    }
    sds message_sds = jsonrpc_notify(sdsempty(), JSONRPC_FACILITY_SCRIPT, severity, message);
    if (request_id == 0) {
        ws_notify(message_sds, partition);
    }
    else {
        ws_notify_client(message_sds, request_id);
    }
    lua_pop(lua_vm, n);
    return 0;
}

/**
 * Function that implements logging
 * @param lua_vm lua instance
 * @return return code
 */
int lua_util_log(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 2) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_log: invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    unsigned level = (unsigned)lua_tonumber(lua_vm, 1);
    const char *message = lua_tostring(lua_vm, 2);
    if (message == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_log: NULL string");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "NULL string");
    }
    const char *partition = get_lua_global_partition(lua_vm);
    if (partition == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_log: Invalid partition");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid partition");
    }
    const char *scriptname = get_lua_global_scriptname(lua_vm);
    if (partition == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_log: Invalid scriptname");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid scriptname");
    }

    switch(level) {
        case LOG_EMERG:    MYMPD_LOG_EMERG(partition, "%s: %s", scriptname, message); break;
        case LOG_ALERT:    MYMPD_LOG_ALERT(partition, "%s: %s", scriptname, message); break;
        case LOG_CRIT:     MYMPD_LOG_CRIT(partition, "%s: %s", scriptname, message); break;
        case LOG_ERR:      MYMPD_LOG_ERROR(partition, "%s: %s", scriptname, message); break;
        case LOG_WARNING:  MYMPD_LOG_WARN(partition, "%s: %s", scriptname, message); break;
        case LOG_NOTICE:   MYMPD_LOG_NOTICE(partition, "%s: %s", scriptname, message); break;
        case LOG_INFO:     MYMPD_LOG_INFO(partition, "%s: %s", scriptname, message); break;
        case LOG_DEBUG:    MYMPD_LOG_DEBUG(partition, "%s: %s", scriptname, message); break;
        default:
            lua_pop(lua_vm, n);
            MYMPD_LOG_ERROR(NULL, "Lua - util_log: invalid loglevel");
            return luaL_error(lua_vm, "invalid loglevel");
    }
    return 0;
}

/**
 * Function that implements hashing functions
 * @param lua_vm lua instance
 * @return return code
 */
int lua_util_hash(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 2) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_hash: invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *str = lua_tostring(lua_vm, 1);
    const char *alg = lua_tostring(lua_vm, 2);

    if (str == NULL ||
        alg == NULL)
    {
        MYMPD_LOG_ERROR(NULL, "Lua - util_hash: NULL string");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "NULL string");
    }

    sds hash = NULL;
    if (strcmp(alg, "sha1") == 0) {
        hash = sds_hash_sha1(str);
    }
    else if (strcmp(alg, "sha256") == 0) {
        hash = sds_hash_sha256(str);
    }
    else {
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Unable to connect to myGPIOd");
    }
    lua_pop(lua_vm, n);
    lua_pushstring(lua_vm, hash);
    FREE_SDS(hash);
    //return response count
    return 1;
}

/**
 * Function that implements url encoding
 * @param lua_vm lua instance
 * @return return code
 */
int lua_util_urlencode(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 1) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_urlencode: invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *str = lua_tostring(lua_vm, 1);
    if (str == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_urlencode: NULL string");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "NULL string");
    }

    sds encoded = sds_urlencode(sdsempty(), str, strlen(str));
    lua_pop(lua_vm, n);
    lua_pushstring(lua_vm, encoded);
    FREE_SDS(encoded);
    //return response count
    return 1;
}

/**
 * Function that implements url decoding
 * @param lua_vm lua instance
 * @return return code
 */
int lua_util_urldecode(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 2) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_urldecode: invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *str = lua_tostring(lua_vm, 1);
    bool form = lua_toboolean(lua_vm, 2);

    if (str == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_urldecode: NULL string");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "NULL string");
    }
    sds decoded = sds_urldecode(sdsempty(), str, strlen(str), form);
    lua_pop(lua_vm, n);
    lua_pushstring(lua_vm, decoded);
    FREE_SDS(decoded);
    //return response count
    return 1;
}
