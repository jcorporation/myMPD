/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SCRIPTS_INTERFACE_MYGPIO_H
#define MYMPD_API_SCRIPTS_INTERFACE_MYGPIO_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

int lua_mygpio_gpio_blink(lua_State *lua_vm);
int lua_mygpio_gpio_get(lua_State *lua_vm);
int lua_mygpio_gpio_set(lua_State *lua_vm);
int lua_mygpio_gpio_toggle(lua_State *lua_vm);

#endif
