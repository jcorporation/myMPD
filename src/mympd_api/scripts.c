/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/scripts.h"

#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/http_client.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/lua_mympd_state.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"

#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef MYMPD_ENABLE_LUA

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#ifdef MYMPD_EMBEDDED_ASSETS
    //embedded files for release build
    #include "scripts_lualibs.c"
#endif

/**
 * Private definitions
 */

/**
 * Struct for passing values to the script thread
 */
struct t_script_thread_arg {
    sds lualibs;               //!< comma separated string of lua libs to load
    bool localscript;          //!< true = read script from filesystem, false = use script_content
    sds script_fullpath;       //!< full uri of the script
    sds script_name;           //!< name of the script
    sds script_content;        //!< script content if localscript = false
    sds partition;             //!< execute the script in this partition
    struct t_list *arguments;  //!< argumentlist
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
static void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg);
static bool mympd_luaopen(lua_State *lua_vm, const char *lualib);
static sds parse_script_metadata(sds buffer, const char *scriptfilename, int *order);
static int lua_http_client(lua_State *lua_vm);

/**
 * Public functions
 */

/**
 * Lists scripts
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param all true = print all scripts, false = print only scripts with order > 0
 * @return pointer to buffer
 */
sds mympd_api_script_list(sds workdir, sds buffer, long request_id, bool all) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SCRIPT_LIST;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    sds scriptdirname = sdscatfmt(sdsempty(), "%S/%s", workdir, DIR_WORK_SCRIPTS);
    errno = 0;
    DIR *script_dir = opendir(scriptdirname);
    if (script_dir == NULL) {
        MYMPD_LOG_ERROR("Can not open directory \"%s\"", scriptdirname);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(scriptdirname);
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Can not open script directory");
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
        strip_file_extension(scriptname);
        entry = sdscatlen(entry, "{", 1);
        entry = tojson_char(entry, "name", scriptname, true);
        scriptfilename = sdscatfmt(scriptfilename, "%S/%s", scriptdirname, next_file->d_name);
        int order = 0;
        entry = parse_script_metadata(entry, scriptfilename, &order);
        entry = sdscatlen(entry, "}", 1);
        if (all == true ||
            order > 0)
        {
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
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Deletes a script
 * @param workdir working directory
 * @param script script to delete (name without extension)
 * @return true on success, else false
 */
bool mympd_api_script_delete(sds workdir, sds script) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", workdir, DIR_WORK_SCRIPTS, script);
    bool rc = rm_file(filepath);
    FREE_SDS(filepath);
    return rc;
}

/**
 * Saves a script
 * @param workdir working directory
 * @param script scriptname
 * @param oldscript old scriptname (leave empty for a new script)
 * @param order script list is order by this value
 * @param content script content
 * @param arguments arguments for the script
 * @return true on success, else false
 */
bool mympd_api_script_save(sds workdir, sds script, sds oldscript, int order, sds content, struct t_list *arguments) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", workdir, DIR_WORK_SCRIPTS, script);
    sds argstr = list_to_json_array(sdsempty(), arguments);
    sds script_content = sdscatfmt(sdsempty(), "-- {\"order\":%i,\"arguments\":%S}\n%S", order, argstr, content);
    bool rc = write_data_to_file(filepath, script_content, sdslen(script_content));
    //delete old scriptfile
    if (rc == true &&
        sdslen(oldscript) > 0 &&
        strcmp(script, oldscript) != 0)
    {
        sds old_filepath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", workdir, DIR_WORK_SCRIPTS, oldscript);
        rc = rm_file(old_filepath);
        FREE_SDS(old_filepath);
    }
    FREE_SDS(argstr);
    FREE_SDS(script_content);
    FREE_SDS(filepath);
    return rc;
}

/**
 * Gets the script and its details
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param script scriptname to read from filesystem
 * @return pointer to buffer
 */
