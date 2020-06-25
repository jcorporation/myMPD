/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>

#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "list.h"
#include "lua_mympd_state.h"

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

