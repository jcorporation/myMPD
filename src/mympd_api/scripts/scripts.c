/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/scripts/scripts.h"

#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/thread.h"
#include "src/lib/utility.h"
#include "src/mympd_api/scripts/interface.h"
#include "src/mympd_api/scripts/interface_http_client.h"
#include "src/mympd_api/scripts/interface_mygpio.h"
#include "src/mympd_api/scripts/interface_mympd_api.h"
#include "src/mympd_api/scripts/interface_util.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#ifdef MYMPD_EMBEDDED_ASSETS
    //embedded files for release build
    #include "src/mympd_api/scripts/lualibs.c"
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

static lua_State *script_load(struct t_script_thread_arg *script_arg, int *rc);
static void *script_execute(void *script_thread_arg);
static sds script_get_result(lua_State *lua_vm, int rc);
const char *lua_err_to_str(int rc);
static void register_lua_functions(lua_State *lua_vm);
static void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg);
static bool mympd_luaopen(lua_State *lua_vm, const char *lualib);
static sds parse_script_metadata(sds buffer, const char *scriptfilename, int *order);

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
sds mympd_api_script_list(sds workdir, sds buffer, unsigned request_id, bool all) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SCRIPT_LIST;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    sds scriptdirname = sdscatfmt(sdsempty(), "%S/%s", workdir, DIR_WORK_SCRIPTS);
    errno = 0;
    DIR *script_dir = opendir(scriptdirname);
    if (script_dir == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can not open directory \"%s\"", scriptdirname);
        MYMPD_LOG_ERRNO(NULL, errno);
        FREE_SDS(scriptdirname);
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Can not open script directory");
        return buffer;
    }

    struct dirent *next_file;
    unsigned returned_entities = 0;
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
            if (returned_entities++) {
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
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint(buffer, "returnedEntities", returned_entities, true);
    buffer = tojson_uint(buffer, "totalEntities", returned_entities, false);
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
 * @param error already allocated sds string to hold the error message
 * @return true on success, else false
 */
bool mympd_api_script_save(sds workdir, sds script, sds oldscript, int order, sds content, struct t_list *arguments, sds *error) {
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
    if (rc == false) {
        *error = sdscat(*error, "Could not save script");
    }
    return rc;
}

/**
 * Validates (compiles) a lua script
 * @param name name of the script
 * @param content the script itself
 * @param lualibs lua libraries to load
 * @param error already allocated sds string to hold the error message
 * @return true on success, else false
 */
bool mympd_api_script_validate(sds name, sds content, sds lualibs, sds *error) {
    struct t_script_thread_arg script_arg;
    script_arg.script_name = name;
    script_arg.script_content = content;
    script_arg.lualibs = lualibs;
    script_arg.localscript = false;
    script_arg.arguments = NULL;
    script_arg.partition = NULL;
    script_arg.script_fullpath = NULL;

    int rc = 0;
    lua_State *lua_vm = script_load(&script_arg, &rc);
    if (lua_vm == NULL) {
        return false;
    }
    sds result = script_get_result(lua_vm, rc);
    lua_close(lua_vm);
    if (rc == 0) {
        FREE_SDS(result);
        return true;
    }
    //compilation error
    MYMPD_LOG_ERROR(NULL, "Error validating script %s: %s", script_arg.script_name, result);
    *error = sdscatsds(*error, result);
    FREE_SDS(result);
    return false;
}

/**
 * Gets the script and its details
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param script scriptname to read from filesystem
 * @return pointer to buffer
 */
sds mympd_api_script_get(sds workdir, sds buffer, unsigned request_id, sds script) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SCRIPT_GET;
    sds scriptfilename = sdscatfmt(sdsempty(), "%S/%s/%S.lua", workdir, DIR_WORK_SCRIPTS, script);
    errno = 0;
    FILE *fp = fopen(scriptfilename, OPEN_FLAGS_READ);
    if (fp != NULL) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = tojson_sds(buffer, "script", script, true);
        int nread = 0;
        sds line = sds_getline(sdsempty(), fp, LINE_LENGTH_MAX, &nread);
        if (nread >= 0 &&
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
                MYMPD_LOG_WARN(NULL, "Invalid metadata for script %s", scriptfilename);
                buffer = sdscat(buffer, "\"metadata\":{\"order\":0, \"arguments\":[]}");
            }
        }
        else {
            MYMPD_LOG_WARN(NULL, "Invalid metadata for script %s", scriptfilename);
            buffer = sdscat(buffer, "\"metadata\":{\"order\":0, \"arguments\":[]}");
        }
        FREE_SDS(line);
        buffer = sdscat(buffer, ",\"content\":");
        nread = 0;
        sds content = sds_getfile_from_fp(sdsempty(), fp, SCRIPTS_SIZE_MAX, false, &nread);
        (void) fclose(fp);
        buffer = sds_catjson(buffer, content, sdslen(content));
        FREE_SDS(content);
        buffer = jsonrpc_end(buffer);
    }
    else {
        MYMPD_LOG_ERROR(NULL, "Can not open file \"%s\"", scriptfilename);
        MYMPD_LOG_ERRNO(NULL, errno);
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
        MYMPD_LOG_ERROR(NULL, "Can not init mympd_script thread attribute");
        return false;
    }
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
        MYMPD_LOG_ERROR(NULL, "Can not set mympd_script thread to detached");
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
    if (pthread_create(&mympd_script_thread, &attr, script_execute, script_thread_arg) != 0) {
        MYMPD_LOG_ERROR(NULL, "Can not create mympd_script thread");
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
        MYMPD_LOG_ERROR(NULL, "Can not open file \"%s\"", scriptfilename);
        MYMPD_LOG_ERRNO(NULL, errno);
        return buffer;
    }

    int nread = 0;
    sds line = sds_getline(sdsempty(), fp, LINE_LENGTH_MAX, &nread);
    if (nread >= 0 &&
        strncmp(line, "-- ", 3) == 0)
    {
        sdsrange(line, 3, -1);
        if (json_get_int(line, "$.order", 0, 99, order, NULL) == true) {
            buffer = sdscat(buffer, "\"metadata\":");
            buffer = sdscatsds(buffer, line);
        }
        else {
            MYMPD_LOG_WARN(NULL, "Invalid metadata for script %s", scriptfilename);
            buffer = sdscat(buffer, "\"metadata\":{\"order\":0,\"arguments\":[]}");
            *order = 0;
        }
    }
    else {
        MYMPD_LOG_WARN(NULL, "Invalid metadata for script %s", scriptfilename);
        buffer = sdscat(buffer, "\"metadata\":{\"order\":0,\"arguments\":[]}");
        *order = 0;
    }
    FREE_SDS(line);
    (void) fclose(fp);
    return buffer;
}

