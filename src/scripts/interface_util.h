/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SCRIPTS_INTERFACE_UTIL_H
#define MYMPD_API_SCRIPTS_INTERFACE_UTIL_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

int lua_util_log(lua_State *lua_vm);
int lua_util_hash(lua_State *lua_vm);
int lua_util_urlencode(lua_State *lua_vm);
int lua_util_urldecode(lua_State *lua_vm);

#endif
