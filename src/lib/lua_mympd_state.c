/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "lua_mympd_state.h"

#include "mem.h"
#include "sds_extras.h"

#include <stdlib.h>

void set_lua_mympd_state_p(struct t_list *lua_mympd_state, const char *k, const char *v) {
    struct t_lua_mympd_state_value *value = (struct t_lua_mympd_state_value *)malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->p = sdsnew(v);
    list_push(lua_mympd_state, k, LUA_TYPE_STRING, NULL, value);
}

void set_lua_mympd_state_i(struct t_list *lua_mympd_state, const char *k, long v) {
    struct t_lua_mympd_state_value *value = (struct t_lua_mympd_state_value *)malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->i = v;
    list_push(lua_mympd_state, k, LUA_TYPE_INTEGER, NULL, value);
}

void set_lua_mympd_state_b(struct t_list *lua_mympd_state, const char *k, bool v) {
    struct t_lua_mympd_state_value *value = (struct t_lua_mympd_state_value *)malloc_assert(sizeof(struct t_lua_mympd_state_value));
    value->b = v;
    list_push(lua_mympd_state, k, LUA_TYPE_BOOLEAN, NULL, value);
}

void free_lua_mympd_state(struct t_list *lua_mympd_state) {
    struct t_list_node *current = lua_mympd_state->head;
    while (current != NULL) {
        if (current->value_i == LUA_TYPE_STRING) {
            struct t_lua_mympd_state_value *user_data = (struct t_lua_mympd_state_value *)current->user_data;
            FREE_SDS(user_data->p);
            free(current->user_data);
            current->user_data = NULL;
        }
        current = current->next;
    }
    list_clear(lua_mympd_state);
    free(lua_mympd_state);
}
