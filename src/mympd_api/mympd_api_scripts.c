/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "../lua_mympd_state.h"
#include "config_defs.h"
#include "../utility.h"
#include "mympd_api_utility.h"
#include "mympd_api_scripts.h"

#ifdef ENABLE_LUA
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"  

//private definitions
static void *mympd_api_script_execute(void *lua_mympd_state_arg);
static sds lua_err_to_str(sds buffer, int rc);
static void free_t_lua_mympd_state(t_lua_mympd_state *lua_mympd_state);
static void populate_lua_table(lua_State *lua_vm, t_lua_mympd_state *lua_mympd_state);
static void populate_lua_table_field_p(lua_State *lua_vm, const char *key, const char *value);
static void populate_lua_table_field_i(lua_State *lua_vm, const char *key, long value);
static void populate_lua_table_field_f(lua_State *lua_vm, const char *key, float value);
static void populate_lua_table_field_b(lua_State *lua_vm, const char *key, bool value);

//public functions
bool mympd_api_script_start(t_lua_mympd_state *lua_mympd_state) {
    pthread_t mympd_script_thread;
    if (pthread_create(&mympd_script_thread, NULL, mympd_api_script_execute, lua_mympd_state) == 0) {
        pthread_setname_np(mympd_script_thread, "mympd_script");
        return true;
    }
    else {
        LOG_ERROR("Can't create mympd_script thread");
        return false;
    }
}

//private functions
static void *mympd_api_script_execute(void *lua_mympd_state_arg) {
    t_lua_mympd_state *lua_mympd_state = (t_lua_mympd_state *) lua_mympd_state_arg;
    const char *script_return_text = NULL;
    lua_State *lua_vm = luaL_newstate();
    lua_newtable(lua_vm);
    populate_lua_table(lua_vm, lua_mympd_state);    
    lua_setglobal(lua_vm, "mympd");
    if (lua_vm == NULL) {
        LOG_ERROR("Memory allocation error in luaL_newstate");
        sds buffer = jsonrpc_start_phrase_notify(sdsempty(), "Error executing script \"%{script}\": Memory allocation error", false);
        buffer = tojson_char(buffer, "script", lua_mympd_state->script, false);
        buffer = jsonrpc_end_phrase(buffer);
        ws_notify(buffer);
        sdsfree(buffer);
        free_t_lua_mympd_state(lua_mympd_state);
        return NULL;
    }
    luaL_openlibs(lua_vm);
    sds script_file = sdscatfmt(sdsempty(), "%s/scripts/%s.lua", lua_mympd_state->varlibdir, lua_mympd_state->script);
    int rc = luaL_loadfilex(lua_vm, script_file, "t");
    sdsfree(script_file);
    if (rc == 0) {
        rc = lua_pcall(lua_vm, 0, 1, 0);
    }
    if (rc == 0) {
        if (lua_gettop(lua_vm) == 1) {
            //return value on stack
            script_return_text = lua_tostring(lua_vm, 1);
        }
        if (script_return_text == NULL) {
            sds buffer = jsonrpc_start_phrase_notify(sdsempty(), "Script \"%{script}\" executed successfully", false);
            buffer = tojson_char(buffer, "script", lua_mympd_state->script, false);
            buffer = jsonrpc_end_phrase(buffer);
            ws_notify(buffer);
            sdsfree(buffer);
        }
        else {
            send_jsonrpc_notify_info(script_return_text);
        }
    }
    else {
        sds err_str = lua_err_to_str(sdsempty(), rc);
        sds buffer = jsonrpc_start_phrase_notify(sdsempty(), err_str, false);
        buffer = tojson_char(buffer, "script", lua_mympd_state->script, false);
        buffer = jsonrpc_end_phrase(buffer);
        ws_notify(buffer);
        sdsfree(buffer);
        LOG_ERROR(err_str);
        sdsfree(err_str);
    }
    lua_close(lua_vm);
    free_t_lua_mympd_state(lua_mympd_state);
    return NULL;
}

