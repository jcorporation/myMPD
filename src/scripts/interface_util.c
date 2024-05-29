/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/scripts/interface_util.h"

#include "src/lib/api.h"
#include "src/lib/cache_disk_cover.h"
#include "src/lib/cache_disk_lyrics.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"

#include <string.h>

/**
 * Function that notifies all clients in a partition or a specific client
 * @param lua_vm lua instance
 * @return return code
 */
int lua_util_notify(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 4) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_notify: invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *partition = lua_tostring(lua_vm, 1);
    if (partition == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_notify: partition is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "partition is NULL");
    }
    unsigned request_id = (unsigned)lua_tonumber(lua_vm, 2);
    unsigned severity = (unsigned)lua_tonumber(lua_vm, 3);
    const char *message = lua_tostring(lua_vm, 4);
    if (message == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_notify: message is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "message is NULL");
    }
    if (severity >= JSONRPC_SEVERITY_MAX) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_notify: Invalid severity");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid severity");
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
    if (n != 4) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_log: Invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *partition = lua_tostring(lua_vm, 1);
    if (partition == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_log: partition is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "partition is NULL");
    }
    const char *scriptname = lua_tostring(lua_vm, 2);
    if (scriptname == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_log: scriptname is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "scriptname is NULL");
    }
    unsigned level = (unsigned)lua_tonumber(lua_vm, 3);
    const char *message = lua_tostring(lua_vm, 4);
    if (message == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_log: message is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "message is NULL");
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
            MYMPD_LOG_ERROR(NULL, "Lua - util_log: Invalid loglevel");
            return luaL_error(lua_vm, "Invalid loglevel");
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
        MYMPD_LOG_ERROR(NULL, "Lua - util_hash: Invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *str = lua_tostring(lua_vm, 1);
    if (str == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_hash: string is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "string is NULL");
    }
    const char *alg = lua_tostring(lua_vm, 2);
    if (alg == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_hash: alg is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "alg is NULL");
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
        MYMPD_LOG_ERROR(NULL, "Lua - util_hash: Invalid hash method");
        return luaL_error(lua_vm, "Invalid hash method");
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
        MYMPD_LOG_ERROR(NULL, "Lua - util_urlencode: Invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *str = lua_tostring(lua_vm, 1);
    if (str == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_urlencode: string is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "string is NULL");
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
        MYMPD_LOG_ERROR(NULL, "Lua - util_urldecode: Invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *str = lua_tostring(lua_vm, 1);
    bool form = lua_toboolean(lua_vm, 2);

    if (str == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_urldecode: string is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "string is NULL");
    }
    sds decoded = sds_urldecode(sdsempty(), str, strlen(str), form);
    lua_pop(lua_vm, n);
    lua_pushstring(lua_vm, decoded);
    FREE_SDS(decoded);
    //return response count
    return 1;
}

/**
 * Renames a file for the cover cache
 * @param lua_vm lua instance
 * @return 0 on success
 */
int lua_util_covercache_write(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 3) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_covercache_write: Invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *cachedir = lua_tostring(lua_vm, 1);
    if (cachedir == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_covercache_write: cachedir is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "cachedir is NULL");
    }
    const char *src = lua_tostring(lua_vm, 2);
    if (src == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_covercache_write: src is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "src is NULL");
    }
    const char *uri = lua_tostring(lua_vm, 3);
    if (uri == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_covercache_write: uri is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "uri is NULL");
    }

    const char *mime_type = get_mime_type_by_magic_file(src);
    const char *ext = get_ext_by_mime_type(mime_type);
    if (ext == NULL) {
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Unknown filetype");
    }
    sds dst = cache_disk_cover_get_basename(cachedir, uri, 0);
    dst = sdscatfmt(dst, ".%s", ext);
    lua_pop(lua_vm, n);
    if (is_image(dst) == false) {
        FREE_SDS(dst);
        return luaL_error(lua_vm, "File is not an image");
    }
    if (rename_file(src, dst) == false) {
        FREE_SDS(dst);
        return luaL_error(lua_vm, "Failure renaming file");
    }
    FREE_SDS(dst);
    lua_pushinteger(lua_vm, 0);
    return 1;
}

/**
 * Writes a file to the lyrics cache
 * @param lua_vm lua instance
 * @return 0 on success
 */
int lua_util_lyricscache_write(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 3) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_lyricscache_write: Invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *cachedir = lua_tostring(lua_vm, 1);
    if (cachedir == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_lyricscache_write: cachedir is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "cachedir is NULL");
    }
    const char *str = lua_tostring(lua_vm, 2);
    if (str == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_lyricscache_write: str is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "str is NULL");
    }
    const char *uri = lua_tostring(lua_vm, 3);
    if (uri == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_lyricscache_write: uri is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "uri is NULL");
    }

    if (cache_disk_lyrics_write_file(cachedir, uri, str) == false) {
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Failure saving file");
    }

    lua_pop(lua_vm, n);
    lua_pushinteger(lua_vm, 0);
    return 1;
}
