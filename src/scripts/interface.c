/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/scripts/interface.h"

#include "src/mympd_api/lua_mympd_state.h"

/**
 * Populates the lua table from the lua_mympd_state struct
 * @param lua_vm lua instance
 * @param lua_mympd_state 
 */
void populate_lua_table(lua_State *lua_vm, struct t_list *lua_mympd_state) {
    struct t_list_node *current = lua_mympd_state->head;
    while (current != NULL) {
        struct t_lua_mympd_state_value *value = (struct t_lua_mympd_state_value *)current->user_data;
        switch (current->value_i) {
            case LUA_TYPE_STRING:
                populate_lua_table_field_p(lua_vm, current->key, value->p);
                break;
            case LUA_TYPE_INTEGER:
                populate_lua_table_field_i(lua_vm, current->key, value->i);
                break;
            case LUA_TYPE_NUMBER:
                populate_lua_table_field_f(lua_vm, current->key, value->f);
                break;
            case LUA_TYPE_BOOLEAN:
                populate_lua_table_field_b(lua_vm, current->key, value->b);
                break;
        }
        current = current->next;
    }
}

/**
 * Helper functions to push a lua table
 * @param lua_vm lua instance
 * @param key the key
 * @param value string value
 */
void populate_lua_table_field_p(lua_State *lua_vm, const char *key, const char *value) {
    lua_pushstring(lua_vm, key);
    lua_pushstring(lua_vm, value);
    lua_settable(lua_vm, -3);
}

/**
 * Helper functions to push a lua table
 * @param lua_vm lua instance
 * @param key the key
 * @param value int64_t value (lua integer)
 */
void populate_lua_table_field_i(lua_State *lua_vm, const char *key, int64_t value) {
    lua_pushstring(lua_vm, key);
    lua_pushinteger(lua_vm, (long long)value);
    lua_settable(lua_vm, -3);
}

/**
 * Helper functions to push a lua table
 * @param lua_vm lua instance
 * @param key the key
 * @param value double value (lua number)
 */
void populate_lua_table_field_f(lua_State *lua_vm, const char *key, double value) {
    lua_pushstring(lua_vm, key);
    lua_pushnumber(lua_vm, value);
    lua_settable(lua_vm, -3);
}

/**
 * Helper functions to push a lua table
 * @param lua_vm lua instance
 * @param key the key
 * @param value bool value
 */
void populate_lua_table_field_b(lua_State *lua_vm, const char *key, bool value) {
    lua_pushstring(lua_vm, key);
    lua_pushboolean(lua_vm, value);
    lua_settable(lua_vm, -3);
}

/**
 * Returns the global lua variable partition
 * @param lua_vm lua instance
 * @return partition or NULL on error
 */
const char *get_lua_global_partition(lua_State *lua_vm) {
    lua_getglobal(lua_vm, "partition");
    return lua_tostring(lua_vm, -1);
}

/**
 * Returns the global lua variable scriptname
 * @param lua_vm lua instance
 * @return scriptname or NULL on error
 */
const char *get_lua_global_scriptname(lua_State *lua_vm) {
    lua_getglobal(lua_vm, "scriptname");
    return lua_tostring(lua_vm, -1);
}
