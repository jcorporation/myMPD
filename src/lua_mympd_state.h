/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __LUA_MYMPD_STATE_H__
#define __LUA_MYMPD_STATE_H__

enum lua_mympd_state_type {
    LUA_TYPE_STRING,
    LUA_TYPE_INTEGER,
    LUA_TYPE_NUMBER,
    LUA_TYPE_BOOLEAN
};

struct t_lua_mympd_state_value {
    sds p;
    long i;
    double f;
    bool b;
};

void free_lua_mympd_state(struct list *lua_mympd_state);

#endif
