/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lua script functions
 */

#include "compile_time.h"
#include "src/scripts/scripts_lua.h"

#include "src/lib/config_def.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"

#include "src/lib/utility.h"
#include "src/scripts/interface.h"
#include "src/scripts/interface_caches.h"
#include "src/scripts/interface_http.h"
#ifdef MYMPD_ENABLE_MYGPIOD
    #include "src/scripts/interface_mygpio.h"
#endif
#include "src/scripts/interface_mympd_api.h"
#include "src/scripts/interface_util.h"
#include "src/scripts/scripts_worker.h"

#include <string.h>

#ifdef MYMPD_EMBEDDED_ASSETS
    //embedded files for release build
    #include "src/scripts/lualibs.c"
#endif

// Private definitions

static bool create_lua_vm(struct t_script_thread_arg *script_arg);
static bool script_load(struct t_script_thread_arg *script_arg, const char *script);
static bool script_load_bytecode(struct t_script_thread_arg *script_arg, sds bytecode);
static int save_bytecode(lua_State *lua_vm, struct t_script_list_data *data);
static void populate_lua_global_vars(struct t_scripts_state *scripts_state,
        struct t_script_thread_arg *script_arg, struct t_list *arguments);
static void register_lua_functions(lua_State *lua_vm);
static int mympd_luaopen(lua_State *lua_vm, const char *lualib);

// Public functions

/**
 * Executes the script in a new thread.
 * @param scripts_state Pointer to script_state
 * @param scriptname Script name to execute
 * @param arguments Script arguments
 * @param partition MPD Partition
 * @param localscript Script is saved locally?
 * @param start_event Event starting the script
 * @param request_id Jsonrpc ID
 * @param conn_id Mongoose request id
 * @param error Pointer to already allocated sds string for the error message
 * @return true on success, else false
 */
bool script_start(struct t_scripts_state *scripts_state, sds scriptname, struct t_list *arguments,
        const char *partition, bool localscript, enum script_start_events start_event,
        unsigned request_id, unsigned long conn_id, sds *error)
{
    if (script_worker_threads > MAX_SCRIPT_WORKER_THREADS) {
        if (start_event == SCRIPT_START_HTTP) {
            send_script_raw_error(conn_id, partition, "Too many script worker threads already running.");
        }
        else {
            *error = sdscat(*error, "Too many script worker threads already running.");
        }
        return false;
    }

    struct t_script_thread_arg *script_arg = malloc_assert(sizeof(struct t_script_thread_arg));
    script_arg->partition = sdsnew(partition);
    script_arg->start_event = start_event;
    script_arg->conn_id = start_event == SCRIPT_START_HTTP ? conn_id : 0;
    script_arg->request_id = request_id;
    script_arg->config = scripts_state->config;
    script_arg->lua_vm = NULL;
    bool rc;

    if (localscript == true) {
        script_arg->script_name = sdsdup(scriptname);
        // Load script from list
        struct t_list_node *script = list_get_node(&scripts_state->script_list, scriptname);
        if (script != NULL) {
            #ifdef MYMPD_DEBUG
                MEASURE_INIT
                MEASURE_START
            #endif
            struct t_script_list_data *data = (struct t_script_list_data *)script->user_data;
            if (data->bytecode == NULL) {
                MYMPD_LOG_DEBUG(partition, "Compiling lua script");
                rc = script_load(script_arg, data->script);
                if (rc == true) {
                    if (save_bytecode(script_arg->lua_vm, data) == 0) {
                        MYMPD_LOG_DEBUG(partition, "Lua byte code cached successfully");
                    }
                    else {
                        MYMPD_LOG_ERROR(partition, "Error caching lua bytecode");
                    }
                }
            }
            else {
                MYMPD_LOG_DEBUG(partition, "Loading cached lua bytecode");
                rc = script_load_bytecode(script_arg, data->bytecode);
            }
            #ifdef MYMPD_DEBUG
                MEASURE_END
                MEASURE_PRINT(partition, "SCRIPT_START")
            #endif
        }
        else {
            rc = false;
        }
    }
    else {
        script_arg->script_name = sdsnew("user_defined");
        rc = script_load(script_arg, scriptname);
    }

    if (script_arg->lua_vm == NULL) {
        if (start_event == SCRIPT_START_HTTP) {
            send_script_raw_error(conn_id, partition, "Error creating Lua instance.");
        }
        else {
            *error = sdscat(*error, "Error creating Lua instance.");
        }
        free_t_script_thread_arg(script_arg);
        return false;
    }
    if (rc == false) {
        sds result = script_get_result(script_arg->lua_vm, rc);
        if (start_event == SCRIPT_START_HTTP) {
            send_script_raw_error(conn_id, partition, result);
        }
        else {
            *error = sdscat(*error, result);
        }
        MYMPD_LOG_ERROR(script_arg->partition, "Error executing script %s: %s", script_arg->script_name, result);
        FREE_SDS(result);
        free_t_script_thread_arg(script_arg);
        return false;
    }

    populate_lua_global_vars(scripts_state, script_arg, arguments);

    //execute script
    pthread_t scripts_worker_thread;
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0 ||
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 ||
        pthread_create(&scripts_worker_thread, &attr, script_run, script_arg) != 0)
    {
        MYMPD_LOG_ERROR(NULL, "Can not create script worker thread");
        if (start_event == SCRIPT_START_HTTP) {
            send_script_raw_error(conn_id, partition, "Can not create script worker thread");
        }
        else {
            *error = sdscat(*error, "Can not create script worker thread");
        }
        free_t_script_thread_arg(script_arg);
        return false;
    }
    mympd_queue_expire_age(script_worker_queue, 120);
    return true;
}

