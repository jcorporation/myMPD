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
#include <inttypes.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>

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
static void populate_lua_table(lua_State *lua_vm, struct list *lua_mympd_state);
static void populate_lua_table_field_p(lua_State *lua_vm, const char *key, const char *value);
static void populate_lua_table_field_i(lua_State *lua_vm, const char *key, long value);
static void populate_lua_table_field_f(lua_State *lua_vm, const char *key, double value);
static void populate_lua_table_field_b(lua_State *lua_vm, const char *key, bool value);
static void register_lua_functions(lua_State *lua_vm);
static int mympd_api(lua_State *lua_vm);
static int mympd_api_raw(lua_State *lua_vm);
static int _mympd_api(lua_State *lua_vm, bool raw);
static void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg);
static bool mympd_luaopen(lua_State *lua_vm, const char *lualib);

//public functions
sds mympd_api_script_list(t_config *config, sds buffer, sds method, long request_id, bool all) {
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    sds scriptdirname = sdscatfmt(sdsempty(), "%s/scripts", config->varlibdir);
    DIR *script_dir = opendir(scriptdirname);
    if (script_dir != NULL) {
        struct dirent *next_file;
        int nr = 0;
        sds entry = sdsempty();
        while ((next_file = readdir(script_dir)) != NULL ) {
            sds extension = get_extension_from_filename(next_file->d_name);
            if (strcmp(extension, "lua") == 0) {
                strip_extension(next_file->d_name);
                entry = sdscatlen(entry, "{", 1);
                entry = tojson_char(entry, "name", next_file->d_name, true);
                sds scriptfilename = sdscatfmt(sdsempty(), "%s/%s.lua", scriptdirname, next_file->d_name);
                FILE *fp = fopen(scriptfilename, "r");
                int order = 0;
                if (fp != NULL) {
                    char *line = NULL;
                    size_t n = 0;
                    if (getline(&line, &n, fp) > 0) {
                        if (strncmp(line, "-- ", 3) == 0) {
                            sds metadata = sdsnew(line);
                            sdsrange(metadata, 3, -2);
                            int je = json_scanf(metadata, sdslen(metadata), "{order: %d}", &order);
                            if (je == 0) {
                                LOG_WARN("Invalid metadata for script %s", scriptfilename);
                                entry = sdscat(entry, "\"metadata\":{\"order\":0,\"arguments\":[]}");
                            }
                            else {
                                entry = sdscat(entry, "\"metadata\":");
                                entry = sdscat(entry, metadata);
                            }
                            sdsfree(metadata);
                        }
                        else {
                            LOG_WARN("Invalid metadata for script %s", scriptfilename);
                            entry = sdscat(entry, "\"metadata\":{\"order\":0,\"arguments\":[]}");
                        }
                    }
                    else {
                        LOG_WARN("Invalid metadata for script %s", scriptfilename);
                        entry = sdscat(entry, "\"metadata\":{\"order\":0,\"arguments\":[]}");
                    }
                    FREE_PTR(line);
                    fclose(fp);
                }
                else {
                    LOG_ERROR("Can not open file %s", scriptfilename);
                }
                entry = sdscatlen(entry, "}", 1);
                if (all == true || order > 0) {
                    if (nr++) {
                        buffer = sdscat(buffer, ",");
                    }
                    buffer = sdscat(buffer, entry);
                }
                entry = sdscrop(entry);
                sdsfree(scriptfilename);
            }
            sdsfree(extension);
        }
        closedir(script_dir);
        sdsfree(entry);
    }
    else {
        LOG_ERROR("Can not open dir %s", scriptdirname);
    }
    sdsfree(scriptdirname);
    buffer = sdscat(buffer, "]");        
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

bool mympd_api_script_delete(t_config *config, const char *script) {
    sds scriptfilename = sdscatfmt(sdsempty(), "%s/scripts/%s.lua", config->varlibdir, script);
    if (unlink(scriptfilename) == -1) {
        LOG_ERROR("Unlinking script file %s failed: %s", scriptfilename, strerror(errno));
        sdsfree(scriptfilename);
        return false;
    }
    sdsfree(scriptfilename);
    return true;
}

bool mympd_api_script_save(t_config *config, const char *script, int order, const char *content, const char *arguments) {
    sds tmp_file = sdscatfmt(sdsempty(), "%s/scripts/%.XXXXXX", config->varlibdir, script);
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    //write metadata line
    fprintf(fp, "-- {\"order\":%d,\"arguments\":[%s]}\n", order, arguments);
    //write script content
    fputs(content, fp);
    fclose(fp);
    sds scriptfilename = sdscatfmt(sdsempty(), "%s/scripts/%s.lua", config->varlibdir, script);
    if (rename(tmp_file, scriptfilename) == -1) {
        LOG_ERROR("Rename file from %s to %s failed", tmp_file, scriptfilename);
        sdsfree(tmp_file);
        sdsfree(scriptfilename);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(scriptfilename);
    return true;
}

sds mympd_api_script_get(t_config *config, sds buffer, sds method, long request_id, const char *script) {
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",");
    buffer = tojson_char(buffer, "script", script, true);
 
    sds scriptfilename = sdscatfmt(sdsempty(), "%s/scripts/%s.lua", config->varlibdir, script);
    FILE *fp = fopen(scriptfilename, "r");
    if (fp != NULL) {
        char *line = NULL;
        size_t n = 0;
        ssize_t read = 0;
        if (getline(&line, &n, fp) > 0) {
            if (strncmp(line, "-- ", 3) == 0) {
                sds metadata = sdsnew(line);
                sdsrange(metadata, 3, -2);
                if (metadata[0] == '{' && metadata[sdslen(metadata) - 1] == '}') {
                    buffer = sdscat(buffer, "\"metadata\":");
                    buffer = sdscat(buffer, metadata);
                }
                else {
                    LOG_WARN("Invalid metadata for script %s", scriptfilename);
                    buffer = sdscat(buffer, "\"metadata\":{\"order\":0, \"arguments\":[]}");
                }
                sdsfree(metadata);
            }
            else {
                LOG_WARN("Invalid metadata for script %s", scriptfilename);
                buffer = sdscat(buffer, "\"metadata\":{\"order\":0, \"arguments\":[]}");
            }
        }
        else {
            LOG_WARN("Invalid metadata for script %s", scriptfilename);
            buffer = sdscat(buffer, "\"metadata\":{\"order\":0, \"arguments\":[]}");
        }
        buffer = sdscat(buffer, ",\"content\":");
        sds content = sdsempty();
        while ((read = getline(&line, &n, fp)) > 0) {
            content = sdscatlen(content, line, read);
        }
        fclose(fp);
        FREE_PTR(line);
        buffer = sdscatjson(buffer, content, sdslen(content));
        sdsfree(content);
    }
    else {
        LOG_ERROR("Can not open file %s", scriptfilename);
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Can not open scriptfile", true);
    }
    sdsfree(scriptfilename);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

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

bool mympd_api_get_lua_mympd_state(t_mympd_state *mympd_state, struct list *lua_mympd_state) {
    set_lua_mympd_state_p(lua_mympd_state, "jukebox_unique_tag", mympd_state->jukebox_unique_tag);
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
        mympd_luaopen(lua_vm, "mympd");
        lua_pop(lua_vm, 1); //pop is only necessary for mympd lib, why?
    }
    else {
        int count;
        sds *tokens = sdssplitlen(script_arg->config->lualibs, sdslen(script_arg->config->lualibs), ",", 1, &count);
        for (int i = 0; i < count; i++) {
            sdstrim(tokens[i], " ");
            LOG_DEBUG("Open lua library %s", tokens[i]);
            if (strcmp(tokens[i], "base") == 0)           { luaL_requiref(lua_vm, "base", luaopen_base, 1); lua_pop(lua_vm, 1); }
            else if (strcmp(tokens[i], "package") == 0)   { luaL_requiref(lua_vm, "package", luaopen_package, 1); lua_pop(lua_vm, 1); }
            else if (strcmp(tokens[i], "coroutine") == 0) { luaL_requiref(lua_vm, "coroutine", luaopen_coroutine, 1); lua_pop(lua_vm, 1); }
            else if (strcmp(tokens[i], "string") == 0)    { luaL_requiref(lua_vm, "string", luaopen_string, 1); lua_pop(lua_vm, 1); }
            else if (strcmp(tokens[i], "utf8") == 0)      { luaL_requiref(lua_vm, "utf8", luaopen_utf8, 1); lua_pop(lua_vm, 1); }
            else if (strcmp(tokens[i], "table") == 0)     { luaL_requiref(lua_vm, "table", luaopen_table, 1); lua_pop(lua_vm, 1); }
            else if (strcmp(tokens[i], "math") == 0)      { luaL_requiref(lua_vm, "math", luaopen_math, 1); lua_pop(lua_vm, 1); }
            else if (strcmp(tokens[i], "io") == 0)        { luaL_requiref(lua_vm, "io", luaopen_io, 1); lua_pop(lua_vm, 1); }
            else if (strcmp(tokens[i], "os") == 0)        { luaL_requiref(lua_vm, "os", luaopen_os, 1); lua_pop(lua_vm, 1); }
            else if (strcmp(tokens[i], "debug") == 0)     { luaL_requiref(lua_vm, "debug", luaopen_debug, 1); lua_pop(lua_vm, 1); }
            //custom libs
            else if (strcmp(tokens[i], "json") == 0)	  { mympd_luaopen(lua_vm, tokens[i]); }
            else if (strcmp(tokens[i], "mympd") == 0) 	  { mympd_luaopen(lua_vm, tokens[i]); lua_pop(lua_vm, 1); /*pop is only necessary for mympd lib, why?*/ }
            else {
                LOG_ERROR("Can not open lua library %s", tokens[i]);
                continue;
            }
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
    int nr_return = lua_gettop(lua_vm);
    LOG_DEBUG("Lua script returns %d values", nr_return);
    for (int i = 1; i <= nr_return; i++) {
        LOG_DEBUG("Lua script return value %d: %s", i, lua_tostring(lua_vm, i));
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
        #if LUA_VERSION_5_3
        case LUA_ERRGCMM:
            buffer = sdscatfmt(buffer, "Error executing script %s: Error in garbage collector", (phrase == true ? "%{script}" : script));
            break;
        #endif
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

static void populate_lua_table_field_f(lua_State *lua_vm, const char *key, double value) {
    lua_pushstring(lua_vm, key);
    lua_pushnumber(lua_vm, value);
    lua_settable(lua_vm, -3);
}

static void populate_lua_table_field_b(lua_State *lua_vm, const char *key, bool value) {
    lua_pushstring(lua_vm, key);
    lua_pushboolean(lua_vm, value);
    lua_settable(lua_vm, -3);
}

static void populate_lua_table(lua_State *lua_vm, struct list *lua_mympd_state) {
    struct list_node *current = lua_mympd_state->head;
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
    //pthread_t tid = pthread_self();
    pid_t tid = syscall(__NR_gettid);
    
    t_work_request *request = create_request(-2, tid, method_id, method, "");
    request->data = sdscatprintf(request->data, "{\"jsonrpc\":\"2.0\",\"id\":%d,\"method\":\"%s\",\"params\":{", tid, method);
    if (raw == false) {
        for (int i = 2; i < n; i = i + 2) {
            bool comma = i + 1 < n ? true : false;
            if (lua_isboolean(lua_vm, i + 1)) {
                request->data = tojson_bool(request->data, lua_tostring(lua_vm, i), lua_toboolean(lua_vm, i + 1), comma);
            }
            else if (lua_isinteger(lua_vm, i + 1)) {
                request->data = tojson_long(request->data, lua_tostring(lua_vm, i), lua_tointeger(lua_vm, i + 1), comma);
            }
            else if (lua_isnumber(lua_vm, i + 1)) {
                request->data = tojson_double(request->data, lua_tostring(lua_vm, i), lua_tonumber(lua_vm, i + 1), comma);
            }
            else {
                request->data = tojson_char(request->data, lua_tostring(lua_vm, i), lua_tostring(lua_vm, i + 1), comma);
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
                    struct list *lua_mympd_state = (struct list *) response->extra;
                    lua_newtable(lua_vm);
                    populate_lua_table(lua_vm, lua_mympd_state);    
                    lua_setglobal(lua_vm, "mympd_state");
                    free_lua_mympd_state(lua_mympd_state);
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
