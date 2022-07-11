/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "lua_mympd_state.h"

#include "mem.h"
#include "sds_extras.h"

#include <stdlib.h>

//private definitions
static void _lua_mympd_state_free_user_data(struct t_list_node *current);

//public

void lua_mympd_state_set_p(struct t_list *lua_mympd_state, const char *k, const char *v) {
    struct t_lua_mympd_state_value *value = malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->p = sdsnew(v);
    list_push(lua_mympd_state, k, LUA_TYPE_STRING, NULL, value);
}

void lua_mympd_state_set_i(struct t_list *lua_mympd_state, const char *k, long long v) {
    struct t_lua_mympd_state_value *value = malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->i = v;
    list_push(lua_mympd_state, k, LUA_TYPE_INTEGER, NULL, value);
}

void lua_mympd_state_set_u(struct t_list *lua_mympd_state, const char *k, unsigned v) {
    struct t_lua_mympd_state_value *value = malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->i = (long long)v;
    list_push(lua_mympd_state, k, LUA_TYPE_INTEGER, NULL, value);
}

void lua_mympd_state_set_b(struct t_list *lua_mympd_state, const char *k, bool v) {
    struct t_lua_mympd_state_value *value = malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->b = v;
    list_push(lua_mympd_state, k, LUA_TYPE_BOOLEAN, NULL, value);
}

void *lua_mympd_state_free(struct t_list *lua_mympd_state) {
    return list_free_user_data(lua_mympd_state, _lua_mympd_state_free_user_data);
}

//private

static void _lua_mympd_state_free_user_data(struct t_list_node *current) {
    if (current->value_i == LUA_TYPE_STRING) {
        struct t_lua_mympd_state_value *user_data = (struct t_lua_mympd_state_value *)current->user_data;
        FREE_SDS(user_data->p);
    }
    FREE_PTR(current->user_data);
}