/**
 * Validates (compiles) a lua script
 * @param config Pointer to config
 * @param scriptname name of the script
 * @param script the script itself
 * @param error already allocated sds string to hold the error message
 * @return true on success, else false
 */
bool script_validate(struct t_config *config, sds scriptname, sds script, sds *error) {
    struct t_script_thread_arg script_arg;
    script_arg.script_name = scriptname;
    script_arg.partition = NULL;
    script_arg.config = config;
    script_arg.request_id = 0;

    bool rc = script_load(&script_arg, script);
    if (script_arg.lua_vm == NULL) {
        *error = sdscat(*error, "Error creating Lua instance.");
        return false;
    }
    sds result = script_get_result(script_arg.lua_vm, rc);
    lua_close(script_arg.lua_vm);
    if (rc == true) {
        FREE_SDS(result);
        return true;
    }
    //compilation error
    MYMPD_LOG_ERROR(NULL, "Error validating script %s: %s", script_arg.script_name, result);
    *error = sdscatsds(*error, result);
    FREE_SDS(result);
    return false;
}

// Private functions

/**
 * Creates the lua instance and opens the standard and myMPD libraries
 * @param script_arg 
 * @return true on success, else false
 */
static bool create_lua_vm(struct t_script_thread_arg *script_arg) {
    script_arg->lua_vm = luaL_newstate();
    if (script_arg->lua_vm == NULL) {
        MYMPD_LOG_ERROR(script_arg->partition, "Memory allocation error in luaL_newstate");
        return false;
    }
    luaL_openlibs(script_arg->lua_vm);
    if (mympd_luaopen(script_arg->lua_vm, "json") == 1 ||
        mympd_luaopen(script_arg->lua_vm, "mympd") == 1)
    {
        lua_close(script_arg->lua_vm);
        script_arg->lua_vm = NULL;
        return false;
    }
    register_lua_functions(script_arg->lua_vm);
    return true;
}

/**
 * Loads the script from a string
 * @param script_arg pointer to t_script_thread_arg struct
 * @param script script to load
 * @return 0 on success, else 1
 */
static bool script_load(struct t_script_thread_arg *script_arg, const char *script) {
    return create_lua_vm(script_arg) &&
        luaL_loadstring(script_arg->lua_vm, script) == 0;
}

/**
 * Loads the cached bytecode
 * This should be faster than compiling the script on each execution.
 * @param script_arg pointer to t_script_thread_arg struct
 * @param bytecode compiled lua script
 * @return 0 on success, else 1
 */
static bool script_load_bytecode(struct t_script_thread_arg *script_arg, sds bytecode) {
    return create_lua_vm(script_arg) &&
        luaL_loadbuffer(script_arg->lua_vm, bytecode, sdslen(bytecode), script_arg->script_name) == 0;
}

/**
 * Callback function for lua_dump to save the lua script bytecode
 * @param lua_vm lua state
 * @param p chunk to write
 * @param sz chunk size
 * @param ud user data
 * @return 0 on success
 */
static int dump_cb(lua_State *lua_vm, const void* p, size_t sz, void* ud) {
    (void)lua_vm;
    struct t_script_list_data *data = (struct t_script_list_data *)ud;
    data->bytecode = sdscatlen(data->bytecode, p, sz);
    return 0;
}

/**
 * Saves the lua bytecode
 * @param lua_vm lua state
 * @param data pointer to script_list data
 * @return 0 on success
 */
static int save_bytecode(lua_State *lua_vm, struct t_script_list_data *data) {
    FREE_SDS(data->bytecode);
    data->bytecode = sdsempty();
    return lua_dump(lua_vm, dump_cb, data, false);
}

/**
 * Populate the global vars for script execution
 * @param scripts_state pointer to scripts_state
 * @param script_arg pointer to t_script_thread_arg struct
 * @param arguments list of arguments
 */
