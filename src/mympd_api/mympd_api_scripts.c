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

    #include "mympd_api_scripts_lualibs.c"

//private definitions
struct t_script_thread_arg {
    t_config *config;
    bool localscript;
    sds script_fullpath;
    sds script_name;
    sds script_content;
    struct list *arguments;
};

static void *mympd_api_script_execute(void *script_thread_arg);
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
static void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg);
static bool mympd_luaopen(lua_State *lua_vm, const char *lualib);

//public functions
bool mympd_api_script_start(t_config *config, const char *script, struct list *arguments, bool localscript) {
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
    struct t_script_thread_arg *script_thread_arg = (struct t_script_thread_arg *)malloc(sizeof(struct t_script_thread_arg));
    script_thread_arg->config = config;
    script_thread_arg->localscript = localscript;
    script_thread_arg->arguments = arguments;
    if (localscript == true) {
        script_thread_arg->script_name = sdsnew(script);
        script_thread_arg->script_fullpath = sdscatfmt(sdsempty(), "%s/scripts/%s.lua", config->varlibdir, script);
        script_thread_arg->script_content = sdsempty();
    }
    else {
        script_thread_arg->script_name = sdsnew("user_defined");
        script_thread_arg->script_fullpath = sdsempty();
        script_thread_arg->script_content = sdsnew(script);
    }
    if (pthread_create(&mympd_script_thread, &attr, mympd_api_script_execute, script_thread_arg) != 0) {
        LOG_ERROR("Can not create mympd_script thread");
        free_t_script_thread_arg(script_thread_arg);
        return false;
    }
    pthread_setname_np(mympd_script_thread, "mympd_script");
    expire_result_queue(mympd_script_queue, 120);
    return true;
}

