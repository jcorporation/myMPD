/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <libgen.h>
#include <dirent.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <inttypes.h>

#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "../dist/src/frozen/frozen.h"
#include "api.h"
#include "log.h"
#include "list.h"
#include "tiny_queue.h"
#include "config_defs.h"
#include "utility.h"
#include "global.h"
#include "lua_mympd_state.h"
#include "mpd_client.h"
#include "maintenance.h"
#include "mympd_api/mympd_api_utility.h"
#include "mympd_api/mympd_api_timer.h"
#include "mympd_api/mympd_api_settings.h"
#include "mympd_api/mympd_api_syscmds.h"
#include "mympd_api/mympd_api_bookmarks.h"
#include "mympd_api/mympd_api_timer.h"
#include "mympd_api/mympd_api_timer_handlers.h"
#include "mympd_api/mympd_api_scripts.h"
#include "mympd_api/mympd_api_home.h"
#include "mympd_api.h"

//private definitions
static void mympd_api(t_config *config, t_mympd_state *mympd_state, t_work_request *request);

//public functions
void *mympd_api_loop(void *arg_config) {
    thread_logname = sdsreplace(thread_logname, "mympdapi");
    
    t_config *config = (t_config *) arg_config;
    
    //read myMPD states under config.varlibdir
    t_mympd_state *mympd_state = (t_mympd_state *)malloc(sizeof(t_mympd_state));
    assert(mympd_state);
    mympd_api_read_statefiles(config, mympd_state);

    //home icons
    list_init(&mympd_state->home_list);
    mympd_api_read_home_list(config, mympd_state);

    //myMPD timer
    init_timerlist(&mympd_state->timer_list);
    timerfile_read(config, mympd_state);
    
    //set timers
    if (config->covercache == true) {
        LOG_DEBUG("Setting timer action \"clear covercache\" to periodic each 7200s");
        add_timer(&mympd_state->timer_list, 60, 7200, timer_handler_covercache, 1, NULL, (void *)config);
    }

    //push settings to mpd_client queue
    mympd_api_push_to_mpd_client(mympd_state);

    while (s_signal_received == 0) {
        //poll message queue
        struct t_work_request *request = tiny_queue_shift(mympd_api_queue, 100, 0);
        if (request != NULL) {
            mympd_api(config, mympd_state, request);
        }
        check_timer(&mympd_state->timer_list, mympd_state->timer);
    }

    //cleanup
    mympd_api_write_home_list(config, mympd_state);
    timerfile_save(config, mympd_state);
    free_mympd_state(mympd_state);
    sdsfree(thread_logname);
    return NULL;
}

