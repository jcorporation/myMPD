/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lua interface for caches
 */

#include "compile_time.h"
#include "src/scripts/interface_caches.h"

#include "src/lib/cache_disk_images.h"
#include "src/lib/cache_disk_lyrics.h"
#include "src/lib/config_def.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"
#include "src/scripts/interface.h"

#include <string.h>

/**
 * Updates the timestamp of a file
 * @param lua_vm lua instance
 * @return number of elements pushed to lua stack
 */
int lua_caches_update_mtime(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 1) {
        MYMPD_LOG_ERROR(NULL, "Lua - caches_update_mtime: Invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *filename = lua_tostring(lua_vm, 1);
    if (filename == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - caches_update_mtime: filename is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "filename is NULL");
    }

    lua_pop(lua_vm, n);
    if (update_mtime(filename) == true) {
        lua_pushnumber(lua_vm, 0);
    }
    lua_pushnumber(lua_vm, 1);
    return 1;
}

/**
 * Renames a file for the images cache
 * @param lua_vm lua instance
 * @return number of elements pushed to lua stack
 */
int lua_caches_images_write(lua_State *lua_vm) {
    struct t_config *config = get_lua_global_config(lua_vm);
    int n = lua_gettop(lua_vm);
    if (n != 4) {
        MYMPD_LOG_ERROR(NULL, "Lua - caches_images_write: Invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *type = lua_tostring(lua_vm, 1);
    if (type == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - caches_images_write: type is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "type is NULL");
    }
    const char *src = lua_tostring(lua_vm, 2);
    if (src == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - caches_images_write: src is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "src is NULL");
    }
    const char *uri = lua_tostring(lua_vm, 3);
    if (uri == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - util_covercache_write: uri is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "uri is NULL");
    }
    const char *mime_type = lua_isstring(lua_vm, 4) == 1
        ? lua_tostring(lua_vm, 4)
        : get_mime_type_by_magic_file(src);
    const char *ext = get_ext_by_mime_type(mime_type);
    if (ext == NULL) {
        lua_pop(lua_vm, n);
        lua_pushnumber(lua_vm, 1);
        lua_pushstring(lua_vm, "Unknown filetype");
        return 2;
    }
    sds dst = cache_disk_images_get_basename(config->cachedir, type, uri, 0);
    dst = sdscatfmt(dst, ".%s", ext);
    lua_pop(lua_vm, n);
    if (is_image(dst) == false) {
        FREE_SDS(dst);
        lua_pushnumber(lua_vm, 1);
        lua_pushstring(lua_vm, "File is not an image");
        return 2;
    }
    if (rename_file(src, dst) == false) {
        FREE_SDS(dst);
        lua_pushnumber(lua_vm, 1);
        lua_pushstring(lua_vm, "Failure renaming file");
        return 2;
    }
    lua_pushnumber(lua_vm, 0);
    lua_pushstring(lua_vm, dst);
    FREE_SDS(dst);
    return 2;
}

/**
 * Writes a file to the lyrics cache
 * @param lua_vm lua instance
 * @return number of elements pushed to lua stack
 */
int lua_caches_lyrics_write(lua_State *lua_vm) {
    struct t_config *config = get_lua_global_config(lua_vm);
    int n = lua_gettop(lua_vm);
    if (n != 2) {
        MYMPD_LOG_ERROR(NULL, "Lua - caches_lyrics_write: Invalid number of arguments");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *str = lua_tostring(lua_vm, 1);
    if (str == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - caches_lyrics_write: str is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "str is NULL");
    }
    const char *uri = lua_tostring(lua_vm, 2);
    if (uri == NULL) {
        MYMPD_LOG_ERROR(NULL, "Lua - caches_lyrics_write: uri is NULL");
        lua_pop(lua_vm, n);
        return luaL_error(lua_vm, "uri is NULL");
    }

    sds filename = cache_disk_lyrics_write_file(config->cachedir, uri, str);
    lua_pop(lua_vm, n);
    if (filename == NULL) {
        lua_pushnumber(lua_vm, 1);
        lua_pushstring(lua_vm, "Failure saving file");
        return 2;
    }
    lua_pushnumber(lua_vm, 0);
    lua_pushstring(lua_vm, filename);
    FREE_SDS(filename);
    return 2;
}