//private functions
static void *mympd_api_script_execute(void *script_thread_arg) {
    thread_logname = sdsreplace(thread_logname, "script");
    struct t_script_thread_arg *script_arg = (struct t_script_thread_arg *) script_thread_arg;
    
    const char *script_return_text = NULL;
    lua_State *lua_vm = luaL_newstate();
    if (lua_vm == NULL) {
        LOG_ERROR("Memory allocation error in luaL_newstate");
        sds buffer = jsonrpc_start_phrase_notify(sdsempty(), "Error executing script %{script}: Memory allocation error", false);
        buffer = tojson_char(buffer, "script", script_arg->script_name, false);
        buffer = jsonrpc_end_phrase(buffer);
        ws_notify(buffer);
        sdsfree(buffer);
        sdsfree(thread_logname);
        free_t_script_thread_arg(script_arg);
        return NULL;
    }
    if (strcmp(script_arg->config->lualibs, "all") == 0) {
        LOG_DEBUG("Open all standard lua libs");
        luaL_openlibs(lua_vm);

        mympd_luaopen(lua_vm, "json");
        lua_pop(lua_vm, 1);
        
        mympd_luaopen(lua_vm, "mympd");
        lua_pop(lua_vm, 1);
    }
    else {
        int count;
        sds *tokens = sdssplitlen(script_arg->config->lualibs, sdslen(script_arg->config->lualibs), ",", 1, &count);
        for (int i = 0; i < count; i++) {
            sdstrim(tokens[i], " ");
            LOG_DEBUG("Open lua library %s", tokens[i]);
            if (strcmp(tokens[i], "base") == 0)           { luaopen_base(lua_vm); }
            else if (strcmp(tokens[i], "package") == 0)   { luaopen_package(lua_vm); }
            else if (strcmp(tokens[i], "coroutine") == 0) { luaopen_coroutine(lua_vm); }
            else if (strcmp(tokens[i], "string") == 0)    { luaopen_string(lua_vm); }
            else if (strcmp(tokens[i], "utf8") == 0)      { luaopen_utf8(lua_vm); }
            else if (strcmp(tokens[i], "table") == 0)     { luaopen_table(lua_vm); }
            else if (strcmp(tokens[i], "math") == 0)      { luaopen_math(lua_vm); }
            else if (strcmp(tokens[i], "io") == 0)        { luaopen_io(lua_vm); }
            else if (strcmp(tokens[i], "os") == 0)        { luaopen_os(lua_vm); }
            else if (strcmp(tokens[i], "debug") == 0)     { luaopen_package(lua_vm); }
            else if (strcmp(tokens[i], "bit32") == 0)     { luaopen_bit32(lua_vm); }
            //custom libs
            else if (strcmp(tokens[i], "json") == 0 ||
                     strcmp(tokens[i], "mympd") == 0)     { mympd_luaopen(lua_vm, tokens[i]);
            }
            else {
                LOG_ERROR("Can not open lua library %s", tokens[i]);
                continue;
            }
            lua_pop(lua_vm, 1);
        }
        sdsfreesplitres(tokens,count);
    }
    register_lua_functions(lua_vm);
    int rc;
    if (script_arg->localscript == true) {
        rc = luaL_loadfilex(lua_vm, script_arg->script_fullpath, "t");
    }
    else {
        rc = luaL_loadstring(lua_vm, script_arg->script_content);
    }
    if (rc == 0) {
        if (script_arg->arguments->length > 0) {
            lua_newtable(lua_vm);
            struct list_node *current = script_arg->arguments->head;
            while (current != NULL) {
                populate_lua_table_field_p(lua_vm, current->key, current->value_p);
                current = current->next;
            }
            lua_setglobal(lua_vm, "arguments");
        }
        LOG_DEBUG("Start script");
        rc = lua_pcall(lua_vm, 0, 1, 0);
        LOG_DEBUG("End script");
    }
    if (lua_gettop(lua_vm) == 1) {
        //return value on stack
        script_return_text = lua_tostring(lua_vm, 1);
        LOG_DEBUG("Script return value: %s", script_return_text);
    }
    if (rc == 0) {
        if (script_return_text == NULL) {
            sds buffer = jsonrpc_start_phrase_notify(sdsempty(), "Script %{script} executed successfully", false);
            buffer = tojson_char(buffer, "script", script_arg->script_name, false);
            buffer = jsonrpc_end_phrase(buffer);
            ws_notify(buffer);
            sdsfree(buffer);
        }
        else {
            send_jsonrpc_notify_info(script_return_text);
        }
    }
    else {
        sds err_str = lua_err_to_str(sdsempty(), rc, true, script_arg->script_name);
        if (script_return_text != NULL) {
            err_str = sdscatfmt(err_str, ": %s", script_return_text);
        }
        sds buffer = jsonrpc_start_phrase_notify(sdsempty(), err_str, true);
        buffer = tojson_char(buffer, "script", script_arg->script_name, false);
        buffer = jsonrpc_end_phrase(buffer);
        ws_notify(buffer);
        sdsfree(buffer);
        err_str = sdscrop(err_str);
        err_str = lua_err_to_str(err_str, rc, false, script_arg->script_name);
        if (script_return_text != NULL) {
            err_str = sdscatfmt(err_str, ": %s", script_return_text);
        }
        LOG_ERROR(err_str);
        sdsfree(err_str);
    }
    lua_close(lua_vm);
    sdsfree(thread_logname);
    free_t_script_thread_arg(script_arg);
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

static bool mympd_luaopen(lua_State *lua_vm, const char *lualib) {
    LOG_DEBUG("Loading embedded lua library %s", lualib);
    #ifdef DEBUG
        sds filename = sdscatfmt(sdsempty(), "%s/%s.lua", LUALIBS_PATH, lualib);
        int rc = luaL_dofile(lua_vm, filename);
        sdsfree(filename);
    #else
        sds lib_string;
        if (strcmp(lualib, "json") == 0) {
            lib_string = sdscatlen(sdsempty(), json_lua_data, json_lua_size);
        }
        if (strcmp(lualib, "mympd") == 0) {
            lib_string = sdscatlen(sdsempty(), mympd_lua_data, mympd_lua_size);
        }
        else {
            return false;
        }
        int rc = luaL_dostring(lua_vm, lib_string);
        sdsfree(lib_string);
    #endif
    if (rc != 0) {
        if (lua_gettop(lua_vm) == 1) {
            //return value on stack
            LOG_ERROR("Error loading library %s: %s", lualib, lua_tostring(lua_vm, 1));
        }
    }
    return rc;
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

    //pid_t tid = gettid();
    pthread_t tid = pthread_self();
    
    t_work_request *request = create_request(-2, tid, method_id, method, "");
    request->data = sdscatprintf(request->data, "{\"jsonrpc\":\"2.0\",\"id\":%ld,\"method\":\"%s\",\"params\":{", tid, method);
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
                    lua_setglobal(lua_vm, "mympd_state");
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

static void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg) {
    sdsfree(script_thread_arg->script_name);
    sdsfree(script_thread_arg->script_fullpath);
    sdsfree(script_thread_arg->script_content);
    list_free(script_thread_arg->arguments);
    free(script_thread_arg->arguments);
    free(script_thread_arg);
}
#endif
