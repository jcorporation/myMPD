/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_SCRIPTS_UTIL_H
#define MYMPD_SCRIPTS_UTIL_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"
#include "src/scripts/events.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/**
 * Holds central scripts state and configuration values.
 */
struct t_scripts_state {
    struct t_config *config;     //!< pointer to static config
    struct t_list var_list;      //!< list of variables for scripts
    struct t_list script_list;   //!< list of scripts
};

/**
 * Userdata for script_list
 */
struct t_script_list_data {
    sds script;    //!< script itself
    sds bytecode;  //!< precompiled script byte code
};

/**
 * Struct for passing values to the script execute function
 */
struct t_script_thread_arg {
    lua_State *lua_vm;                     //!< new lua vm
    sds script_name;                       //!< name of the script
    sds partition;                         //!< execute the script in this partition
    enum script_start_events start_event;  //!< script start event
    unsigned long conn_id;                 //!< mongoose connection id
    unsigned request_id;                   //!< jsonrpc request id
    struct t_config *config;               //!< pointer to myMPD config
};

void list_free_cb_script_list_user_data(struct t_list_node *current);
void scripts_state_save(struct t_scripts_state *scripts_state, bool free_data);
void scripts_state_default(struct t_scripts_state *scripts_state, struct t_config *config);
void scripts_state_free(struct t_scripts_state *scripts_state);
void send_script_raw_response(unsigned long conn_id, const char *partition, sds data);
void send_script_raw_error(unsigned long conn_id, const char *partition, const char *data);
sds script_get_result(lua_State *lua_vm, int rc);
void free_t_script_thread_arg(struct t_script_thread_arg *script_thread_arg);

#endif