static sds lua_err_to_str(sds buffer, int rc) {
    switch(rc) {
        case LUA_ERRSYNTAX:
            buffer = sdscat(buffer, "Error executing script \"%{script}}\": Syntax error during precompilation");
            break;
        case LUA_ERRMEM:
            buffer = sdscat(buffer, "Error executing script \"%{script}}\": Memory allocation error");
            break;
        case LUA_ERRGCMM:
            buffer = sdscat(buffer, "Error executing script \"%{script}}\": Error in garbage collector");
            break;
        case LUA_ERRFILE:
            buffer = sdscat(buffer, "Error executing script \"%{script}}\": Can not open or read script file");
            break;
        case LUA_ERRRUN:
            buffer = sdscat(buffer, "Error executing script \"%{script}}\": Runtime error");
            break;
        case LUA_ERRERR:
            buffer = sdscat(buffer, "Error executing script \"%{script}}\": Error while running the message handler");
            break;
        default:
            buffer = sdscat(buffer, "Error executing script \"%{script}}\"");
    }
    return buffer;
}

static void populate_lua_table_field_p(lua_State *lua_vm, const char *key, const char *value) {
    lua_pushstring(lua_vm, key);
    lua_pushstring(lua_vm, value);
    lua_settable(lua_vm, -3);
}

static void populate_lua_table_field_i(lua_State *lua_vm, const char *key, long value) {
    lua_pushstring(lua_vm, key);
    lua_pushinteger(lua_vm, value);
    lua_settable(lua_vm, -3);
}

static void populate_lua_table_field_f(lua_State *lua_vm, const char *key, float value) {
    lua_pushstring(lua_vm, key);
    lua_pushnumber(lua_vm, value);
    lua_settable(lua_vm, -3);
}

static void populate_lua_table_field_b(lua_State *lua_vm, const char *key, bool value) {
    lua_pushstring(lua_vm, key);
    lua_pushboolean(lua_vm, value);
    lua_settable(lua_vm, -3);
}

static void populate_lua_table(lua_State *lua_vm, t_lua_mympd_state *lua_mympd_state) {
    populate_lua_table_field_i(lua_vm, "play_state", lua_mympd_state->play_state);
    populate_lua_table_field_i(lua_vm, "volume", lua_mympd_state->volume);
    populate_lua_table_field_i(lua_vm, "song_pos", lua_mympd_state->song_pos);
    populate_lua_table_field_i(lua_vm, "elapsed_time", lua_mympd_state->elapsed_time);
    populate_lua_table_field_i(lua_vm, "total_time", lua_mympd_state->total_time);
    populate_lua_table_field_i(lua_vm, "song_id", lua_mympd_state->song_id);
    populate_lua_table_field_i(lua_vm, "next_song_id", lua_mympd_state->next_song_id);
    populate_lua_table_field_i(lua_vm, "next_song_pos", lua_mympd_state->next_song_pos);
    populate_lua_table_field_i(lua_vm, "queue_length", lua_mympd_state->queue_length);
    populate_lua_table_field_i(lua_vm, "queue_version", lua_mympd_state->queue_version);
    populate_lua_table_field_b(lua_vm, "repeat", lua_mympd_state->repeat);
    populate_lua_table_field_b(lua_vm, "random", lua_mympd_state->random);
    populate_lua_table_field_i(lua_vm, "single_state", lua_mympd_state->single_state);
    populate_lua_table_field_b(lua_vm, "consume", lua_mympd_state->consume);
    populate_lua_table_field_i(lua_vm, "crossfade", lua_mympd_state->crossfade);
    populate_lua_table_field_f(lua_vm, "mixrampdb", lua_mympd_state->mixrampdb);
    populate_lua_table_field_f(lua_vm, "mixrampdelay", lua_mympd_state->mixrampdelay);
    populate_lua_table_field_p(lua_vm, "music_directory", lua_mympd_state->music_directory);
    populate_lua_table_field_p(lua_vm, "varlibdir", lua_mympd_state->varlibdir);
}

static void free_t_lua_mympd_state(t_lua_mympd_state *lua_mympd_state) {
    sdsfree(lua_mympd_state->script);
    sdsfree(lua_mympd_state->music_directory);
    sdsfree(lua_mympd_state->varlibdir);
    free(lua_mympd_state);
}

#endif
