/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lua interface for myMPD API
 */

#ifndef MYMPD_API_SCRIPTS_INTERFACE_MYMPD_API_H
#define MYMPD_API_SCRIPTS_INTERFACE_MYMPD_API_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

int lua_mympd_api(lua_State *lua_vm);

#endif
