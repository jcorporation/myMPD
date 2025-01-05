/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lua interface helpers
 */

#include "compile_time.h"
#include "src/scripts/interface.h"

#include "src/mpd_client/tags.h"
#include "src/mympd_api/lua_mympd_state.h"

/**
 * Gets the config struct from lua userdata
 * @param lua_vm lua instance
 * @return pointer to mympd config struct
 */
struct t_config *get_lua_global_config(lua_State *lua_vm) {
    lua_getglobal(lua_vm, "mympd_config");
    struct t_config *config = (struct t_config *)lua_touserdata(lua_vm, -1);
    lua_pop(lua_vm, 1);
    return config;
}

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
            case LUA_TYPE_MPD_SONG:
                populate_lua_table_field_mpd_song(lua_vm, current->key, current->user_data);
                break;
        }
        current = current->next;
    }
}

/**
 * Creates a lua sub table from a mpd song
 * @param lua_vm lua instance
 * @param key the key
 * @param song MPD song
 */
void populate_lua_table_field_mpd_song(lua_State *lua_vm, const char *key, const struct mpd_song *song) {
    lua_pushstring(lua_vm, key);
    lua_newtable(lua_vm);
    populate_lua_table_field_p(lua_vm, "uri", mpd_song_get_uri(song));
    populate_lua_table_field_i(lua_vm, "Duration", mpd_song_get_duration(song));
    for (int tag = 0; tag < MPD_TAG_COUNT; tag++) {
        unsigned i = 0;
        const char *value = mpd_song_get_tag(song, tag, i);
        if (value != NULL) {
            if (is_multivalue_tag(tag) == true) {
                lua_pushstring(lua_vm, mpd_tag_name(tag));
                lua_newtable(lua_vm);
                while (value != NULL) {
                    lua_pushnumber(lua_vm, i + 1);
                    lua_pushstring(lua_vm, value);
                    lua_settable(lua_vm, -3);
                    i++;
                    value = mpd_song_get_tag(song, tag, i);
                }
                lua_settable(lua_vm, -3);
            }
            else {
                populate_lua_table_field_p(lua_vm, mpd_tag_name(tag), value);
            }
        }
    }
    lua_settable(lua_vm, -3);
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
