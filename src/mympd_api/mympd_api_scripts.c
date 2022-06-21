/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_scripts.h"

#include "../lib/api.h"
#include "../lib/http_client.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/lua_mympd_state.h"
#include "../lib/mem.h"
#include "../lib/mimetype.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <unistd.h>

#ifdef ENABLE_LUA

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#ifdef EMBEDDED_ASSETS
    //embedded files for release build
    #include "mympd_api_scripts_lualibs.c"
#endif

//private definitions
struct t_script_thread_arg {
    struct t_config *config;
    bool localscript;
    sds script_fullpath;
    sds script_name;
    sds script_content;
    struct t_list *arguments;
};

static void *mympd_api_script_execute(void *script_thread_arg);
static sds lua_err_to_str(sds buffer, int rc, bool phrase, const char *script);
static void populate_lua_table(lua_State *lua_vm, struct t_list *lua_mympd_state);
static void populate_lua_table_field_p(lua_State *lua_vm, const char *key, const char *value);
static void populate_lua_table_field_i(lua_State *lua_vm, const char *key, long long value);
static void populate_lua_table_field_f(lua_State *lua_vm, const char *key, double value);
static void populate_lua_table_field_b(lua_State *lua_vm, const char *key, bool value);
static void register_lua_functions(lua_State *lua_vm);
static int mympd_api(lua_State *lua_vm);
static int mympd_api_raw(lua_State *lua_vm);
static int _mympd_api(lua_State *lua_vm, bool raw);
static void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg);
static bool mympd_luaopen(lua_State *lua_vm, const char *lualib);
static sds parse_script_metadata(sds entry, const char *scriptfilename, int *order);
static int _mympd_api_http_client(lua_State *lua_vm);

//public functions
sds mympd_api_script_list(struct t_config *config, sds buffer, sds method, long request_id, bool all) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    sds scriptdirname = sdscatfmt(sdsempty(), "%s/scripts", config->workdir);
    errno = 0;
    DIR *script_dir = opendir(scriptdirname);
    if (script_dir == NULL) {
        MYMPD_LOG_ERROR("Can not open directory \"%s\"", scriptdirname);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(scriptdirname);
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "script", "error", "Can not open script directory");
        return buffer;
    }

    struct dirent *next_file;
    int nr = 0;
    sds entry = sdsempty();
    sds scriptname = sdsempty();
    sds scriptfilename = sdsempty();
    while ((next_file = readdir(script_dir)) != NULL ) {
        const char *ext = get_extension_from_filename(next_file->d_name);
        if (ext == NULL ||
            strcasecmp(ext, "lua") != 0)
        {
            continue;
        }

        scriptname = sdscat(scriptname, next_file->d_name);
        sds_strip_file_extension(scriptname);
        entry = sdscatlen(entry, "{", 1);
        entry = tojson_char(entry, "name", scriptname, true);
        scriptfilename = sdscatfmt(scriptfilename, "%s/%s", scriptdirname, next_file->d_name);
        int order = 0;
        entry = parse_script_metadata(entry, scriptfilename, &order);
        entry = sdscatlen(entry, "}", 1);
        if (all == true || order > 0) {
            if (nr++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscat(buffer, entry);
        }
        sdsclear(entry);
        sdsclear(scriptname);
        sdsclear(scriptfilename);
    }
    closedir(script_dir);
    FREE_SDS(scriptname);
    FREE_SDS(scriptfilename);
    FREE_SDS(entry);
    FREE_SDS(scriptdirname);
    buffer = sdscatlen(buffer, "]", 1);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

bool mympd_api_script_delete(struct t_config *config, const char *script) {
    sds scriptfilename = sdscatfmt(sdsempty(), "%s/scripts/%s.lua", config->workdir, script);
    errno = 0;
    if (unlink(scriptfilename) == -1) {
        MYMPD_LOG_ERROR("Unlinking script file \"%s\" failed", scriptfilename);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(scriptfilename);
        return false;
    }
    FREE_SDS(scriptfilename);
    return true;
}

bool mympd_api_script_save(struct t_config *config, sds script, sds oldscript, int order, sds content, struct t_list *arguments) {
    sds tmp_file = sdscatfmt(sdsempty(), "%s/scripts/%s.XXXXXX", config->workdir, script);
    errno = 0;
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", tmp_file);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(tmp_file);
        return false;
    }
    bool rc = true;
    FILE *fp = fdopen(fd, OPEN_FLAGS_WRITE);
    sds argstr = sdsempty();
    argstr = list_to_json_array(argstr, arguments);
    if (fprintf(fp, "-- {\"order\":%d,\"arguments\":[%s]}\n%s", order, argstr, content) < 0) {
        MYMPD_LOG_ERROR("Could not write to file \"%s\"", tmp_file);
        rc = false;
    }
    FREE_SDS(argstr);
    if (fclose(fp) != 0) {
        MYMPD_LOG_ERROR("Could not close file \"%s\"", tmp_file);
        rc = false;
    }
    sds script_filename = sdscatfmt(sdsempty(), "%s/scripts/%s.lua", config->workdir, script);
    errno = 0;
    if (rc == true) {
        if (rename(tmp_file, script_filename) == -1) {
            MYMPD_LOG_ERROR("Rename file from \"%s\" to \"%s\" failed", tmp_file, script_filename);
            MYMPD_LOG_ERRNO(errno);
            FREE_SDS(tmp_file);
            FREE_SDS(script_filename);
            return false;
        }
        if (sdslen(oldscript) > 0 && strcmp(script, oldscript) != 0) {
            sds oldscript_filename = sdscatfmt(sdsempty(), "%s/scripts/%s.lua", config->workdir, oldscript);
            errno = 0;
            if (unlink(oldscript_filename) == -1) {
                MYMPD_LOG_ERROR("Error removing file \"%s\"", oldscript_filename);
                MYMPD_LOG_ERRNO(errno);
                rc = false;
            }
            FREE_SDS(oldscript_filename);
        }
    }
    else {
        //remove incomplete tmp file
        if (unlink(tmp_file) != 0) {
            MYMPD_LOG_ERROR("Could not remove incomplete tmp file \"%s\"", tmp_file);
            MYMPD_LOG_ERRNO(errno);
            rc = false;
        }
    }
    FREE_SDS(tmp_file);
    FREE_SDS(script_filename);
    return rc;
}