sds mympd_api_script_get(sds workdir, sds buffer, long request_id, sds script) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SCRIPT_GET;
    sds scriptfilename = sdscatfmt(sdsempty(), "%S/%s/%S.lua", workdir, DIR_WORK_SCRIPTS, script);
    errno = 0;
    FILE *fp = fopen(scriptfilename, OPEN_FLAGS_READ);
    if (fp != NULL) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = tojson_sds(buffer, "script", script, true);
        sds line = sdsempty();
        if (sds_getline(&line, fp, LINE_LENGTH_MAX) >= 0 &&
            strncmp(line, "-- ", 3) == 0)
        {
            sdsrange(line, 3, -1);
            if (line[0] == '{' &&
                line[sdslen(line) - 1] == '}')
            {
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
        sds_getfile_from_fp(&content, fp, SCRIPTS_SIZE_MAX, false);
        (void) fclose(fp);
        buffer = sds_catjson(buffer, content, sdslen(content));
        FREE_SDS(content);
        buffer = jsonrpc_end(buffer);
    }
    else {
        MYMPD_LOG_ERROR("Can not open file \"%s\"", scriptfilename);
        MYMPD_LOG_ERRNO(errno);
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Can not open scriptfile");
    }
    FREE_SDS(scriptfilename);

    return buffer;
}

/**
 * Executes a script in a new detached thread
 * @param workdir working directory
 * @param script script to execute (name or script content)
 * @param lualibs comma separated string of lua libs to load
 * @param arguments argument list for the script
 * @param localscript true = load script from filesystem, false = load script from script parameter
 * @return true on success, else false
 */
bool mympd_api_script_start(sds workdir, sds script, sds lualibs, struct t_list *arguments,
        const char *partition, bool localscript)
{
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
    script_thread_arg->lualibs = lualibs;
    script_thread_arg->localscript = localscript;
    script_thread_arg->arguments = arguments;
    script_thread_arg->partition = sdsnew(partition);
    if (localscript == true) {
        script_thread_arg->script_name = sdsnew(script);
        script_thread_arg->script_fullpath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", workdir, DIR_WORK_SCRIPTS, script);
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
    mympd_queue_expire(mympd_script_queue, 120);
    return true;
}

/**
 * Private functions
 */

/**
 * Parses the script metadata line.
 * The metadata line is the first line of the scriptfile.
 * @param buffer already allocated sds string to append the metadata
 * @param scriptfilename file to read
 * @param order pointer to int to populate with order
 * @return pointer to buffer
 */
static sds parse_script_metadata(sds buffer, const char *scriptfilename, int *order) {
    errno = 0;
    FILE *fp = fopen(scriptfilename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Can not open file \"%s\"", scriptfilename);
        MYMPD_LOG_ERRNO(errno);
        return buffer;
    }

    sds line = sdsempty();
    if (sds_getline(&line, fp, LINE_LENGTH_MAX) >= 0 &&
        strncmp(line, "-- ", 3) == 0)
    {
        sdsrange(line, 3, -1);
        if (json_get_int(line, "$.order", 0, 99, order, NULL) == true) {
            buffer = sdscat(buffer, "\"metadata\":");
            buffer = sdscatsds(buffer, line);
        }
        else {
            MYMPD_LOG_WARN("Invalid metadata for script %s", scriptfilename);
            buffer = sdscat(buffer, "\"metadata\":{\"order\":0,\"arguments\":[]}");
            *order = 0;
        }
    }
    else {
        MYMPD_LOG_WARN("Invalid metadata for script %s", scriptfilename);
        buffer = sdscat(buffer, "\"metadata\":{\"order\":0,\"arguments\":[]}");
        *order = 0;
    }
    FREE_SDS(line);
    (void) fclose(fp);
    return buffer;
}

/**
 * Executes the script.
 * This is the main function of the new thread.
 * @param script_thread_arg pointer to t_script_thread_arg struct
 */
