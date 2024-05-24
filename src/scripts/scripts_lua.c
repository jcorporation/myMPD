/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/scripts/scripts_lua.h"

#include "src/lib/config_def.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

#include "src/scripts/interface.h"
#include "src/scripts/interface_http_client.h"
#ifdef MYMPD_ENABLE_MYGPIOD
    #include "src/scripts/interface_mygpio.h"
#endif
#include "src/scripts/interface_mympd_api.h"
#include "src/scripts/interface_util.h"

#include <string.h>

#ifdef MYMPD_EMBEDDED_ASSETS
    //embedded files for release build
    #include "src/scripts/lualibs.c"
#endif

// Private definitions

static void register_lua_functions(lua_State *lua_vm);
static int mympd_luaopen(lua_State *lua_vm, const char *lualib);

// Public functions

/**
 * Creates the lua instance and loads the script
 * @param script_arg pointer to t_script_thread_arg struct
 * @param rc loading script return value
 * @return lua instance or NULL on error
 */
lua_State *script_load(struct t_script_thread_arg *script_arg, int *rc) {
    lua_State *lua_vm = luaL_newstate();
    if (lua_vm == NULL) {
        MYMPD_LOG_ERROR(script_arg->partition, "Memory allocation error in luaL_newstate");
        return NULL;
    }
    if (strcmp(script_arg->config->lualibs, "all") == 0) {
        MYMPD_LOG_DEBUG(NULL, "Open all standard lua libs");
        luaL_openlibs(lua_vm);
        if (mympd_luaopen(lua_vm, "json") == 1 ||
            mympd_luaopen(lua_vm, "mympd") == 1)
        {
            lua_close(lua_vm);
            return NULL;
        }
    }
    else {
        int count = 0;
        sds *tokens = sdssplitlen(script_arg->config->lualibs, (ssize_t)sdslen(script_arg->config->lualibs), ",", 1, &count);
        for (int i = 0; i < count; i++) {
            sdstrim(tokens[i], " ");
            MYMPD_LOG_DEBUG(NULL, "Open lua library %s", tokens[i]);
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
                     strcmp(tokens[i], "mympd") == 0)
            { 
                if (mympd_luaopen(lua_vm, tokens[i]) == 1) {
                    lua_close(lua_vm);
                    return NULL;
                }
            }
            else {
                MYMPD_LOG_WARN(NULL, "Can not find lua library %s", tokens[i]);
                continue;
            }
        }
        sdsfreesplitres(tokens,count);
    }
    register_lua_functions(lua_vm);
    *rc = script_arg->localscript == true
        ? luaL_loadfilex(lua_vm, script_arg->script_fullpath, "t")
        : luaL_loadstring(lua_vm, script_arg->script_content);
    return lua_vm;
}

/**
 * Populate the global vars for script execution
 * @param lua_vm lua instance
 * @param script_arg pointer to t_script_thread_arg struct
 */
void populate_lua_global_vars(lua_State *lua_vm, struct t_script_thread_arg *script_arg) {
    // Set global mympd_env lua table
    lua_newtable(lua_vm);
    populate_lua_table_field_p(lua_vm, "partition", script_arg->partition);
    populate_lua_table_field_i(lua_vm, "requestid", script_arg->request_id);
    populate_lua_table_field_p(lua_vm, "scriptevent", script_start_event_name(script_arg->start_event));
    populate_lua_table_field_p(lua_vm, "scriptname", script_arg->script_name);
    populate_lua_table_field_p(lua_vm, "cachedir", script_arg->config->cachedir);
    sds covercache = sdscatfmt(sdsempty(), "%s/%s", script_arg->config->cachedir,  DIR_CACHE_COVER);
    populate_lua_table_field_p(lua_vm, "cachedir_cover", covercache);
    FREE_SDS(covercache);
    populate_lua_table_field_p(lua_vm, "workdir", script_arg->config->workdir);
    lua_setglobal(lua_vm, "mympd_env");

    // Set global arguments lua table
    lua_newtable(lua_vm);
    if (script_arg->arguments->length > 0) {
        struct t_list_node *current = script_arg->arguments->head;
        while (current != NULL) {
            populate_lua_table_field_p(lua_vm, current->key, current->value_p);
            current = current->next;
        }
    }
    lua_setglobal(lua_vm, "mympd_arguments");
}

/**
 * Returns a phrase for lua errors
 * @param rc return code of the lua script
 * @return error string literal
 */
const char *lua_err_to_str(int rc) {
    switch(rc) {
        case LUA_ERRSYNTAX:
            return "Syntax error during precompilation";
        case LUA_ERRMEM:
            return "Memory allocation error";
        #if LUA_VERSION_NUM >= 503 && LUA_VERSION_NUM < 504
        case LUA_ERRGCMM:
            return "Error in garbage collector";
        #endif
        case LUA_ERRFILE:
            return "Can not open or read script file";
        case LUA_ERRRUN:
            return "Runtime error";
        case LUA_ERRERR:
            return "Error while running the message handler";
            break;
        default:
            return "Unknown error";
    }
}

// Private functions

/**
 * Loads a lua library from filesystem or embedded in release
 * @param lua_vm lua instance
 * @param lualib lua library to load
 * @return true on success, else false
 */
static int mympd_luaopen(lua_State *lua_vm, const char *lualib) {
    MYMPD_LOG_DEBUG(NULL, "Loading embedded lua library %s", lualib);
    #ifdef MYMPD_EMBEDDED_ASSETS
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
        sds filename = sdscatfmt(sdsempty(), "%s/%s.lua", MYMPD_LUALIBS_PATH, lualib);
        int rc = luaL_dofile(lua_vm, filename);
        FREE_SDS(filename);
    #endif
    int nr_return = lua_gettop(lua_vm);
    MYMPD_LOG_DEBUG(NULL, "Lua library returns %d values", nr_return);
    for (int i = 1; i <= nr_return; i++) {
        if (lua_isnil(lua_vm, 1) == 0 &&
            lua_istable(lua_vm, 1) == 0)
        {
            MYMPD_LOG_ERROR(NULL, "Error loading library \"%s\": \"%s\"", lualib, lua_tostring(lua_vm, 1));
        }
        lua_pop(lua_vm, 1);
    }
    return rc;
}

/**
 * Registers myMPD specific lua functions
 * @param lua_vm lua instance
 */
static void register_lua_functions(lua_State *lua_vm) {
    lua_register(lua_vm, "mympd_api", lua_mympd_api);
    lua_register(lua_vm, "mympd_http_client", lua_http_client);
    lua_register(lua_vm, "mympd_http_download", lua_http_download);
    lua_register(lua_vm, "mympd_util_hash", lua_util_hash);
    lua_register(lua_vm, "mympd_util_urlencode", lua_util_urlencode);
    lua_register(lua_vm, "mympd_util_urldecode", lua_util_urldecode);
    lua_register(lua_vm, "mympd_util_log", lua_util_log);
    lua_register(lua_vm, "mympd_util_notify", lua_util_notify);
    lua_register(lua_vm, "mympd_util_covercache_write", lua_util_covercache_write);
    #ifdef MYMPD_ENABLE_MYGPIOD
        lua_register(lua_vm, "mygpio_gpio_blink", lua_mygpio_gpio_blink);
        lua_register(lua_vm, "mygpio_gpio_get", lua_mygpio_gpio_get);
        lua_register(lua_vm, "mygpio_gpio_set", lua_mygpio_gpio_set);
        lua_register(lua_vm, "mygpio_gpio_toggle", lua_mygpio_gpio_toggle);
    #endif
}
