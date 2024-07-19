/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lua interface for http
 */

#ifndef MYMPD_API_SCRIPTS_INTERFACE_HTTP_CLIENT_H
#define MYMPD_API_SCRIPTS_INTERFACE_HTTP_CLIENT_H

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

int lua_http_client(lua_State *lua_vm);
int lua_http_download(lua_State *lua_vm);
int lua_http_serve_file(lua_State *lua_vm);

#endif
