/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_LUA_MYMPD_STATE_H
#define MYMPD_LUA_MYMPD_STATE_H

#include "src/lib/list.h"

#include <stdbool.h>

/**
 * Lua variable types
 */
enum lua_mympd_state_type {
    LUA_TYPE_STRING,
    LUA_TYPE_INTEGER,
    LUA_TYPE_NUMBER,
    LUA_TYPE_BOOLEAN
};

/**
 * Holds a lua variable.
 * Which member is used is defined by lua_mympd_state_type
 */
struct t_lua_mympd_state_value {
    sds p;        //!< lua string
    int64_t i;  //!< lua number
    double f;     //!< lua float
    bool b;       //!< lua bool
};

void lua_mympd_state_set_p(struct t_list *lua_mympd_state, const char *k, const char *v);
void lua_mympd_state_set_i(struct t_list *lua_mympd_state, const char *k, int64_t v);
void lua_mympd_state_set_u(struct t_list *lua_mympd_state, const char *k, unsigned v);
void lua_mympd_state_set_b(struct t_list *lua_mympd_state, const char *k, bool v);
void lua_mympd_state_set_f(struct t_list *lua_mympd_state, const char *k, double v);
void *lua_mympd_state_free(struct t_list *lua_mympd_state);

#endif
