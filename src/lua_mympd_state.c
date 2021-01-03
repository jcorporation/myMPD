/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <assert.h>
#include <stdlib.h>

#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "list.h"
#include "lua_mympd_state.h"

void set_lua_mympd_state_p(struct list *lua_mympd_state, const char *k, const char *v) {
    struct t_lua_mympd_state_value *value = (struct t_lua_mympd_state_value *)malloc(sizeof(struct t_lua_mympd_state_value));
    assert(value);
    value->p = sdsnew(v);
    list_push(lua_mympd_state, k, LUA_TYPE_STRING, NULL, value);
}

void set_lua_mympd_state_i(struct list *lua_mympd_state, const char *k, long v) {
    struct t_lua_mympd_state_value *value = (struct t_lua_mympd_state_value *)malloc(sizeof(struct t_lua_mympd_state_value));
    assert(value);
    value->i = v;
    list_push(lua_mympd_state, k, LUA_TYPE_INTEGER, NULL, value);
}

void set_lua_mympd_state_f(struct list *lua_mympd_state, const char *k, double v) {
    struct t_lua_mympd_state_value *value = (struct t_lua_mympd_state_value *)malloc(sizeof(struct t_lua_mympd_state_value));
    assert(value);
    value->f = v;
    list_push(lua_mympd_state, k, LUA_TYPE_NUMBER, NULL, value);
}

void set_lua_mympd_state_b(struct list *lua_mympd_state, const char *k, bool v) {
    struct t_lua_mympd_state_value *value = (struct t_lua_mympd_state_value *)malloc(sizeof(struct t_lua_mympd_state_value));
    assert(value);
    value->b = v;
    list_push(lua_mympd_state, k, LUA_TYPE_BOOLEAN, NULL, value);
}

void free_lua_mympd_state(struct list *lua_mympd_state) {
    struct list_node *current = lua_mympd_state->head;
    while (current != NULL) {
        if (current->value_i == LUA_TYPE_STRING) {
            struct t_lua_mympd_state_value *u = (struct t_lua_mympd_state_value *)current->user_data;
            sdsfree(u->p);
            free(current->user_data);
            current->user_data = NULL;
        }
        current = current->next;
    }
    list_free(lua_mympd_state);
    free(lua_mympd_state);
}
