/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../log.h"
#include "../list.h"
#include "../lua_mympd_state.h"
#include "config_defs.h"
#include "../tiny_queue.h"
#include "../api.h"
#include "../global.h"
#include "../utility.h"
#include "mympd_api_utility.h"
#include "mympd_api_scripts.h"

#ifdef ENABLE_LUA
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"  

//private definitions
static void *mympd_api_script_execute(void *script_arg);
static sds lua_err_to_str(sds buffer, int rc, bool phrase, const char *script);
static void populate_lua_table(lua_State *lua_vm, t_lua_mympd_state *lua_mympd_state);
static void populate_lua_table_field_p(lua_State *lua_vm, const char *key, const char *value);
static void populate_lua_table_field_i(lua_State *lua_vm, const char *key, long value);
static void populate_lua_table_field_f(lua_State *lua_vm, const char *key, float value);
static void populate_lua_table_field_b(lua_State *lua_vm, const char *key, bool value);
static void register_lua_functions(lua_State *lua_vm);
static int mympd_api(lua_State *lua_vm);
static int mympd_api_raw(lua_State *lua_vm);
static int _mympd_api(lua_State *lua_vm, bool raw);
static int mympd_init(lua_State *lua_vm);

//public functions
bool mympd_api_script_start(t_config *config, const char *script) {
    pthread_t mympd_script_thread;
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        LOG_ERROR("Can not init mympd_script thread attribute");
        return false;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
        LOG_ERROR("Can not set mympd_script thread to detached");
        return false;
    }
    sds script_file = sdscatfmt(sdsempty(), "%s/scripts/%s.lua", config->varlibdir, script);
    if (pthread_create(&mympd_script_thread, &attr, mympd_api_script_execute, script_file) != 0) {
        LOG_ERROR("Can not create mympd_script thread");
        sdsfree(script_file);
        return false;
    }
    pthread_setname_np(mympd_script_thread, "mympd_script");
    expire_result_queue(mympd_script_queue, 120);
    return true;
}

//private functions
static void *mympd_api_script_execute(void *script_arg) {
    thread_logname = sdsreplace(thread_logname, "script");
    sds script_file = (sds) script_arg;
    const char *script_return_text = NULL;
    lua_State *lua_vm = luaL_newstate();
    if (lua_vm == NULL) {
        LOG_ERROR("Memory allocation error in luaL_newstate");
        sds buffer = jsonrpc_start_phrase_notify(sdsempty(), "Error executing script %{script}: Memory allocation error", false);
        buffer = tojson_char(buffer, "script", script_file, false);
        buffer = jsonrpc_end_phrase(buffer);
        ws_notify(buffer);
        sdsfree(buffer);
        sdsfree(thread_logname);
        sdsfree(script_file);
        return NULL;
    }
    luaL_openlibs(lua_vm);
    register_lua_functions(lua_vm);
    int rc = luaL_loadfilex(lua_vm, script_file, "t");
    if (rc == 0) {
        rc = lua_pcall(lua_vm, 0, 1, 0);
    }
    if (lua_gettop(lua_vm) == 1) {
        //return value on stack
        script_return_text = lua_tostring(lua_vm, 1);
    }
    if (rc == 0) {
        if (script_return_text == NULL) {
            sds buffer = jsonrpc_start_phrase_notify(sdsempty(), "Script %{script} executed successfully", false);
            buffer = tojson_char(buffer, "script", script_file, false);
            buffer = jsonrpc_end_phrase(buffer);
            ws_notify(buffer);
            sdsfree(buffer);
        }
        else {
            send_jsonrpc_notify_info(script_return_text);
        }
    }
    else {
        sds err_str = lua_err_to_str(sdsempty(), rc, true, script_file);
        if (script_return_text != NULL) {
            err_str = sdscatfmt(err_str, ": %s", script_return_text);
        }
        sds buffer = jsonrpc_start_phrase_notify(sdsempty(), err_str, true);
        buffer = tojson_char(buffer, "script", script_file, false);
        buffer = jsonrpc_end_phrase(buffer);
        ws_notify(buffer);
        sdsfree(buffer);
        err_str = sdscrop(err_str);
        err_str = lua_err_to_str(err_str, rc, false, script_file);
        if (script_return_text != NULL) {
            err_str = sdscatfmt(err_str, ": %s", script_return_text);
        }
        LOG_ERROR(err_str);
        sdsfree(err_str);
    }
    lua_close(lua_vm);
    sdsfree(script_file);
    sdsfree(thread_logname);
    return NULL;
}

