/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SCRIPTS_INTERFACE_H
#define MYMPD_API_SCRIPTS_INTERFACE_H

#include "src/lib/list.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdbool.h>

void populate_lua_table(lua_State *lua_vm, struct t_list *lua_mympd_state);
void populate_lua_table_field_p(lua_State *lua_vm, const char *key, const char *value);
void populate_lua_table_field_i(lua_State *lua_vm, const char *key, int64_t value);
void populate_lua_table_field_f(lua_State *lua_vm, const char *key, double value);
void populate_lua_table_field_b(lua_State *lua_vm, const char *key, bool value);

#endif