static void *mympd_api_script_execute(void *script_thread_arg) {
    thread_logname = sds_replace(thread_logname, "script");
    prctl(PR_SET_NAME, thread_logname, 0, 0, 0);
    struct t_script_thread_arg *script_arg = (struct t_script_thread_arg *) script_thread_arg;

    const char *script_return_text = NULL;
    lua_State *lua_vm = luaL_newstate();
    if (lua_vm == NULL) {
        MYMPD_LOG_ERROR("Memory allocation error in luaL_newstate");
        sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR,
            "Error executing script %{script}: Memory allocation error", 2, "script", script_arg->script_name);
        ws_notify(buffer, script_arg->partition);
        FREE_SDS(buffer);
        FREE_SDS(thread_logname);
        free_t_script_thread_arg(script_arg);
        return NULL;
    }
    if (strcmp(script_arg->lualibs, "all") == 0) {
        MYMPD_LOG_DEBUG("Open all standard lua libs");
        luaL_openlibs(lua_vm);
        mympd_luaopen(lua_vm, "json");
        mympd_luaopen(lua_vm, "mympd");
    }
    else {
        int count = 0;
        sds *tokens = sdssplitlen(script_arg->lualibs, (ssize_t)sdslen(script_arg->lualibs), ",", 1, &count);
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
        //set global lua variable partition
        lua_pushstring(lua_vm, script_arg->partition);
        lua_setglobal(lua_vm, "partition");
        //set arguments lua table
        lua_newtable(lua_vm);
        if (script_arg->arguments->length > 0) {
            struct t_list_node *current = script_arg->arguments->head;
            while (current != NULL) {
                populate_lua_table_field_p(lua_vm, current->key, current->value_p);
                current = current->next;
            }
        }
        lua_setglobal(lua_vm, "arguments");
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
            sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_SCRIPT,
                JSONRPC_SEVERITY_INFO, "Script %{script} executed successfully",
                2, "script", script_arg->script_name);
            ws_notify(buffer, script_arg->partition);
            FREE_SDS(buffer);
        }
        else {
            send_jsonrpc_notify(JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_INFO, script_arg->partition, script_return_text);
        }
    }
    else {
        //return message
        sds err_str = lua_err_to_str(sdsempty(), rc, true, script_arg->script_name);
        if (script_return_text != NULL) {
            err_str = sdscatfmt(err_str, ": %s", script_return_text);
        }
        sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_SCRIPT,
            JSONRPC_SEVERITY_ERROR, err_str, 2, "script", script_arg->script_name);
        ws_notify(buffer, script_arg->partition);
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

/**
 * Returns a phrase for lua errors
 * @param buffer already allocated sds string to append the error message
 * @param rc return code of the lua script
 * @param phrase true if we should return a phrase for the translation framework
 * @param script executed script
 * @return pointer to buffer
 */
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

/**
 * Loads a lue library from filesystem or embedded in release
 * @param lua_vm lua instance
 * @param lualib lua library to load
 * @return true on success, else false
 */