static sds lua_err_to_str(sds buffer, int rc, bool phrase, const char *script) {
    switch(rc) {
        case LUA_ERRSYNTAX:
            buffer = sdscatfmt(buffer, "Error executing script %s: Syntax error during precompilation", (phrase == true ? "%{script}" : script));
            break;
        case LUA_ERRMEM:
            buffer = sdscatfmt(buffer, "Error executing script %s}: Memory allocation error", (phrase == true ? "%{script}" : script));
            break;
        case LUA_ERRGCMM:
            buffer = sdscatfmt(buffer, "Error executing script %s: Error in garbage collector", (phrase == true ? "%{script}" : script));
            break;
        case LUA_ERRFILE:
            buffer = sdscatfmt(buffer, "Error executing script %s: Can not open or read script file", (phrase == true ? "%{script}" : script));
            break;
        case LUA_ERRRUN:
            buffer = sdscatfmt(buffer, "Error executing script %s: Runtime error", (phrase == true ? "%{script}" : script));
            break;
        case LUA_ERRERR:
            buffer = sdscatfmt(buffer, "Error executing script %s: Error while running the message handler", (phrase == true ? "%{script}" : script));
            break;
        default:
            buffer = sdscatfmt(buffer, "Error executing script %s", (phrase == true ? "%{script}" : script));
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

static void register_lua_functions(lua_State *lua_vm) {
    lua_register(lua_vm, "mympd_api", mympd_api);
    lua_register(lua_vm, "mympd_api_raw", mympd_api_raw);
    lua_register(lua_vm, "mympd_init", mympd_init);
}

static int mympd_init(lua_State *lua_vm) {
    lua_pushstring(lua_vm, "MPD_API_SCRIPT_INIT");
    return mympd_api(lua_vm);
}

static int mympd_api(lua_State *lua_vm) {
    return _mympd_api(lua_vm, false);
}

static int mympd_api_raw(lua_State *lua_vm) {
    return _mympd_api(lua_vm, true);
}

static int _mympd_api(lua_State *lua_vm, bool raw) {
    int n = lua_gettop(lua_vm);
    if (raw == false && n % 2 == 0) {
        LOG_ERROR("Lua - mympd_api: invalid number of arguments");
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    if (raw == true && n != 2) {
        LOG_ERROR("Lua - mympd_api: invalid number of arguments");
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *method = lua_tostring(lua_vm, 1);
    enum mympd_cmd_ids method_id = get_cmd_id(method);
    if (method_id == MPD_API_UNKNOWN) {
        LOG_ERROR("Lua - mympd_api: Invalid method \"%s\"", method);
        return luaL_error(lua_vm, "Invalid method");
    }

    pid_t tid = gettid();
    t_work_request *request = create_request(-2, tid, method_id, method, "");
    request->data = sdscatprintf(request->data, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"method\":\"%s\",\"params\":{", tid, method);
    if (raw == false) {
        for (int i = 2; i < n; i = i + 2) {
            if (lua_isboolean(lua_vm, i + 1)) {
                request->data = tojson_bool(request->data, lua_tostring(lua_vm, i), lua_toboolean(lua_vm, i + 1), true);
            }
            else if (lua_isinteger(lua_vm, i + 2)) {
                request->data = tojson_long(request->data, lua_tostring(lua_vm, i), lua_tointeger(lua_vm, i + 1), true);
            }
            else if (lua_isnumber(lua_vm, i + 2)) {
                request->data = tojson_double(request->data, lua_tostring(lua_vm, i), lua_tonumber(lua_vm, i + 1), true);
            }
            else {
                request->data = tojson_char(request->data, lua_tostring(lua_vm, i), lua_tostring(lua_vm, i + 1), true);
            }
        }
    }
    else {
        request->data = sdscat(request->data, lua_tostring(lua_vm, 2));
    }
    request->data = sdscat(request->data, "}}");
    
    if (strncmp(method, "MYMPD_API_", 10) == 0) {
        tiny_queue_push(mympd_api_queue, request, tid);
    }
    else if (strncmp(method, "MPDWORKER_API_", 14) == 0) {
        tiny_queue_push(mpd_worker_queue, request, tid);
        
    }
    else {
        tiny_queue_push(mpd_client_queue, request, tid);
    }

    int i = 0;
    while (s_signal_received == 0 && i < 60) {
        i++;
        t_work_result *response = tiny_queue_shift(mympd_script_queue, 1000000, tid);
        if (response != NULL) {
            LOG_DEBUG("Got result: %s", response->data);
            if (raw == true) {
                lua_pushlstring(lua_vm, response->data, sdslen(response->data));
                free_result(response);
                return 1;
            }
            
            char *p_charbuf1 = NULL;
            int je = json_scanf(response->data, sdslen(response->data), "{result: {message: %Q}}", &p_charbuf1);
            if (je == 1 && p_charbuf1 != NULL) {
                if (strcmp(response->method, "MYMPD_API_SCRIPT_INIT") == 0) {
                    LOG_DEBUG("Populating lua global state table mympd");
                    t_lua_mympd_state *lua_mympd_state = (t_lua_mympd_state *) response->extra;
                    lua_newtable(lua_vm);
                    populate_lua_table(lua_vm, lua_mympd_state);    
                    lua_setglobal(lua_vm, "mympd");
                    free_t_lua_mympd_state(lua_mympd_state);
                    lua_mympd_state = NULL;
                }
                lua_pushstring(lua_vm, p_charbuf1);
                FREE_PTR(p_charbuf1);
                free_result(response);
                return 1;
            }
            
            je = json_scanf(response->data, sdslen(response->data), "{error: {message: %Q}}", &p_charbuf1);
            if (je == 1 && p_charbuf1 != NULL) {
                size_t el = strlen(p_charbuf1);
                char es[el + 1];
                snprintf(es, el, "%s", p_charbuf1);
                FREE_PTR(p_charbuf1);
                free_result(response);
                return luaL_error(lua_vm, es);
            }
            
            free_result(response);
            return luaL_error(lua_vm, "Invalid API response");
        }
    }
    return luaL_error(lua_vm, "No API response, timeout after 10s");
}

#endif