sds mympd_api_script_get(struct t_config *config, sds buffer, sds method, long request_id, const char *script) {
    sds scriptfilename = sdscatfmt(sdsempty(), "%s/scripts/%s.lua", config->workdir, script);
    errno = 0;
    FILE *fp = fopen(scriptfilename, OPEN_FLAGS_READ);
    if (fp != NULL) {
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = tojson_char(buffer, "script", script, true);
        sds line = sdsempty();
        if (sds_getline(&line, fp, 1000) == 0 && strncmp(line, "-- ", 3) == 0) {
            sdsrange(line, 3, -1);
            if (line[0] == '{' && line[sdslen(line) - 1] == '}') {
                buffer = sdscat(buffer, "\"metadata\":");
                buffer = sdscat(buffer, line);
            }
            else {
                MYMPD_LOG_WARN("Invalid metadata for script %s", scriptfilename);
                buffer = sdscat(buffer, "\"metadata\":{\"order\":0, \"arguments\":[]}");
            }
        }
        else {
            MYMPD_LOG_WARN("Invalid metadata for script %s", scriptfilename);
            buffer = sdscat(buffer, "\"metadata\":{\"order\":0, \"arguments\":[]}");
        }
        FREE_SDS(line);
        buffer = sdscat(buffer, ",\"content\":");
        sds content = sdsempty();
        sds_getfile(&content, fp, 10000);
        (void) fclose(fp);
        buffer = sds_catjson(buffer, content, sdslen(content));
        FREE_SDS(content);
        buffer = jsonrpc_result_end(buffer);
    }
    else {
        MYMPD_LOG_ERROR("Can not open file \"%s\"", scriptfilename);
        MYMPD_LOG_ERRNO(errno);
        buffer = jsonrpc_respond_message(buffer, method, request_id, true,
            "script", "error", "Can not open scriptfile");
    }
    FREE_SDS(scriptfilename);

    return buffer;
}