//private functions
static void mympd_api(t_config *config, t_mympd_state *mympd_state, t_work_request *request) {
    int je;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    char *p_charbuf4 = NULL;
    char *p_charbuf5 = NULL;
    unsigned int uint_buf1;
    unsigned int uint_buf2;
    int int_buf1;
    int int_buf2;
    bool bool_buf1;
    bool rc;

    #ifdef DEBUG
    MEASURE_START
    #endif

    LOG_VERBOSE("MYMPD API request (%d): %s", request->conn_id, request->data);
    //create response struct
    t_work_result *response = create_result(request);
    
    switch(request->cmd_id) {
        case MYMPD_API_STATE_SAVE:
            if (config->home == true) {
                mympd_api_write_home_list(config, mympd_state);
            }
            if (mympd_state->timer == true) {
                timerfile_save(config, mympd_state);
            }
            response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
            break;
        case MYMPD_API_PICTURE_LIST:
            response->data = mympd_api_picture_list(config, response->data, request->method, request->id);
            break;
        case MYMPD_API_HOME_ICON_SAVE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {replace: %B, oldpos: %u, name: %Q, ligature: %Q, bgcolor: %Q, image: %Q, cmd: %Q}}", 
                &bool_buf1, &uint_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3, &p_charbuf4, &p_charbuf5);
            if (je == 7) {
                struct list *options = (struct list *) malloc(sizeof(struct list));
                assert(options);
                list_init(options);
                struct json_token t;
                for (int i = 0; json_scanf_array_elem(request->data, sdslen(request->data), ".params.options", i, &t) > 0; i++) {
                    list_push_len(options, t.ptr, t.len, 0, NULL, 0, NULL);
                }
                rc = mympd_api_save_home_icon(mympd_state, bool_buf1, uint_buf1, p_charbuf1, p_charbuf2, p_charbuf3, p_charbuf4, p_charbuf5, options);
                list_free(options);
                free(options);
                if (rc == true) {
                    response->data = mympd_api_put_home_list(mympd_state, response->data, request->method, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "home", "error", "Can not save home icon");
                }
            }
            break;
        case MYMPD_API_HOME_ICON_MOVE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {from: %u, to: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                rc = mympd_api_move_home_icon(mympd_state, uint_buf1, uint_buf2);
                if (rc == true) {
                    response->data = mympd_api_put_home_list(mympd_state, response->data, request->method, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "home", "error", "Can not move home icon");
                }
            }
            break;
        case MYMPD_API_HOME_ICON_DELETE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {pos: %u}}", &uint_buf1);
            if (je == 1) {
                rc = mympd_api_rm_home_icon(mympd_state, uint_buf1);
                if (rc == true) {
                    response->data = mympd_api_put_home_list(mympd_state, response->data, request->method, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "home", "error", "Can not delete home icon");
                }
            }
            break;
        case MYMPD_API_HOME_ICON_GET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {pos: %u}}", &uint_buf1);
            if (je == 1) {
                response->data = mympd_api_get_home_icon(mympd_state, response->data, request->method, request->id, uint_buf1);
            }
            break;
        case MYMPD_API_HOME_LIST:
            response->data = mympd_api_put_home_list(mympd_state, response->data, request->method, request->id);
            break;
        #ifdef ENABLE_LUA
        case MYMPD_API_SCRIPT_SAVE:
            if (config->scripteditor == false) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                    "script", "error", "Editing scripts is disabled");
                break;
            }
            je = json_scanf(request->data, sdslen(request->data), "{params: {script: %Q, order: %d, content: %Q, oldscript: %Q}}", 
                &p_charbuf1, &int_buf1, &p_charbuf2, &p_charbuf3);
            if (je == 4) {
                struct json_token val;
                int idx;
                sds arguments = sdsempty();
                void *h = NULL;
                while ((h = json_next_elem(request->data, sdslen(request->data), h, ".params.arguments", &idx, &val)) != NULL) {
                    if (idx > 0) {
                        arguments = sdscat(arguments, ",");
                    }
                    arguments = sdscatjson(arguments, val.ptr, val.len);
                }
                if (validate_string_not_empty(p_charbuf1) == true && validate_string(p_charbuf3) == true) {
                    rc = mympd_api_script_save(config, p_charbuf1, int_buf1, p_charbuf2, arguments, p_charbuf3);
                    if (rc == true) {
                        response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "script");
                    }
                    else {
                        response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                            "script", "error", "Could not save script");
                    }
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "script", "error", "Invalid script name");
                }
                sdsfree(arguments);
            }
            break;
        case MYMPD_API_SCRIPT_DELETE:
            if (config->scripteditor == false) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                    "script", "error", "Editing scripts is disabled");
                break;
            }
            je = json_scanf(request->data, sdslen(request->data), "{params: {script: %Q}}", &p_charbuf1);
            if (je == 1 && validate_string_not_empty(p_charbuf1) == true) {
                rc = mympd_api_script_delete(config, p_charbuf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "script");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "script", "error", "Could not delete script");
                }
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                    "script", "error", "Invalid script name");
            }
            break;
        case MYMPD_API_SCRIPT_GET:
            if (config->scripteditor == false) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                    "script", "error", "Editing scripts is disabled");
                break;
            }
            je = json_scanf(request->data, sdslen(request->data), "{params: {script: %Q}}", &p_charbuf1);
            if (je == 1 && validate_string_not_empty(p_charbuf1) == true) {
                response->data = mympd_api_script_get(config, response->data, request->method, request->id, p_charbuf1);
            }
            break;
        case MYMPD_API_SCRIPT_LIST: {
            je = json_scanf(request->data, sdslen(request->data), "{params: {all: %B}}", &bool_buf1);
            if (je == 1) {
                response->data = mympd_api_script_list(config, response->data, request->method, request->id, bool_buf1);
            }
            break;
        }
        case MYMPD_API_SCRIPT_INIT:
            if (config->scripting == true) {
                if (request->extra != NULL) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "script");
                    struct list *lua_mympd_state = (struct list *)request->extra;
                    rc = mympd_api_get_lua_mympd_state(mympd_state, lua_mympd_state);
                    if (rc == false) {
                        LOG_ERROR("Error getting mympd state for script execution");
                    }
                    response->extra = request->extra;
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "script", "error", "No mpd state for script execution submitted");
                }
            } 
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                    "script", "error", "Scripting is disabled");
            }
            break;
        case MYMPD_API_SCRIPT_EXECUTE:
            if (config->scripting == true) {
                je = json_scanf(request->data, sdslen(request->data), "{params: {script: %Q}}", &p_charbuf1);
                if (je == 1 && validate_string_not_empty(p_charbuf1) == true) {
                    struct list *arguments = (struct list *) malloc(sizeof(struct list));
                    assert(arguments);
                    list_init(arguments);
                    void *h = NULL;
                    struct json_token key;
                    struct json_token val;
                    while ((h = json_next_key(request->data, sdslen(request->data), h, ".params.arguments", &key, &val)) != NULL) {
                        list_push_len(arguments, key.ptr, key.len, 0, val.ptr, val.len, NULL);
                    }
                    rc = mympd_api_script_start(config, p_charbuf1, arguments, true);
                    if (rc == true) {
                        response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "script");
                    }
                    else {
                        response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                            "script", "error", "Can't create mympd_script thread");
                    }
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "script", "error", "Invalid script name");
                }
            } 
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                    "script", "error", "Scripting is disabled");
            }
            break;
        case MYMPD_API_SCRIPT_POST_EXECUTE:
            if (config->remotescripting == true) {
                je = json_scanf(request->data, sdslen(request->data), "{params: {script: %Q}}", &p_charbuf1);
                if (je == 1 && strlen(p_charbuf1) > 0) {
                    struct list *arguments = (struct list *) malloc(sizeof(struct list));
                    assert(arguments);
                    list_init(arguments);
                    void *h = NULL;
                    struct json_token key;
                    struct json_token val;
                    while ((h = json_next_key(request->data, sdslen(request->data), h, ".params.arguments", &key, &val)) != NULL) {
                        list_push_len(arguments, key.ptr, key.len, 0, val.ptr, val.len, NULL);
                    }
                    rc = mympd_api_script_start(config, p_charbuf1, arguments, false);
                    if (rc == true) {
                        response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "script");
                    }
                    else {
                        response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                            "script", "error", "Can't create mympd_script thread");
                    }
                }
            } 
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                    "script", "error", "Remote scripting is disabled");
            }
            break;
        #endif
        case MYMPD_API_SYSCMD:
            if (config->syscmds == true) {
                je = json_scanf(request->data, sdslen(request->data), "{params: {cmd: %Q}}", &p_charbuf1);
                if (je == 1) {
                    response->data = mympd_api_syscmd(config, response->data, request->method, request->id, p_charbuf1);
                }
            } 
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                    "script", "error", "System commands are disabled");
            }
            break;
        case MYMPD_API_COLS_SAVE: {
            sds cols = sdsnewlen("[", 1);
            je = json_scanf(request->data, sdslen(request->data), "{params: {table: %Q}}", &p_charbuf1);
            if (je == 1) {
                bool error = false;
                cols = json_to_cols(cols, request->data, sdslen(request->data), &error);
                if (error == false) {
                    cols = sdscatlen(cols, "]", 1);
                    if (mympd_api_cols_save(config, mympd_state, p_charbuf1, cols)) {
                        response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
                    }
                    else {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, true,
                            "general", "error", "Unknown table %{table}", 2, "table", p_charbuf1);
                        LOG_ERROR("MYMPD_API_COLS_SAVE: Unknown table %s", p_charbuf1);
                    }
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "general", "error", "Invalid column");
                }
            }
            sdsfree(cols);
            break;
        }
        case MYMPD_API_SETTINGS_RESET:
            //TODO: error checking
            mympd_api_settings_reset(config, mympd_state);
            response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
            break;
        case MYMPD_API_SETTINGS_SET: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            rc = true;
            while ((h = json_next_key(request->data, sdslen(request->data), h, ".params", &key, &val)) != NULL) {
                rc = mympd_api_settings_set(config, mympd_state, &key, &val);
                if (rc == false) {
                    break;
                }
            }
            if (rc == true) {
                //forward request to mpd_client queue            
                t_work_request *mpd_client_request = create_request(-1, request->id, request->cmd_id, request->method, request->data);
                tiny_queue_push(mpd_client_queue, mpd_client_request, 0);
                //forward request to mpd_worker queue            
                t_work_request *mpd_client_request2 = create_request(-1, request->id, request->cmd_id, request->method, request->data);
                tiny_queue_push(mpd_worker_queue, mpd_client_request2, 0);
                //respond with ok
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
            }
            else {
                sds value = sdsnewlen(key.ptr, key.len);
                response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, true,
                    "general", "error", "Can't save setting %{setting}", 2, "setting", value);
                sdsfree(value);
            }
            break;
        }
        case MYMPD_API_SETTINGS_GET:
            response->data = mympd_api_settings_put(config, mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_CONNECTION_SAVE: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            rc = true;
            while ((h = json_next_key(request->data, sdslen(request->data), h, ".params", &key, &val)) != NULL) {
                rc = mympd_api_connection_save(config, mympd_state, &key, &val);
                if (rc == false) {
                    break;
                }
            }
            if (rc == true) {
                //push settings to mpd_client queue
                mympd_api_push_to_mpd_client(mympd_state);
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
            }
            else {
                sds value = sdsnewlen(key.ptr, key.len);
                response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, true,
                    "general", "error", "Can't save setting %{setting}", 2, "setting", value);
                sdsfree(value);
            }
            break;
        }
        case MYMPD_API_BOOKMARK_SAVE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {id: %d, name: %Q, uri: %Q, type: %Q}}", 
                &int_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 4) {
                if (mympd_api_bookmark_update(config, int_buf1, p_charbuf1, p_charbuf2, p_charbuf3)) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "general", "error", "Saving bookmark failed");
                }
            }
            break;
        case MYMPD_API_BOOKMARK_RM:
            je = json_scanf(request->data, sdslen(request->data), "{params: {id: %d}}", &int_buf1);
            if (je == 1) {
                if (mympd_api_bookmark_update(config, int_buf1, NULL, NULL, NULL)) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "general", "error", "Deleting bookmark failed");
                }
            }
            break;
        case MYMPD_API_BOOKMARK_CLEAR:
            if (mympd_api_bookmark_clear(config)) {
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "general", "error", "Clearing bookmarks failed");
            }
            break;
        case MYMPD_API_BOOKMARK_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u}}", &uint_buf1);
            if (je == 1) {
                response->data = mympd_api_bookmark_list(config, response->data, request->method, request->id, uint_buf1);
            }
            break;
        case MYMPD_API_COVERCACHE_CROP:
            //TODO: error checking
            clear_covercache(config, -1);
            response->data = jsonrpc_respond_message(response->data, request->method, request->id, false, 
                        "general", "info", "Successfully croped covercache");
            break;
        case MYMPD_API_COVERCACHE_CLEAR:
            clear_covercache(config, 0);
            //TODO: error checking
            response->data = jsonrpc_respond_message(response->data, request->method, request->id, false, 
                        "general", "info", "Successfully cleared covercache");
            break;
        case MYMPD_API_TIMER_SET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {timeout: %d, interval: %d, handler: %Q}}", &int_buf1, &int_buf2, &p_charbuf1);
            if (je == 3) {
                bool handled = false;
                if (strcmp(p_charbuf1, "timer_handler_smartpls_update") == 0) {
                    replace_timer(&mympd_state->timer_list, int_buf1, int_buf2, timer_handler_smartpls_update, 2, NULL, NULL);
                    handled = true;
                }
                if (handled == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "timer");
                }
            }
            break;
        case MYMPD_API_TIMER_SAVE: {
            struct t_timer_definition *timer_def = malloc(sizeof(struct t_timer_definition));
            assert(timer_def);
            timer_def = parse_timer(timer_def, request->data, sdslen(request->data));
            je = json_scanf(request->data, sdslen(request->data), "{params: {timerid: %d, interval: %d}}", &int_buf1, &int_buf2);
            if (je == 2 && timer_def != NULL) {
                if (int_buf1 == 0) {
                    mympd_state->timer_list.last_id++;
                    int_buf1 = mympd_state->timer_list.last_id;
                }
                time_t start = timer_calc_starttime(timer_def->start_hour, timer_def->start_minute);
                rc = replace_timer(&mympd_state->timer_list, start, int_buf2, timer_handler_select, int_buf1, timer_def, NULL);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "timer");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true,
                        "timer", "error", "Adding timer failed");
                    free_timer_definition(timer_def);
                }
            }
            else if (timer_def != NULL) {
                LOG_ERROR("No timer id received, discarding timer definition");
                free_timer_definition(timer_def);
            }
            break;
        }
        case MYMPD_API_TIMER_LIST:
            response->data = timer_list(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_TIMER_GET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {timerid: %d}}", &int_buf1);
            if (je == 1) {
                response->data = timer_get(mympd_state, response->data, request->method, request->id, int_buf1);
            }
            break;
        case MYMPD_API_TIMER_RM:
            je = json_scanf(request->data, sdslen(request->data), "{params: {timerid: %d}}", &int_buf1);
            if (je == 1) {
                remove_timer(&mympd_state->timer_list, int_buf1);
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "timer");
            }
            break;
        case MYMPD_API_TIMER_TOGGLE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {timerid: %d}}", &int_buf1);
            if (je == 1) {
                toggle_timer(&mympd_state->timer_list, int_buf1);
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "timer");
            }
            break;
        default:
            response->data = jsonrpc_respond_message(response->data, request->method, request->id, true,
                "general", "error", "Unknown request");
            LOG_ERROR("Unknown API request: %.*s", sdslen(request->data), request->data);
    }

    FREE_PTR(p_charbuf1);
    FREE_PTR(p_charbuf2);
    FREE_PTR(p_charbuf3);
    FREE_PTR(p_charbuf4);
    FREE_PTR(p_charbuf5);

    #ifdef DEBUG
    MEASURE_END
    MEASURE_PRINT(request->method)
    #endif

    if (sdslen(response->data) == 0) {
        response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, true, 
            "general", "error", "No response for method %{method}", 2, "method", request->method);
        LOG_ERROR("No response for method \"%s\"", request->method);
    }
    if (request->conn_id == -2) {
        LOG_DEBUG("Push response to mympd_script_queue for thread %ld: %s", request->id, response->data);
        tiny_queue_push(mympd_script_queue, response, request->id);
    }
    else if (request->conn_id > -1) {
        LOG_DEBUG("Push response to web_server_queue for connection %lu: %s", request->conn_id, response->data);
        tiny_queue_push(web_server_queue, response, 0);
    }
    else {
        free_result(response);
    }
    free_request(request);
}
