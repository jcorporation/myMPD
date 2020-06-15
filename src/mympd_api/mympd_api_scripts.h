/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_API_SCRIPTS_H
#define __MYMPD_API_SCRIPTS_H
#ifdef ENABLE_LUA
bool mympd_api_script_start(t_lua_mympd_state *lua_mympd_state);
#endif
#endif