static void populate_lua_global_vars(struct t_scripts_state *scripts_state,
        struct t_script_thread_arg *script_arg, struct t_list *arguments)
{
    // Set myMPD config as a global
    lua_pushlightuserdata(script_arg->lua_vm, script_arg->config);
    lua_setglobal(script_arg->lua_vm, "mympd_config");
    // Set global mympd_env lua table
    lua_newtable(script_arg->lua_vm);
    populate_lua_table_field_p(script_arg->lua_vm, "partition", script_arg->partition);
    populate_lua_table_field_i(script_arg->lua_vm, "requestid", script_arg->request_id);
    populate_lua_table_field_p(script_arg->lua_vm, "scriptevent", script_start_event_name(script_arg->start_event));
    populate_lua_table_field_p(script_arg->lua_vm, "scriptname", script_arg->script_name);
    sds cachedir = sdscatfmt(sdsempty(), "%s/%s", script_arg->config->cachedir,  DIR_CACHE_COVER);
    populate_lua_table_field_p(script_arg->lua_vm, "cachedir_cover", cachedir);
    sdsclear(cachedir);
    cachedir = sdscatfmt(cachedir, "%s/%s", script_arg->config->cachedir,  DIR_CACHE_LYRICS);
    populate_lua_table_field_p(script_arg->lua_vm, "cachedir_lyrics", cachedir);
    sdsclear(cachedir);
    cachedir = sdscatfmt(cachedir, "%s/%s", script_arg->config->cachedir,  DIR_CACHE_MISC);
    populate_lua_table_field_p(script_arg->lua_vm, "cachedir_misc", cachedir);
    sdsclear(cachedir);
    cachedir = sdscatfmt(cachedir, "%s/%s", script_arg->config->cachedir,  DIR_CACHE_THUMBS);
    populate_lua_table_field_p(script_arg->lua_vm, "cachedir_thumbs", cachedir);
    FREE_SDS(cachedir);
    populate_lua_table_field_p(script_arg->lua_vm, "workdir", script_arg->config->workdir);
    // User defined variables
    struct t_list_node *current = scripts_state->var_list.head;
    sds key = sdsempty();
    while (current != NULL) {
        key = sdscatfmt(key, "var_%S", current->key);
        populate_lua_table_field_p(script_arg->lua_vm, key, current->value_p);
        sdsclear(key);
        current = current->next;
    }
    FREE_SDS(key);
    lua_setglobal(script_arg->lua_vm, "mympd_env");

    // Set global arguments lua table
    lua_newtable(script_arg->lua_vm);
    if (arguments->length > 0) {
        current = arguments->head;
        while (current != NULL) {
            populate_lua_table_field_p(script_arg->lua_vm, current->key, current->value_p);
            current = current->next;
        }
    }
    lua_setglobal(script_arg->lua_vm, "mympd_arguments");
}

/**
 * Loads a lua library from filesystem or embedded in release
 * @param lua_vm lua instance
 * @param lualib lua library to load
 * @return true on success, else false
 */
static int mympd_luaopen(lua_State *lua_vm, const char *lualib) {
    MYMPD_LOG_DEBUG(NULL, "Loading embedded lua library %s", lualib);
    #ifdef MYMPD_EMBEDDED_ASSETS
        int rc;
        if (strcmp(lualib, "json") == 0) {
            rc = luaL_loadbuffer(lua_vm, (const char *)json_lua_data, json_lua_size, lualib);
        }
        else if (strcmp(lualib, "mympd") == 0) {
            rc = luaL_loadbuffer(lua_vm, (const char *)mympd_lua_data, mympd_lua_size, lualib);
        }
        else {
            return false;
        }
        if (rc == 0) {
            rc = lua_pcall(lua_vm, 0, 1, 0);
        }
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
    lua_register(lua_vm, "mympd_http_serve_file", lua_http_serve_file);
    lua_register(lua_vm, "mympd_util_hash", lua_util_hash);
    lua_register(lua_vm, "mympd_util_urlencode", lua_util_urlencode);
    lua_register(lua_vm, "mympd_util_urldecode", lua_util_urldecode);
    lua_register(lua_vm, "mympd_util_log", lua_util_log);
    lua_register(lua_vm, "mympd_util_notify", lua_util_notify);
    lua_register(lua_vm, "mympd_caches_images_write", lua_caches_images_write);
    lua_register(lua_vm, "mympd_caches_lyrics_write", lua_caches_lyrics_write);
    lua_register(lua_vm, "mympd_caches_update_mtime", lua_caches_update_mtime);
    #ifdef MYMPD_ENABLE_MYGPIOD
        lua_register(lua_vm, "mygpio_gpio_blink", lua_mygpio_gpio_blink);
        lua_register(lua_vm, "mygpio_gpio_get", lua_mygpio_gpio_get);
        lua_register(lua_vm, "mygpio_gpio_set", lua_mygpio_gpio_set);
        lua_register(lua_vm, "mygpio_gpio_toggle", lua_mygpio_gpio_toggle);
    #endif
}