/**
 * Creates the lua instance and loads the script
 * @param script_arg pointer to t_script_thread_arg struct
 * @param rc loading script return value
 * @return lua instance or NULL on error
 */
static lua_State *script_load(struct t_script_thread_arg *script_arg, int *rc) {
    lua_State *lua_vm = luaL_newstate();
    if (lua_vm == NULL) {
        MYMPD_LOG_ERROR(script_arg->partition, "Memory allocation error in luaL_newstate");
        return NULL;
    }
    if (strcmp(script_arg->lualibs, "all") == 0) {
        MYMPD_LOG_DEBUG(NULL, "Open all standard lua libs");
        luaL_openlibs(lua_vm);
        mympd_luaopen(lua_vm, "json");
        mympd_luaopen(lua_vm, "mympd");
    }
    else {
        int count = 0;
        sds *tokens = sdssplitlen(script_arg->lualibs, (ssize_t)sdslen(script_arg->lualibs), ",", 1, &count);
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
                     strcmp(tokens[i], "mympd") == 0)	  { mympd_luaopen(lua_vm, tokens[i]); }
            else {
                MYMPD_LOG_ERROR(NULL, "Can not open lua library %s", tokens[i]);
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
 * Gets the result from script loading or execution
 * @param lua_vm lua instance
 * @param rc return code
 * @return newly allocated sds string with script return value or error string
 */
static sds script_get_result(lua_State *lua_vm, int rc) {
    //it should be only one value on the stack
    int nr_return = lua_gettop(lua_vm);
    MYMPD_LOG_DEBUG(NULL, "Lua script returns %d values", nr_return);
    for (int i = 1; i <= nr_return; i++) {
        MYMPD_LOG_DEBUG(NULL, "Lua script return value %d: %s", i, lua_tostring(lua_vm, i));
    }
    const char *script_return_text = NULL;
    if (lua_gettop(lua_vm) == 1) {
        //return value on stack
        script_return_text = lua_tostring(lua_vm, 1);
    }
    if (rc == 0) {
        //success
        return script_return_text == NULL
            ? sdsempty()
            : sdsnew(script_return_text);
    }
    //error
    const char *err_str = lua_err_to_str(rc);
    sds result = sdsnew(err_str);
    if (script_return_text != NULL) {
        result = sdscatfmt(result, ": %s", script_return_text);
    }
    return result;
}

/**
 * Executes the script.
 * This is the main function of the new thread.
 * @param script_thread_arg pointer to t_script_thread_arg struct
 */
static void *script_execute(void *script_thread_arg) {
    thread_logname = sds_replace(thread_logname, "script");
    set_threadname(thread_logname);
    struct t_script_thread_arg *script_arg = (struct t_script_thread_arg *) script_thread_arg;

    int rc = 0;
    lua_State *lua_vm = script_load(script_arg, &rc);
    if (lua_vm == NULL) {
        sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR,
            "Error executing script %{script}: Memory allocation error", 2, "script", script_arg->script_name);
        ws_notify(buffer, script_arg->partition);
        FREE_SDS(buffer);
        FREE_SDS(thread_logname);
        free_t_script_thread_arg(script_arg);
        return NULL;
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
        //execute script
        MYMPD_LOG_DEBUG(NULL, "Start script");
        rc = lua_pcall(lua_vm, 0, 1, 0);
        MYMPD_LOG_DEBUG(NULL, "End script");
    }
    sds result = script_get_result(lua_vm, rc);
    if (rc == 0) {
        if (sdslen(result) == 0) {
            sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_SCRIPT,
                JSONRPC_SEVERITY_INFO, "Script %{script} executed successfully",
                2, "script", script_arg->script_name);
            ws_notify(buffer, script_arg->partition);
            FREE_SDS(buffer);
        }
        else {
            //send script return string
            send_jsonrpc_notify(JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_INFO, script_arg->partition, result);
        }
    }
    else {
        sds buffer = jsonrpc_notify_phrase(sdsempty(), JSONRPC_FACILITY_SCRIPT,
            JSONRPC_SEVERITY_ERROR, "Error executing script %{script}: %{msg}",
            4, "script", script_arg->script_name, "msg", result);
        ws_notify(buffer, script_arg->partition);
        FREE_SDS(buffer);
        MYMPD_LOG_ERROR(script_arg->partition, "Error executing script %s: %s", script_arg->script_name, result);
    }
    lua_close(lua_vm);
    FREE_SDS(result);
    free_t_script_thread_arg(script_arg);
    FREE_SDS(thread_logname);
    return NULL;
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

/**
 * Loads a lue library from filesystem or embedded in release
 * @param lua_vm lua instance
 * @param lualib lua library to load
 * @return true on success, else false
 */
static bool mympd_luaopen(lua_State *lua_vm, const char *lualib) {
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
        MYMPD_LOG_DEBUG(NULL, "Lua library return value \"%d\": \"%s\"", i, lua_tostring(lua_vm, i));
        lua_pop(lua_vm, i);
    }
    if (rc != 0) {
        if (lua_gettop(lua_vm) == 1) {
            //return value on stack
            MYMPD_LOG_ERROR(NULL, "Error loading library \"%s\": \"%s\"", lualib, lua_tostring(lua_vm, 1));
        }
    }
    return rc;
}

/**
 * Registers myMPD specific lua functions
 * @param lua_vm lua instance
 */
static void register_lua_functions(lua_State *lua_vm) {
    lua_register(lua_vm, "mympd_api", lua_mympd_api);
    lua_register(lua_vm, "mympd_api_http_client", lua_http_client);
    lua_register(lua_vm, "mympd_util_hash", lua_util_hash);
    lua_register(lua_vm, "mympd_util_urlencode", lua_util_urlencode);
    lua_register(lua_vm, "mympd_util_urldecode", lua_util_urldecode);
    #ifdef MYMPD_ENABLE_MYGPIOD
        lua_register(lua_vm, "mygpio_gpio_blink", lua_mygpio_gpio_blink);
        lua_register(lua_vm, "mygpio_gpio_get", lua_mygpio_gpio_get);
        lua_register(lua_vm, "mygpio_gpio_set", lua_mygpio_gpio_set);
        lua_register(lua_vm, "mygpio_gpio_toggle", lua_mygpio_gpio_toggle);
    #endif
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