static bool mympd_luaopen(lua_State *lua_vm, const char *lualib) {
    MYMPD_LOG_DEBUG("Loading embedded lua library %s", lualib);
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

/**
 * Helper functions to push a lua table
 * @param lua_vm lua instance
 * @param key the key
 * @param value string value
 */
static void populate_lua_table_field_p(lua_State *lua_vm, const char *key, const char *value) {
    lua_pushstring(lua_vm, key);
    lua_pushstring(lua_vm, value);
    lua_settable(lua_vm, -3);
}

/**
 * Helper functions to push a lua table
 * @param lua_vm lua instance
 * @param key the key
 * @param value long long value (lua integer)
 */
static void populate_lua_table_field_i(lua_State *lua_vm, const char *key, long long value) {
    lua_pushstring(lua_vm, key);
    lua_pushinteger(lua_vm, value);
    lua_settable(lua_vm, -3);
}

/**
 * Helper functions to push a lua table
 * @param lua_vm lua instance
 * @param key the key
 * @param value double value (lua number)
 */
static void populate_lua_table_field_f(lua_State *lua_vm, const char *key, double value) {
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
static void populate_lua_table_field_b(lua_State *lua_vm, const char *key, bool value) {
    lua_pushstring(lua_vm, key);
    lua_pushboolean(lua_vm, value);
    lua_settable(lua_vm, -3);
}

/**
 * Populates the lua table from the lua_mympd_state struct
 * @param lua_vm lua instance
 * @param lua_mympd_state 
 */
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

/**
 * Registers myMPD specific lua functions
 * @param lua_vm lua instance
 */
static void register_lua_functions(lua_State *lua_vm) {
    lua_register(lua_vm, "mympd_api", mympd_api);
    lua_register(lua_vm, "mympd_api_http_client", lua_http_client);
}

/**
 * Function that implements mympd_api lua function
 * @param lua_vm lua instance
 * @return return code
 */
static int mympd_api(lua_State *lua_vm) {
    //check arguments
    int n = lua_gettop(lua_vm);
    if (n != 2) {
        MYMPD_LOG_ERROR("Lua - mympd_api: invalid number of arguments");
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    //get method
    const char *method = lua_tostring(lua_vm, 1);
    enum mympd_cmd_ids method_id = get_cmd_id(method);
    if (method_id == GENERAL_API_UNKNOWN) {
        MYMPD_LOG_ERROR("Lua - mympd_api: Invalid method \"%s\"", method);
        return luaL_error(lua_vm, "Invalid method");
    }
    //get partition
    lua_getglobal(lua_vm, "partition");
    const char *partition = lua_tostring(lua_vm, -1);
    if (partition == NULL) {
        MYMPD_LOG_ERROR("Lua - mympd_api: Invalid partition");
        return luaL_error(lua_vm, "Invalid partition");
    }
    //get the thread id
    long tid = syscall(__NR_gettid);
    //create the request
    struct t_work_request *request = create_request(-2, tid, method_id, NULL, partition);
    const char *params = lua_tostring(lua_vm, 2);
    if (params[0] != '{') {
        //param is invalid json, ignore it
        request->data = sdscatlen(request->data, "}", 1);
    }
    else {
        sdsrange(request->data, 0, -2); //trim opening curly bracket
        request->data = sdscat(request->data, params);
    }
    request->data = sdscatlen(request->data, "}", 1);
    mympd_queue_push(mympd_api_queue, request, tid);

    int i = 0;
    while (s_signal_received == 0 && i < 60) {
        i++;
        struct t_work_response *response = mympd_queue_shift(mympd_script_queue, 1000000, tid);
        if (response != NULL) {
            MYMPD_LOG_DEBUG("Got response: %s", response->data);
            if (response->cmd_id == INTERNAL_API_SCRIPT_INIT) {
                //this populates a lua table with some MPD and myMPD states
                MYMPD_LOG_DEBUG("Populating lua global state table mympd");
                if (response->extra != NULL) {
                    struct t_list *lua_mympd_state = (struct t_list *)response->extra;
                    lua_newtable(lua_vm);
                    populate_lua_table(lua_vm, lua_mympd_state);
                    lua_setglobal(lua_vm, "mympd_state");
                    lua_mympd_state_free(lua_mympd_state);
                }
            }
            //push return code and jsonrpc response
            int rc = json_find_key(response->data, "$.error.message") == true ? 1 : 0;
            lua_pushinteger(lua_vm, rc);
            lua_pushlstring(lua_vm, response->data, sdslen(response->data));
            free_response(response);
            //return response count
            return 2;
        }
    }
    return luaL_error(lua_vm, "No API response, timeout after 60s");
}

/**
 * Frees the t_script_thread_arg struct
 * @param script_thread_arg pointer to the struct to free
 */
static void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg) {
    FREE_SDS(script_thread_arg->script_name);
    FREE_SDS(script_thread_arg->script_fullpath);
    FREE_SDS(script_thread_arg->script_content);
    FREE_SDS(script_thread_arg->partition);
    list_free(script_thread_arg->arguments);
    FREE_PTR(script_thread_arg);
}

/**
 * Simple HTTP client for lua
 * @param lua_vm lua instance
 * @return number of variables on the stack with the response
 */
static int lua_http_client(lua_State *lua_vm) {
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
        .response_code = 0,
        .header = sdsempty(),
        .body = sdsempty()
    };

    http_client_request(&mg_client_request, &mg_client_response);

    lua_pushinteger(lua_vm, mg_client_response.rc);
    lua_pushinteger(lua_vm, mg_client_response.response_code);
    lua_pushlstring(lua_vm, mg_client_response.header, sdslen(mg_client_response.header));
    lua_pushlstring(lua_vm, mg_client_response.body, sdslen(mg_client_response.body));
    FREE_SDS(mg_client_response.header);
    FREE_SDS(mg_client_response.body);
    //return response count
    return 4;
}

#endif