bool mympd_api_script_start(struct t_config *config, const char *script, struct t_list *arguments, bool localscript) {
    pthread_t mympd_script_thread;
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        MYMPD_LOG_ERROR("Can not init mympd_script thread attribute");
        return false;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
        MYMPD_LOG_ERROR("Can not set mympd_script thread to detached");
        return false;
    }
    struct t_script_thread_arg *script_thread_arg = malloc_assert(sizeof(struct t_script_thread_arg));
    script_thread_arg->config = config;
    script_thread_arg->localscript = localscript;
    script_thread_arg->arguments = arguments;
    if (localscript == true) {
        script_thread_arg->script_name = sdsnew(script);
        script_thread_arg->script_fullpath = sdscatfmt(sdsempty(), "%s/scripts/%s.lua", config->workdir, script);
        script_thread_arg->script_content = sdsempty();
    }
    else {
        script_thread_arg->script_name = sdsnew("user_defined");
        script_thread_arg->script_fullpath = sdsempty();
        script_thread_arg->script_content = sdsnew(script);
    }
    if (pthread_create(&mympd_script_thread, &attr, mympd_api_script_execute, script_thread_arg) != 0) {
        MYMPD_LOG_ERROR("Can not create mympd_script thread");
        free_t_script_thread_arg(script_thread_arg);
        return false;
    }
    expire_result_queue(mympd_script_queue, 120);
    return true;
}

//private functions
static sds parse_script_metadata(sds entry, const char *scriptfilename, int *order) {
    errno = 0;
    FILE *fp = fopen(scriptfilename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Can not open file \"%s\"", scriptfilename);
        MYMPD_LOG_ERRNO(errno);
        return entry;
    }

    sds line = sdsempty();
    if (sds_getline(&line, fp, 1000) == 0 && strncmp(line, "-- ", 3) == 0) {
        sdsrange(line, 3, -1);
        if (json_get_int(line, "$.order", 0, 99, order, NULL) == true) {
            entry = sdscat(entry, "\"metadata\":");
            entry = sdscatsds(entry, line);
        }
        else {
            MYMPD_LOG_WARN("Invalid metadata for script %s", scriptfilename);
            entry = sdscat(entry, "\"metadata\":{\"order\":0,\"arguments\":[]}");
        }
    }
    else {
        MYMPD_LOG_WARN("Invalid metadata for script %s", scriptfilename);
        entry = sdscat(entry, "\"metadata\":{\"order\":0,\"arguments\":[]}");
    }
    FREE_SDS(line);
    (void) fclose(fp);
    return entry;
}

