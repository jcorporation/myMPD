/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Script thread utility functions
 */

#include "compile_time.h"
#include "src/scripts/util.h"

#include "src/lib/api.h"
#include "src/lib/config_def.h"
#include "src/lib/list.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/scripts/api_vars.h"

// Private definitions
static const char *lua_err_to_str(int rc);

// Public functions

/**
 * Saves in-memory states to disc. This is done on shutdown and on SIGHUP.
 * @param scripts_state pointer to central scripts state
 * @param free_data true=free the struct, else not
 */
void scripts_state_save(struct t_scripts_state *scripts_state, bool free_data) {
    scripts_vars_file_save(&scripts_state->var_list, scripts_state->config->workdir);
    if (free_data == true) {
        scripts_state_free(scripts_state);
    }
}

/**
 * Sets scripts_state defaults.
 * @param scripts_state pointer to central scripts state
 * @param config pointer to static config
 */
void scripts_state_default(struct t_scripts_state *scripts_state, struct t_config *config) {
    scripts_state->config = config;
    list_init(&scripts_state->var_list);
    list_init(&scripts_state->script_list);
}

/**
 * Callback function to free script_list user_data.
 * @param current list node
 */
void list_free_cb_script_list_user_data(struct t_list_node *current) {
    struct t_script_list_data *data = (struct t_script_list_data *)current->user_data;
    FREE_SDS(data->script);
    FREE_SDS(data->bytecode);
    FREE_PTR(current->user_data);
}

/**
 * Frees the scripts_state.
 * @param scripts_state pointer to central scripts state
 */
void scripts_state_free(struct t_scripts_state *scripts_state) {
    list_clear(&scripts_state->var_list);
    list_clear_user_data(&scripts_state->script_list, list_free_cb_script_list_user_data);
    //struct itself
    FREE_PTR(scripts_state);
}

/**
 * Frees the t_script_thread_arg struct
 * @param script_thread_arg pointer to the struct to free
 */
void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg) {
    FREE_SDS(script_thread_arg->script_name);
    FREE_SDS(script_thread_arg->partition);
    if (script_thread_arg->lua_vm != NULL) {
        lua_close(script_thread_arg->lua_vm);
    }
    FREE_PTR(script_thread_arg);
}

/**
 * Gets the result from script loading or execution
 * @param lua_vm lua instance
 * @param rc return code
 * @return newly allocated sds string with script return value or error string
 */
sds script_get_result(lua_State *lua_vm, int rc) {
    //it should be only one value on the stack
    int nr_return = lua_gettop(lua_vm);
    MYMPD_LOG_DEBUG(NULL, "Lua script returns %d values", nr_return);
    if (rc == 0 &&
        lua_gettop(lua_vm) == 1)
    {
        //success
        if (lua_type(lua_vm, 1) == LUA_TLIGHTUSERDATA) {
            sds script_return_binary = (sds)lua_touserdata(lua_vm, 1);
            MYMPD_LOG_DEBUG(NULL, "Got binary data from script with %lu bytes", (unsigned long)sdslen(script_return_binary));
            return script_return_binary == NULL
                ? sdsempty()
                : script_return_binary;
        }
        // Script has returned a string
        const char *script_return_text = lua_tostring(lua_vm, 1);
        return script_return_text == NULL
            ? sdsempty()
            : sdsnew(script_return_text);
    }
    //error
    const char *error_text = lua_tostring(lua_vm, 1);
    const char *err_str = lua_err_to_str(rc);
    sds result = sdsnew(err_str);
    if (error_text != NULL) {
        result = sdscatfmt(result, ": %s", error_text);
    }
    return result;
}

/**
 * Sends the scripts raw response to the webserver queue
 * @param conn_id mongoose connection id
 * @param partition MPD partition
 * @param data raw http response to send
 */
void send_script_raw_response(unsigned long conn_id, const char *partition, sds data) {
    struct t_work_response *response = create_response_new(RESPONSE_TYPE_RAW, conn_id, 0, INTERNAL_API_RAW, partition);
    response->data = sdscatsds(response->data, data);
    push_response(response);
}

/**
 * Sends the scripts raw error to the webserver queue
 * @param conn_id mongoose connection id
 * @param partition MPD partition
 * @param data raw http response to send
 */
void send_script_raw_error(unsigned long conn_id, const char *partition, const char *data) {
    struct t_work_response *response = create_response_new(RESPONSE_TYPE_RAW, conn_id, 0, INTERNAL_API_RAW, partition);
    sds body = sdscatfmt(sdsempty(), "<!DOCTYPE html><html><head><title>myMPD error</title></head><body>"
        "<h1>myMPD error</h1>"
        "<p>%s</p>"
        "</body></html>",
        data);
    response->data = sdscatfmt(response->data, "HTTP/1.1 500 Internal server error\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %L\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s", sdslen(body), body);
    FREE_SDS(body);
    push_response(response);
}

// Private functions

/**
 * Returns a phrase for lua errors
 * @param rc return code of the lua script
 * @return error string literal
 */
static const char *lua_err_to_str(int rc) {
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
