/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lua interface for caches
 */

#ifndef MYMPD_API_SCRIPTS_INTERFACE_CACHES_H
#define MYMPD_API_SCRIPTS_INTERFACE_CACHES_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

int lua_caches_tmp_file(lua_State *lua_vm);
int lua_caches_update_mtime(lua_State *lua_vm);
int lua_caches_images_write(lua_State *lua_vm);
int lua_caches_lyrics_write(lua_State *lua_vm);

#endif