static void *mympd_api_script_execute(void *script_thread_arg) {
    thread_logname = sds_replace(thread_logname, "script");
    prctl(PR_SET_NAME, thread_logname, 0, 0, 0);
    struct t_script_thread_arg *script_arg = (struct t_script_thread_arg *) script_thread_arg;

    const char *script_return_text = NULL;
    lua_State *lua_vm = luaL_newstate();
    if (lua_vm == NULL) {
        MYMPD_LOG_ERROR("Memory allocation error in luaL_newstate");
        sds buffer = jsonrpc_notify_phrase(sdsempty(), "script", "error",
            "Error executing script %{script}: Memory allocation error", 2, "script", script_arg->script_name);
        ws_notify(buffer);
        FREE_SDS(buffer);
        FREE_SDS(thread_logname);
        free_t_script_thread_arg(script_arg);
        return NULL;
    }
    if (strcmp(script_arg->config->lualibs, "all") == 0) {
        MYMPD_LOG_DEBUG("Open all standard lua libs");
        luaL_openlibs(lua_vm);
        mympd_luaopen(lua_vm, "json");
        mympd_luaopen(lua_vm, "mympd");
    }
    else {
        int count = 0;
        sds *tokens = sdssplitlen(script_arg->config->lualibs, (ssize_t)sdslen(script_arg->config->lualibs), ",", 1, &count);
        for (int i = 0; i < count; i++) {
            sdstrim(tokens[i], " ");
            MYMPD_LOG_DEBUG("Open lua library %s", tokens[i]);
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
            else if (strcmp(tokens[i], "json") == 0 ||
                     strcmp(tokens[i], "mympd") == 0)	  { mympd_luaopen(lua_vm, tokens[i]); }
            else {
                MYMPD_LOG_ERROR("Can not open lua library %s", tokens[i]);
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
            struct t_list_node *current = script_arg->arguments->head;
            while (current != NULL) {
                populate_lua_table_field_p(lua_vm, current->key, current->value_p);
                current = current->next;
            }
            lua_setglobal(lua_vm, "arguments");
        }
        MYMPD_LOG_DEBUG("Start script");
        rc = lua_pcall(lua_vm, 0, 1, 0);
        MYMPD_LOG_DEBUG("End script");
    }
    //it should be only one value on the stack
    int nr_return = lua_gettop(lua_vm);
    MYMPD_LOG_DEBUG("Lua script returns %d values", nr_return);
    for (int i = 1; i <= nr_return; i++) {
        MYMPD_LOG_DEBUG("Lua script return value %d: %s", i, lua_tostring(lua_vm, i));
    }
    if (lua_gettop(lua_vm) == 1) {
        //return value on stack
        script_return_text = lua_tostring(lua_vm, 1);
    }
    if (rc == 0) {
        if (script_return_text == NULL) {
            sds buffer = jsonrpc_notify_phrase(sdsempty(), "script", "info", "Script %{script} executed successfully",
                2, "script", script_arg->script_name);
            ws_notify(buffer);
            FREE_SDS(buffer);
        }
        else {
            send_jsonrpc_notify("script", "info", script_return_text);
        }
    }
    else {
        //return message
        sds err_str = lua_err_to_str(sdsempty(), rc, true, script_arg->script_name);
        if (script_return_text != NULL) {
            err_str = sdscatfmt(err_str, ": %s", script_return_text);
        }
        sds buffer = jsonrpc_notify_phrase(sdsempty(), "script", "error", err_str, 2, "script", script_arg->script_name);
        ws_notify(buffer);
        FREE_SDS(buffer);
        //Error log message
        sdsclear(err_str);
        err_str = lua_err_to_str(err_str, rc, false, script_arg->script_name);
        if (script_return_text != NULL) {
            err_str = sdscatfmt(err_str, ": %s", script_return_text);
        }
        MYMPD_LOG_ERROR("%s", err_str);
        FREE_SDS(err_str);
    }
    lua_close(lua_vm);
    FREE_SDS(thread_logname);
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
        #if LUA_VERSION_NUM >= 503 && LUA_VERSION_NUM < 504
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
    MYMPD_LOG_DEBUG("Loading embedded lua library %s", lualib);
    #ifdef EMBEDDED_ASSETS
        sds lib_string;
        if (strcmp(lualib, "json") == 0) {
            lib_string = sdscatlen(sdsempty(), json_lua_data, json_lua_size);
        }
        else if (strcmp(lualib, "mympd") == 0) {
            lib_string = sdscatlen(sdsempty(), mympd_lua_data, mympd_lua_size);
        }
        else {
            return false;
        }
        int rc = luaL_dostring(lua_vm, lib_string);
        FREE_SDS(lib_string);
    #else
        sds filename = sdscatfmt(sdsempty(), "%s/%s.lua", LUALIBS_PATH, lualib);
        int rc = luaL_dofile(lua_vm, filename);
        FREE_SDS(filename);
    #endif
    int nr_return = lua_gettop(lua_vm);
    MYMPD_LOG_DEBUG("Lua library returns %d values", nr_return);
    for (int i = 1; i <= nr_return; i++) {
        MYMPD_LOG_DEBUG("Lua library return value \"%d\": \"%s\"", i, lua_tostring(lua_vm, i));
        lua_pop(lua_vm, i);
    }
    if (rc != 0) {
        if (lua_gettop(lua_vm) == 1) {
            //return value on stack
            MYMPD_LOG_ERROR("Error loading library \"%s\": \"%s\"", lualib, lua_tostring(lua_vm, 1));
        }
    }
    return rc;
}

static void populate_lua_table_field_p(lua_State *lua_vm, const char *key, const char *value) {
    lua_pushstring(lua_vm, key);
    lua_pushstring(lua_vm, value);
    lua_settable(lua_vm, -3);
}

static void populate_lua_table_field_i(lua_State *lua_vm, const char *key, long long value) {
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

static void populate_lua_table(lua_State *lua_vm, struct t_list *lua_mympd_state) {
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

static void register_lua_functions(lua_State *lua_vm) {
    lua_register(lua_vm, "mympd_api", mympd_api);
    lua_register(lua_vm, "mympd_api_raw", mympd_api_raw);
    lua_register(lua_vm, "mympd_api_http_client", _mympd_api_http_client);
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
        MYMPD_LOG_ERROR("Lua - mympd_api: invalid number of arguments");
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *method = lua_tostring(lua_vm, 1);
    enum mympd_cmd_ids method_id = get_cmd_id(method);
    if (method_id == GENERAL_API_UNKNOWN) {
        MYMPD_LOG_ERROR("Lua - mympd_api: Invalid method \"%s\"", method);
        return luaL_error(lua_vm, "Invalid method");
    }

    long tid = syscall(__NR_gettid);

    struct t_work_request *request = create_request(-2, tid, method_id, NULL);
    if (raw == false) {
        for (int i = 2; i < n; i = i + 2) {
            bool comma = i + 1 < n ? true : false;
            if (lua_isboolean(lua_vm, i + 1)) {
                request->data = tojson_bool(request->data, lua_tostring(lua_vm, i), lua_toboolean(lua_vm, i + 1), comma);
            }
            else if (lua_isinteger(lua_vm, i + 1)) {
                request->data = tojson_llong(request->data, lua_tostring(lua_vm, i), lua_tointeger(lua_vm, i + 1), comma);
            }
            else if (lua_isnumber(lua_vm, i + 1)) {
                request->data = tojson_double(request->data, lua_tostring(lua_vm, i), lua_tonumber(lua_vm, i + 1), comma);
            }
            else {
                request->data = tojson_char(request->data, lua_tostring(lua_vm, i), lua_tostring(lua_vm, i + 1), comma);
            }
        }
        request->data = sdscatlen(request->data, "}", 1);
    }
    else if (n == 2) {
        sdsrange(request->data, 0, -2); //trim {
        request->data = sdscat(request->data, lua_tostring(lua_vm, 2));
    }
    request->data = sdscatlen(request->data, "}", 1);

    mympd_queue_push(mympd_api_queue, request, tid);

    int i = 0;
    while (s_signal_received == 0 && i < 60) {
        i++;
        struct t_work_result *response = mympd_queue_shift(mympd_script_queue, 1000000, tid);
        if (response != NULL) {
            MYMPD_LOG_DEBUG("Got result: %s", response->data);
            if (response->cmd_id == INTERNAL_API_SCRIPT_INIT) {
                MYMPD_LOG_DEBUG("Populating lua global state table mympd");
                struct t_list *lua_mympd_state = (struct t_list *)response->extra;
                lua_newtable(lua_vm);
                populate_lua_table(lua_vm, lua_mympd_state);
                lua_setglobal(lua_vm, "mympd_state");
                lua_mympd_state_free(lua_mympd_state);
                lua_mympd_state = NULL;
                lua_pushinteger(lua_vm, 0);
                lua_pushstring(lua_vm, "mympd_state is now populated");
            }
            else {
                //return code
                if (json_find_key(response->data, "$.error.message") == true) {
                    lua_pushinteger(lua_vm, 1);
                }
                else {
                    lua_pushinteger(lua_vm, 0);
                }
                //result
                lua_pushlstring(lua_vm, response->data, sdslen(response->data));
            }
            free_result(response);
            return 2;
        }
    }
    return luaL_error(lua_vm, "No API response, timeout after 60s");
}

static void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg) {
    FREE_SDS(script_thread_arg->script_name);
    FREE_SDS(script_thread_arg->script_fullpath);
    FREE_SDS(script_thread_arg->script_content);
    list_clear(script_thread_arg->arguments);
    FREE_PTR(script_thread_arg->arguments);
    FREE_PTR(script_thread_arg);
}

static int _mympd_api_http_client(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 4) {
        MYMPD_LOG_ERROR("Lua - mympd_api_http_client: invalid number of arguments");
        return luaL_error(lua_vm, "Invalid number of arguments");
    }

    struct mg_client_request_t mg_client_request = {
        .method = lua_tostring(lua_vm, 1),
        .uri = lua_tostring(lua_vm, 2),
        .extra_headers = lua_tostring(lua_vm, 3),
        .post_data = lua_tostring(lua_vm, 4)
    };

    struct mg_client_response_t mg_client_response = {
        .rc = -1,
        .response = sdsempty(),
        .header = sdsempty(),
        .body = sdsempty()
    };

    http_client_request(&mg_client_request, &mg_client_response);

    lua_pushinteger(lua_vm, mg_client_response.rc);
    lua_pushlstring(lua_vm, mg_client_response.response, sdslen(mg_client_response.response));
    lua_pushlstring(lua_vm, mg_client_response.header, sdslen(mg_client_response.header));
    lua_pushlstring(lua_vm, mg_client_response.body, sdslen(mg_client_response.body));
    FREE_SDS(mg_client_response.response);
    FREE_SDS(mg_client_response.header);
    FREE_SDS(mg_client_response.body);
    return 4;
}

#endif
