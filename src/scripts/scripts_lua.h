/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_LIB_SCRIPTS_LUA_H
#define MYMPD_LIB_SCRIPTS_LUA_H

#include "src/scripts/scripts.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

lua_State *script_load(struct t_script_thread_arg *script_arg, int *rc);
void populate_lua_global_vars(lua_State *lua_vm, struct t_script_thread_arg *script_arg);
const char *lua_err_to_str(int rc);

#endif
