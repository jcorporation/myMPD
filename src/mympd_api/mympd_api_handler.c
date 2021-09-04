/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_handler.h"

#include "../lib/api.h"
#include "../lib/covercache.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/lua_mympd_state.h"
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_client/mpd_client_browse.h"
#include "../mpd_client/mpd_client_cover.h" 
#include "../mpd_client/mpd_client_features.h"
#include "../mpd_client/mpd_client_jukebox.h"
#include "../mpd_client/mpd_client_lyrics.h"
#include "../mpd_client/mpd_client_mounts.h"
#include "../mpd_client/mpd_client_partitions.h"
#include "../mpd_client/mpd_client_playlists.h"
#include "../mpd_client/mpd_client_queue.h"
#include "../mpd_client/mpd_client_state.h"
#include "../mpd_client/mpd_client_stats.h"
#include "../mpd_client/mpd_client_sticker.h"
#include "../mpd_client/mpd_client_timer.h"
#include "../mpd_client/mpd_client_trigger.h"
#include "../mpd_client/mpd_client_utility.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_playlists.h"
#include "../mpd_shared/mpd_shared_search.h"
#include "../mpd_shared/mpd_shared_sticker.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_worker.h"
#include "mympd_api_home.h"
#include "mympd_api_scripts.h"
#include "mympd_api_settings.h"
#include "mympd_api_timer.h"
#include "mympd_api_timer_handlers.h"
#include "mympd_api_utility.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void mympd_api_handler(struct t_mympd_state *mympd_state, void *arg_request) {
    t_work_request *request = (t_work_request*) arg_request;
    unsigned int uint_buf1;
    unsigned int uint_buf2;
    int int_buf1;
    int int_buf2; 
    bool bool_buf1;
    bool bool_buf2;
    bool rc;
    sds sds_buf1 = NULL;
    sds sds_buf2 = NULL;
    sds sds_buf3 = NULL;
    sds sds_buf4 = NULL;
    sds sds_buf5 = NULL;
    sds sds_buf6 = NULL;
    sds error = sdsempty();
    bool async = false;
    
    #ifdef DEBUG
    MEASURE_START
    #endif

    MYMPD_LOG_INFO("MYMPD API request (%lld)(%ld) %s: %s", request->conn_id, request->id, request->method, request->data);
    //create response struct
    t_work_result *response = create_result(request);
    
    switch(request->cmd_id) {
        case MYMPD_API_SMARTPLS_UPDATE_ALL:
        case MYMPD_API_SMARTPLS_UPDATE:
        case INTERNAL_API_CACHES_CREATE:
            if (worker_threads > 5) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                            "general", "error", "Too many worker threads are already running");
                break;
            }
            async = true;
            free_result(response);
            mpd_worker_start(mympd_state, request);
            break;
        case MYMPD_API_PICTURE_LIST:
            response->data = mympd_api_picture_list(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_HOME_ICON_SAVE: {
            if (mympd_state->home_list.length == 100) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "home", "error", "Too many home icons");
                break;
            }
            struct list options;
            list_init(&options);
            if (json_get_bool(request->data, "$.params.replace", &bool_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.oldpos", 0, 99, &uint_buf1, &error) == true &&
                json_get_string_max(request->data, "$.params.name", &sds_buf1, vcb_isname, &error) == true &&
                json_get_string_max(request->data, "$.params.ligature", &sds_buf2, vcb_isalnum, &error) == true &&
                json_get_string_max(request->data, "$.params.bgcolor", &sds_buf3, vcb_ishexcolor, &error) == true &&
                json_get_string_max(request->data, "$.params.color", &sds_buf4, vcb_ishexcolor, &error) == true &&
                json_get_string_max(request->data, "$.params.image", &sds_buf5, vcb_isfilepath, &error) == true &&
                json_get_string_max(request->data, "$.params.cmd", &sds_buf6, vcb_isalnum, &error) == true &&
                json_get_array_string(request->data, "$.params.options", &options, vcb_isname, 10, &error) == true)
            {
                rc = mympd_api_save_home_icon(mympd_state, bool_buf1, uint_buf1, sds_buf1, sds_buf2, sds_buf3, sds_buf4, sds_buf5, sds_buf6, &options);                
                if (rc == true) {
                    response->data = mympd_api_put_home_list(mympd_state, response->data, request->method, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "home", "error", "Can not save home icon");
                }
            }
            list_free(&options);
            break;
        }
        case MYMPD_API_HOME_ICON_MOVE:
            if (json_get_uint(request->data, "$.params.from", 0, 99, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, 99, &uint_buf2, &error) == true)
            {
                rc = mympd_api_move_home_icon(mympd_state, uint_buf1, uint_buf2);
                if (rc == true) {
                    response->data = mympd_api_put_home_list(mympd_state, response->data, request->method, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "home", "error", "Can not move home icon");
                }
            }
            break;
        case MYMPD_API_HOME_ICON_RM:
            if (json_get_uint(request->data, "$.params.pos", 0, 99, &uint_buf1, &error) == true) {
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
            if (json_get_uint(request->data, "$.params.pos", 0, 99, &uint_buf1, &error) == true) {
                response->data = mympd_api_get_home_icon(mympd_state, response->data, request->method, request->id, uint_buf1);
            }
            break;
        case MYMPD_API_HOME_LIST:
            response->data = mympd_api_put_home_list(mympd_state, response->data, request->method, request->id);
            break;
        #ifdef ENABLE_LUA
        case MYMPD_API_SCRIPT_SAVE: {
            struct list arguments;
            list_init(&arguments);
            if (json_get_string(request->data, "$.params.script", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.oldscript", 0, 200, &sds_buf2, vcb_isfilename, &error) == true &&
                json_get_int(request->data, "$.params.order", 0, 99, &int_buf1, &error) == true &&
                json_get_string(request->data, "$.params.content", 0, 2000, &sds_buf3, vcb_istext, &error) == true &&
                json_get_array_string(request->data, "$.params.arguments", &arguments, vcb_isalnum, 10, &error) == true)
            {
                rc = mympd_api_script_save(mympd_state->config, sds_buf1, sds_buf2, int_buf1, sds_buf3, &arguments);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "script");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "script", "error", "Could not save script");
                }
            }
            list_free(&arguments);
            break;
        }
        case MYMPD_API_SCRIPT_RM:
            if (json_get_string(request->data, "$.params.script", 1, 200, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mympd_api_script_delete(mympd_state->config, sds_buf1);
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
            if (json_get_string(request->data, "$.params.script", 1, 200, &sds_buf1, vcb_isfilename, &error) == true) {
                response->data = mympd_api_script_get(mympd_state->config, response->data, request->method, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_SCRIPT_LIST: {
            if (json_get_bool(request->data, "$.params.all", &bool_buf1, &error) == true) {
                response->data = mympd_api_script_list(mympd_state->config, response->data, request->method, request->id, bool_buf1);
            }
            break;
        }
        case MYMPD_API_SCRIPT_EXECUTE: {
            //malloc list - it is used in another thread
            struct list *arguments = (struct list *) malloc(sizeof(struct list));
            assert(arguments);
            list_init(arguments);
            if (json_get_string(request->data, "$.params.script", 1, 200, &sds_buf1, vcb_isfilename, &error) == true && 
                json_get_object_string(request->data, "$.params.arguments", arguments, vcb_isname, 10, &error) == true)
            {
                rc = mympd_api_script_start(mympd_state->config, sds_buf1, arguments, true);
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
                list_free(arguments);
                FREE_PTR(arguments);
            }
            break;
        }
        case INTERNAL_API_SCRIPT_POST_EXECUTE: {
            //malloc list - it is used in another thread
            struct list *arguments = (struct list *) malloc(sizeof(struct list));
            assert(arguments);
            list_init(arguments);
            if (json_get_string(request->data, "$.params.script", 1, 200, &sds_buf1, vcb_istext, &error) == true &&
                json_get_object_string(request->data, "$.params.arguments", arguments, vcb_isname, 10, &error) == true)
            {
                rc = mympd_api_script_start(mympd_state->config, sds_buf1, arguments, false);
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
                    "script", "error", "Invalid script content");
                list_free(arguments);
                FREE_PTR(arguments);
            }
            break;
        }
        #endif
        case MYMPD_API_COLS_SAVE: {
            if (json_get_string(request->data, "$.params.table", 1, 200, &sds_buf1, vcb_isalnum, &error) == true) {
                bool rc_error = false;
                sds cols = sdsnewlen("[", 1);
                cols = json_to_cols(cols, request->data, &rc_error);
                if (rc_error == false) {
                    cols = sdscatlen(cols, "]", 1);
                    if (mympd_api_cols_save(mympd_state, sds_buf1, cols)) {
                        response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
                    }
                    else {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, true,
                            "general", "error", "Unknown table %{table}", 2, "table", sds_buf1);
                        MYMPD_LOG_ERROR("MYMPD_API_COLS_SAVE: Unknown table %s", sds_buf1);
                    }
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "general", "error", "Invalid column");
                }
                FREE_SDS(cols);
            }
            break;
        }
        case MYMPD_API_SETTINGS_SET: {
            if (json_iterate_object(request->data, "$.params", mympd_api_settings_set, mympd_state, NULL, 1000, &error) == true) {
                if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                    //feature detection
                    mpd_client_mpd_features(mympd_state);
                }
                //respond with ok
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true,
                    "general", "error", "Can't save settings");
            }
            break;
        }
        case MYMPD_API_PLAYER_OPTIONS_SET: {
            if (json_iterate_object(request->data, "$.params", mpdclient_api_options_set, mympd_state, NULL, 100, &error) == true) {
                if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                    //feature detection
                    mpd_client_mpd_features(mympd_state);
                    if (mympd_state->jukebox_mode != JUKEBOX_OFF) {
                        //enable jukebox
                        mpd_client_jukebox(mympd_state, 0);
                    }
                }
                //respond with ok
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true,
                    "general", "error", "Can't save settings");
            }
            break;
        }
        case MYMPD_API_SETTINGS_GET:
            response->data = mympd_api_settings_put(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_CONNECTION_SAVE: {
            sds old_mpd_settings = sdscatprintf(sdsempty(), "%s%d%s", mympd_state->mpd_state->mpd_host, mympd_state->mpd_state->mpd_port, mympd_state->mpd_state->mpd_pass);

            if (json_iterate_object(request->data, "$.params", mympd_api_connection_save, mympd_state, NULL, 100, &error) == true) {
                sds new_mpd_settings = sdscatprintf(sdsempty(), "%s%d%s", mympd_state->mpd_state->mpd_host, mympd_state->mpd_state->mpd_port, mympd_state->mpd_state->mpd_pass);
                if (strcmp(old_mpd_settings, new_mpd_settings) != 0) {
                    //reconnect to new mpd
                    MYMPD_LOG_DEBUG("MPD host has changed, disconnecting");
                    mympd_state->mpd_state->conn_state = MPD_DISCONNECT_INSTANT;
                }
                else if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                    //feature detection
                    mpd_client_mpd_features(mympd_state);
                }
                FREE_SDS(new_mpd_settings);
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true,
                    "mpd", "error", "Can't save settings");
            }
            FREE_SDS(old_mpd_settings);
            break;
        }
        case MYMPD_API_COVERCACHE_CROP:
            int_buf1 = clear_covercache(mympd_state->config->workdir, mympd_state->covercache_keep_days);
            if (int_buf1 >= 0) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, false, 
                            "general", "info", "Successfully croped covercache");
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, false, 
                            "general", "error", "Error in cropping covercache");
            }
            break;
        case MYMPD_API_COVERCACHE_CLEAR:
            int_buf1 = clear_covercache(mympd_state->config->workdir, 0);
            if (int_buf1 >= 0) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, false, 
                            "general", "info", "Successfully cleared covercache");
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, false, 
                            "general", "error", "Error in clearing the covercache");
            }
            break;
        case MYMPD_API_TIMER_SAVE: {
            struct t_timer_definition *timer_def = malloc(sizeof(struct t_timer_definition));
            assert(timer_def);
            timer_def = parse_timer(timer_def, request->data, &error);
            if (timer_def != NULL &&
                json_get_int(request->data, "$.params.interval", -1, 604800, &int_buf2, &error) == true &&
                json_get_int(request->data, "$.params.timerid", 0, 200, &int_buf1, &error) == true)
            {
                if (int_buf1 == 0) {
                    mympd_state->timer_list.last_id++;
                    int_buf1 = mympd_state->timer_list.last_id;
                }
                else if (int_buf1 <= 100) {
                    //user defined timers must be gt 100
                    MYMPD_LOG_ERROR("Timer id must be greater than 100, but id is: \"%d\"", int_buf1);
                    error = sdscat(error, "Invalid timer id");
                    break;
                }
                time_t start = timer_calc_starttime(timer_def->start_hour, timer_def->start_minute, int_buf2);
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
                MYMPD_LOG_ERROR("No timer id or interval, discarding timer definition");
                free_timer_definition(timer_def);
            }
            break;
        }
        case MYMPD_API_TIMER_LIST:
            response->data = timer_list(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_TIMER_GET:
            if (json_get_int(request->data, "$.params.timerid", 100, 200, &int_buf1, &error) == true) {
                response->data = timer_get(mympd_state, response->data, request->method, request->id, int_buf1);
            }
            break;
        case MYMPD_API_TIMER_RM:
            if (json_get_int(request->data, "$.params.timerid", 100, 200, &int_buf1, &error) == true) {
                remove_timer(&mympd_state->timer_list, int_buf1);
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "timer");
            }
            break;
        case MYMPD_API_TIMER_TOGGLE:
            if (json_get_int(request->data, "$.params.timerid", 100, 200, &int_buf1, &error) == true) {
                toggle_timer(&mympd_state->timer_list, int_buf1);
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "timer");
            }
            break;
        case MYMPD_API_LYRICS_GET:
            if (json_get_string(request->data, "$.params.uri", 1, 200, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mpd_client_lyrics_get(mympd_state, response->data, request->method, request->id, sds_buf1);
            }
            break;
        case INTERNAL_API_STATE_SAVE:
            mpd_client_last_played_list_save(mympd_state);
            triggerfile_save(mympd_state);
            mympd_api_write_home_list(mympd_state);
            timerfile_save(mympd_state);
            response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
            break;
        case MYMPD_API_JUKEBOX_RM:
            if (json_get_uint_max(request->data, "$.params.pos", &uint_buf1, &error) == true) {
                rc = mpd_client_rm_jukebox_entry(mympd_state, uint_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "jukebox");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "jukebox", "error", "Could not remove song from jukebox queue");
                }
            }
            break;
        case MYMPD_API_JUKEBOX_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);          
            if (json_get_uint_max(request->data, "$.params.offset", &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.limit", 0, MAX_MPD_RESULTS, &uint_buf2, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, 20, &error) == true)
            {
                response->data = mpd_client_put_jukebox_list(mympd_state, response->data, request->method, request->id, uint_buf1, uint_buf2, &tagcols);
            }
            break;
        }
        case MYMPD_API_TRIGGER_LIST:
            response->data = trigger_list(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_TRIGGER_GET:
            if (json_get_uint(request->data, "$.params.id", 0, 99, &uint_buf1, &error) == true) {
                response->data = trigger_get(mympd_state, response->data, request->method, request->id, uint_buf1);
            }
            break;
        case MYMPD_API_TRIGGER_SAVE: {
            struct list *arguments = (struct list *) malloc(sizeof(struct list));
            assert(arguments);
            list_init(arguments);

            if (json_get_string(request->data, "$.params.name", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.script", 0, 200, &sds_buf2, vcb_isfilename, &error) == true &&
                json_get_int(request->data, "$.params.id", -1, 99, &int_buf1, &error) == true &&
                json_get_int_max(request->data, "$.params.event", &int_buf2, &error) == true &&
                json_get_object_string(request->data, "$.params.arguments", arguments, vcb_isname, 10, &error) == true)
            {
                rc = list_push(&mympd_state->triggers, sds_buf1, int_buf2, sds_buf2, arguments);
                if (rc == true) {
                    if (int_buf1 >= 0) {
                        //delete old entry
                        rc = delete_trigger(mympd_state, int_buf1);
                    }
                    if (rc == true) {
                        response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "trigger");
                        break;
                    }
                }
                else {
                    list_free(arguments);
                }
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "trigger", "error", "Could not save trigger");
            }
            else {
                list_free(arguments);
            }
            break;
        }
        case MYMPD_API_TRIGGER_RM:
            if (json_get_uint(request->data, "$.params.id", 0, 99, &uint_buf1, &error) == true) {
                rc = delete_trigger(mympd_state, uint_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "trigger");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "trigger", "error", "Could not delete trigger");
                }
            }
            break;
        case INTERNAL_API_SCRIPT_INIT: {
            struct list *lua_mympd_state = (struct list *) malloc(sizeof(struct list));
            assert(lua_mympd_state);
            list_init(lua_mympd_state);
            rc = mpd_client_get_lua_mympd_state(mympd_state, lua_mympd_state);
            if (rc == true) {
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "script");
                response->extra = lua_mympd_state;
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                    "script", "error", "Error getting mympd state for script execution");
                MYMPD_LOG_ERROR("Error getting mympd state for script execution");
                free(lua_mympd_state);
            }
            break;
        }
        case MYMPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET: {
            struct list attributes;
            list_init(&attributes);
            if (json_get_uint(request->data, "$.params.outputId", 0, 20, &uint_buf1, &error) == true &&
                json_get_object_string(request->data, "$.params.attributes", &attributes, vcb_isname, 10, &error) == true)
            {
                struct list_node *current = attributes.head;
                while (current != NULL) {
                    rc = mpd_run_output_set(mympd_state->mpd_state->conn, uint_buf1, current->key, current->value_p);
                    response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_output_set");
                    if (rc == false) {
                        break;
                    }
                    current = current->next;
                }
            }
            list_free(&attributes);
            break;
        }
        case INTERNAL_API_STICKERCACHE_CREATED:
            sticker_cache_free(&mympd_state->sticker_cache);
            if (request->extra != NULL) {
                mympd_state->sticker_cache = (rax *) request->extra;
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "sticker");
                MYMPD_LOG_INFO("Sticker cache was replaced");
            }
            else {
                MYMPD_LOG_ERROR("Sticker cache is NULL");
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "sticker", "error", "Sticker cache is NULL");
            }
            mympd_state->sticker_cache_building = false;
            break;
        case INTERNAL_API_ALBUMCACHE_CREATED:
            album_cache_free(&mympd_state->album_cache);
            if (request->extra != NULL) {
                mympd_state->album_cache = (rax *) request->extra;
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "database");
                MYMPD_LOG_INFO("Album cache was replaced");
            }
            else {
                MYMPD_LOG_ERROR("Album cache is NULL");
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "database", "error", "Album cache is NULL");
            }
            mympd_state->album_cache_building = false;
            break;
        case MYMPD_API_MESSAGE_SEND:
            if (json_get_string(request->data, "$.params.channel", 1, 200, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.message", 1, 2000, &sds_buf2, vcb_isname, &error) == true)
            {
                uint_buf1 = mpd_run_send_message(mympd_state->mpd_state->conn, sds_buf1, sds_buf2);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, (uint_buf1 == 0 ? false : true), "mpd_run_send_message");
            }
            break;
        case MYMPD_API_LIKE:
            if (mympd_state->mpd_state->feat_stickers == false) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "sticker", "error", "MPD stickers are disabled");
                MYMPD_LOG_ERROR("MPD stickers are disabled");
                break;
            }
            if (json_get_string(request->data, "$.params.uri", 1, 200, &sds_buf1, vcb_isfilepath, &error) == true &&
                json_get_int(request->data, "$.params.like", 0, 2, &int_buf1, &error) == true)
            {
                rc = mpd_client_sticker_like(mympd_state, sds_buf1, int_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "sticker");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "sticker", "error", "Failed to set like, unknown error");
                }
            }
            break;
        case MYMPD_API_PLAYER_STATE:
            response->data = mpd_client_put_state(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_PLAYER_CLEARERROR:
            rc = mpd_run_clearerror(mympd_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_clearerror");
            break;
        case MYMPD_API_DATABASE_UPDATE:
        case MYMPD_API_DATABASE_RESCAN: {
            long update_id = mpd_client_get_updatedb_id(mympd_state);
            if (update_id == -1) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "database", "error", "Error getting MPD status");
                break;
            }
            if (update_id > 0) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, false, "database", "info", "Database update already startet");
                break;
            }
            if (json_get_string(request->data, "$.params.uri", 0, 200, &sds_buf1, vcb_isfilepath, &error) == true) {
                if (sdslen(sds_buf1) == 0) {
                    //path should be NULL to scan root directory
                    FREE_SDS(sds_buf1);
                    sds_buf1 = NULL;
                }
                if (request->cmd_id == MYMPD_API_DATABASE_UPDATE) {
                    uint_buf1 = mpd_run_update(mympd_state->mpd_state->conn, sds_buf1);
                    response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, (uint_buf1 == 0 ? false : true), "mpd_run_update");
                }
                else {
                    uint_buf1 = mpd_run_rescan(mympd_state->mpd_state->conn, sds_buf1);
                    response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, (uint_buf1 == 0 ? false : true), "mpd_run_rescan");
                }
            }
            break;
        }
        case MYMPD_API_SMARTPLS_STICKER_SAVE:
        case MYMPD_API_SMARTPLS_NEWEST_SAVE:
        case MYMPD_API_SMARTPLS_SEARCH_SAVE:
            if (mympd_state->mpd_state->feat_smartpls == false) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "playlist", "error", "Smart playlists are not supported");
                break;
            }
            rc = false;
            if (request->cmd_id == MYMPD_API_SMARTPLS_STICKER_SAVE) {
                if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                    json_get_string(request->data, "$.params.sticker", 1, 200, &sds_buf2, vcb_isalnum, &error) == true &&
                    json_get_int(request->data, "$.params.maxentries", 0, MAX_MPD_PLAYLIST_LENGTH, &int_buf1, &error) == true &&
                    json_get_int(request->data, "$.params.minvalue", 0, 100, &int_buf2, &error) == true &&
                    json_get_string(request->data, "$.params.sort", 0, 100, &sds_buf3, vcb_ismpdsort, &error) == true)
                {
                    rc = mpd_shared_smartpls_save(mympd_state->config->workdir, "sticker", sds_buf1, sds_buf2, int_buf1, int_buf2, sds_buf3);
                }
            }
            else if (request->cmd_id == MYMPD_API_SMARTPLS_NEWEST_SAVE) {
                if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                    json_get_int(request->data, "$.params.timerange", 0, JSONRPC_INT_MAX, &int_buf1, &error) == true &&
                    json_get_string(request->data, "$.params.sort", 0, 100, &sds_buf2, vcb_ismpdsort, &error) == true)
                {
                    rc = mpd_shared_smartpls_save(mympd_state->config->workdir, "newest", sds_buf1, NULL, 0, int_buf1, sds_buf2);
                }
            }            
            else if (request->cmd_id == MYMPD_API_SMARTPLS_SEARCH_SAVE) {
                if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                    json_get_string(request->data, "$.params.expression", 1, 200, &sds_buf2, vcb_isname, &error) == true &&
                    json_get_string(request->data, "$.params.sort", 0, 100, &sds_buf3, vcb_ismpdsort, &error) == true)
                {
                    rc = mpd_shared_smartpls_save(mympd_state->config->workdir, "search", sds_buf1, sds_buf2, 0, 0, sds_buf3);
                }
            }
            if (rc == true) {
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "playlist");
                mpd_client_smartpls_update(sds_buf1);
            }
            break;
        case MYMPD_API_SMARTPLS_GET:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true) {
                response->data = mpd_client_smartpls_put(mympd_state->config, response->data, request->method, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_PLAYER_PAUSE:
            rc = mpd_run_pause(mympd_state->mpd_state->conn, true);
            response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_pause");
            break;
        case MYMPD_API_PLAYER_RESUME:
            rc = mpd_run_pause(mympd_state->mpd_state->conn, false);
            response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_pause");
            break;
        case MYMPD_API_PLAYER_PREV:
            rc = mpd_run_previous(mympd_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_previous");
            break;
        case MYMPD_API_PLAYER_NEXT:
            rc = mpd_run_next(mympd_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_next");
            break;
        case MYMPD_API_PLAYER_PLAY:
            rc = mpd_run_play(mympd_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_play");
            break;
        case MYMPD_API_PLAYER_STOP:
            rc = mpd_run_stop(mympd_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_stop");
            break;
        case MYMPD_API_QUEUE_CLEAR:
            rc = mpd_run_clear(mympd_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_clear");
            break;
        case MYMPD_API_QUEUE_CROP:
            response->data = mpd_client_crop_queue(mympd_state, response->data, request->method, request->id, false);
            break;
        case MYMPD_API_QUEUE_CROP_OR_CLEAR:
            response->data = mpd_client_crop_queue(mympd_state, response->data, request->method, request->id, true);
            break;
        case MYMPD_API_QUEUE_RM_SONG:
            if (json_get_uint_max(request->data, "$.params.songId", &uint_buf1, &error) == true) {
                rc = mpd_run_delete_id(mympd_state->mpd_state->conn, uint_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_delete_id");
            }
            break;
        case MYMPD_API_QUEUE_RM_RANGE:
            if (json_get_uint(request->data, "$.params.start", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.end", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf2, &error) == true)
            {
                rc = mpd_run_delete_range(mympd_state->mpd_state->conn, uint_buf1, uint_buf2);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_delete_range");
            }
            break;
        case MYMPD_API_QUEUE_MOVE_SONG:
            if (json_get_uint(request->data, "$.params.from", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf2, &error) == true)
            {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2) {
                    uint_buf2--;
                }
                rc = mpd_run_move(mympd_state->mpd_state->conn, uint_buf1, uint_buf2);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_move");
            }
            break;
        case MYMPD_API_QUEUE_PRIO_SET_HIGHEST:
            if (json_get_uint_max(request->data, "$.params.songId", &uint_buf1, &error) == true) {
                rc = mpd_client_queue_prio_set_highest(mympd_state, uint_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "queue");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "queue", "error", "Failed to set song priority");
                }
            }
            break;
        case MYMPD_API_PLAYLIST_MOVE_SONG:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.from", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf2, &error) == true)
            {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2) {
                    uint_buf2--;
                }
                rc = mpd_run_playlist_move(mympd_state->mpd_state->conn, sds_buf1, uint_buf1, uint_buf2);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_move");
            }
            break;
        case MYMPD_API_PLAYER_PLAY_SONG:
            if (json_get_uint_max(request->data, "$.params.songId", &uint_buf1, &error) == true) {
                rc = mpd_run_play_id(mympd_state->mpd_state->conn, uint_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_play_id");
            }
            break;
        case MYMPD_API_PLAYER_OUTPUT_LIST:
            if (json_get_string(request->data, "$.params.partition", 0, 200, &sds_buf1, vcb_isname, &error) == true) {
                if (sdslen(sds_buf1) == 0) {
                    response->data = mpd_client_put_partition_outputs(mympd_state, response->data, request->method, request->id, sds_buf1);
                }
                else {
                    response->data = mpd_client_put_outputs(mympd_state, response->data, request->method, request->id);
                }
            }
            break;
        case MYMPD_API_PLAYER_OUTPUT_TOGGLE:
            if (json_get_uint(request->data, "$.params.outputId", 0, 20, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.state", 0, 1, &uint_buf2, &error) == true) 
            {
                if (uint_buf2 == 1) {
                    rc = mpd_run_enable_output(mympd_state->mpd_state->conn, uint_buf1);
                    response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_enable_output");
                }
                else {
                    rc = mpd_run_disable_output(mympd_state->mpd_state->conn, uint_buf1);
                    response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_disable_output");
                }
            }
            break;
        case MYMPD_API_PLAYER_VOLUME_SET:
            if (json_get_uint(request->data, "$.params.volume", 0, 100, &uint_buf1, &error) == true) {
                if (uint_buf1 > mympd_state->volume_max || uint_buf1 < mympd_state->volume_min) {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "player", "error", "Invalid volume level");
                }
                else {
                    rc = mpd_run_set_volume(mympd_state->mpd_state->conn, uint_buf1);
                    response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_set_volume");
                }
            }
            break;
        case MYMPD_API_PLAYER_VOLUME_GET:
            response->data = mpd_client_put_volume(mympd_state, response->data, request->method, request->id);
            break;            
        case MYMPD_API_PLAYER_SEEK_CURRENT:
            if (json_get_int_max(request->data, "$.params.seek", &int_buf1, &error) == true &&
                json_get_bool(request->data, "$.params.relative", &bool_buf1, &error) == true)
            {
                rc = mpd_run_seek_current(mympd_state->mpd_state->conn, (float)int_buf1, bool_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_seek_current");
            }
            break;
        case MYMPD_API_QUEUE_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);          
            if (json_get_uint_max(request->data, "$.params.offset", &uint_buf1, &error) == true &&
                json_get_uint_max(request->data, "$.params.limit", &uint_buf2, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, 20, &error) == true)
            {
                response->data = mpd_client_put_queue(mympd_state, response->data, request->method, request->id, uint_buf1, uint_buf2, &tagcols);
            }
            break;
        }
        case MYMPD_API_QUEUE_LAST_PLAYED: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);          
            if (json_get_uint_max(request->data, "$.params.offset", &uint_buf1, &error) == true &&
                json_get_uint_max(request->data, "$.params.limit", &uint_buf2, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, 20, &error) == true)
            {
                response->data = mpd_client_put_last_played_songs(mympd_state, response->data, request->method, request->id, uint_buf1, uint_buf2, &tagcols);
            }
            break;
        }
        case MYMPD_API_PLAYER_CURRENT_SONG: {
            response->data = mpd_client_put_current_song(mympd_state, response->data, request->method, request->id);
            break;
        }
        case MYMPD_API_DATABASE_SONGDETAILS:
            if (json_get_string(request->data, "$.params.uri", 1, 200, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mpd_client_put_songdetails(mympd_state, response->data, request->method, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_DATABASE_COMMENTS:
            if (json_get_string(request->data, "$.params.uri", 1, 200, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mpd_client_read_comments(mympd_state, response->data, request->method, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_DATABASE_FINGERPRINT:
            if (mympd_state->mpd_state->feat_fingerprint == true) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "database", "error", "Fingerprint command not supported");
                break;
            }
            if (json_get_string(request->data, "$.params.uri", 1, 200, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mpd_client_put_fingerprint(mympd_state, response->data, request->method, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_PLAYLIST_RENAME:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.newName", 1, 200, &sds_buf2, vcb_isfilename, &error) == true)
            {
                response->data = mpd_client_playlist_rename(mympd_state, response->data, request->method, request->id, sds_buf1, sds_buf2);
            }
            break;            
        case MYMPD_API_PLAYLIST_LIST:          
            if (json_get_uint_max(request->data, "$.params.offset", &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.limit", 0, MAX_MPD_RESULTS, &uint_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, 200, &sds_buf1, vcb_isname, &error) == true)
            {
                response->data = mpd_client_put_playlists(mympd_state, response->data, request->method, request->id, uint_buf1, uint_buf2, sds_buf1);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);          
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.offset", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.limit", 0, MAX_MPD_RESULTS, &uint_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, 200, &sds_buf2, vcb_isname, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, 20, &error) == true)
            {
                response->data = mpd_client_put_playlist_list(mympd_state, response->data, request->method, request->id, sds_buf1, uint_buf1, uint_buf2, sds_buf2, &tagcols);
            }
            break;
        }
        case MYMPD_API_PLAYLIST_ADD_URI:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.uri", 1, 200, &sds_buf2, vcb_isuri, &error) == true)
            {
                rc = mpd_run_playlist_add(mympd_state->mpd_state->conn, sds_buf1, sds_buf2);
                if (check_error_and_recover2(mympd_state->mpd_state, &response->data, request->method, request->id, false) == true && rc == true) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, false, 
                        "playlist", "info", "Added %{uri} to playlist %{playlist}", 4, "uri", sds_buf2, "playlist", sds_buf1);
                }
                else if (rc == false) {
                    response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_add");
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CLEAR:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mpd_run_playlist_clear(mympd_state->mpd_state->conn, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_clear");
            }
            break;
        case MYMPD_API_PLAYLIST_RM_ALL:
            if (json_get_string(request->data, "$.params.type", 1, 200, &sds_buf1, vcb_isalnum, &error) == true) {
                response->data = mpd_client_playlist_delete_all(mympd_state, response->data, request->method, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_PLAYLIST_RM_SONG:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.pos", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf1, &error) == true)
            {
                rc = mpd_run_playlist_delete(mympd_state->mpd_state->conn, sds_buf1, uint_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_delete");
            }
            break;
        case MYMPD_API_PLAYLIST_SHUFFLE:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true) {
                response->data = mpd_shared_playlist_shuffle_sort(mympd_state->mpd_state, response->data, request->method, request->id, sds_buf1, "shuffle");
            }
            break;
        case MYMPD_API_PLAYLIST_SORT:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.tag", 1, 200, &sds_buf2, vcb_ismpdtag, &error) == true)
            {
                response->data = mpd_shared_playlist_shuffle_sort(mympd_state->mpd_state, response->data, request->method, request->id, sds_buf1, sds_buf2);
            }
            break;
        case MYMPD_API_DATABASE_FILESYSTEM_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);          
            if (json_get_uint(request->data, "$.params.offset", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.limit", 0, MAX_MPD_RESULTS, &uint_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 1, 200, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.path", 1, 200, &sds_buf2, vcb_isfilepath, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, 20, &error) == true)
            {
                response->data = mpd_client_put_filesystem(mympd_state, response->data, request->method, request->id, sds_buf2, uint_buf1, uint_buf2, sds_buf1, &tagcols);
            }
            break;
        }
        case MYMPD_API_QUEUE_ADD_URI_AFTER:
            if (json_get_string(request->data, "$.params.uri", 1, 200, &sds_buf1, vcb_isuri, &error) == true &&
                json_get_int(request->data, "$.params.to", 0, MAX_MPD_PLAYLIST_LENGTH, &int_buf1, &error) == true)
            {
                rc = mpd_run_add_id_to(mympd_state->mpd_state->conn, sds_buf1, int_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_add_id_to");
            }
            break;
        case MYMPD_API_QUEUE_REPLACE_URI:
            if (json_get_string(request->data, "$.params.uri", 1, 200, &sds_buf1, vcb_isuri, &error) == true) {
                rc = mpd_client_queue_replace_with_song(mympd_state, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_client_queue_replace_with_song");
            }
            break;
        case MYMPD_API_QUEUE_ADD_URI:
            if (json_get_string(request->data, "$.params.uri", 1, 200, &sds_buf1, vcb_isuri, &error) == true) {
                rc = mpd_run_add(mympd_state->mpd_state->conn, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_add");
            }
            break;
        case MYMPD_API_QUEUE_ADD_PLAY_URI:
            if (json_get_string(request->data, "$.params.uri", 1, 200, &sds_buf1, vcb_isuri, &error) == true) {
                int_buf1 = mpd_run_add_id(mympd_state->mpd_state->conn, sds_buf1);
                if (int_buf1 != -1) {
                    rc = mpd_run_play_id(mympd_state->mpd_state->conn, int_buf1);
                }
                else {
                    rc = false;
                }
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_add_id");
            }
            break;
        case MYMPD_API_QUEUE_REPLACE_PLAYLIST:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mpd_client_queue_replace_with_playlist(mympd_state, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_client_queue_replace_with_playlist");
            }
            break;
        case MYMPD_API_QUEUE_ADD_RANDOM:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.mode", 0, 2, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.mode", 0, 1000, &uint_buf2, &error) == true)
            {
                rc = mpd_client_jukebox_add_to_queue(mympd_state, uint_buf2, uint_buf1, sds_buf1, true);
                if (rc == true) {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, false, 
                        "queue", "info", "Successfully added random songs to queue");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "queue", "error", "Adding random songs to queue failed");
                }
            }
            break;
        case MYMPD_API_QUEUE_ADD_PLAYLIST:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mpd_run_load(mympd_state->mpd_state->conn, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_load");
            }
            break;
        case MYMPD_API_QUEUE_SAVE:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mpd_run_save(mympd_state->mpd_state->conn, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_save");
            }
            break;
        case MYMPD_API_QUEUE_SEARCH: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);          
            if (json_get_uint(request->data, "$.params.offset", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.limit", 0, MAX_MPD_RESULTS, &uint_buf2, &error) == true &&
                json_get_string(request->data, "$.params.filter", 1, 200, &sds_buf1, vcb_ismpdtag_or_any, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 1, 200, &sds_buf2, vcb_isname, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, 20, &error) == true)
            {
                response->data = mpd_client_search_queue(mympd_state, response->data, request->method, request->id, sds_buf1, uint_buf1, uint_buf2, sds_buf2, &tagcols);
            }
            break;
        }
        case MYMPD_API_DATABASE_SEARCH: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);          
            if (json_get_string(request->data, "$.params.searchstr", 1, 200, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.filter", 1, 200, &sds_buf2, vcb_ismpdtag_or_any, &error) == true &&
                json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf3, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.offset", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.limit", 0, MAX_MPD_RESULTS, &uint_buf2, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, 20, &error) == true && 
                json_get_bool(request->data, "$.params.replace", &bool_buf1, &error) == true)
            {
                if (bool_buf1 == true) {
                    rc = mpd_run_clear(mympd_state->mpd_state->conn);
                    if (rc == false) {
                        MYMPD_LOG_ERROR("Clearing queue failed");
                    }
                    check_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0);
                }
                response->data = mpd_shared_search(mympd_state->mpd_state, response->data, request->method, request->id, 
                    sds_buf1, sds_buf2, sds_buf3, uint_buf1, uint_buf2, &tagcols, mympd_state->sticker_cache);
            }
            break;
        }
        case MYMPD_API_DATABASE_SEARCH_ADV: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);          
            if (json_get_string(request->data, "$.params.expression", 1, 200, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.sort", 1, 200, &sds_buf2, vcb_ismpdtag, &error) == true &&
                json_get_bool(request->data, "$.params.sortdesc", &bool_buf1, &error) == true &&
                json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf3, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.offset", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.limit", 0, MAX_MPD_RESULTS, &uint_buf2, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, 20, &error) == true && 
                json_get_bool(request->data, "$.params.replace", &bool_buf2, &error) == true)
            {
                if (bool_buf2 == true) {
                    rc = mpd_run_clear(mympd_state->mpd_state->conn);
                    if (rc == false) {
                        MYMPD_LOG_ERROR("Clearing queue failed");
                    }
                    check_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0);
                }
                response->data = mpd_shared_search_adv(mympd_state->mpd_state, response->data, request->method, request->id, 
                    sds_buf1, sds_buf2, bool_buf1, NULL, sds_buf3, uint_buf1, uint_buf2, &tagcols, mympd_state->sticker_cache);
            }
            break;
        }
        case MYMPD_API_QUEUE_SHUFFLE:
            rc = mpd_run_shuffle(mympd_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_shuffle");
            break;
        case MYMPD_API_PLAYLIST_RM:
            if (json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true) {
                response->data = mpd_client_playlist_delete(mympd_state, response->data, request->method, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_DATABASE_STATS:
            response->data = mpd_client_put_stats(mympd_state, response->data, request->method, request->id);
            break;
        case INTERNAL_API_ALBUMART:
            if (json_get_string(request->data, "$.params.uri", 1, 200, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mpd_client_getcover(mympd_state, response->data, request->method, request->id, sds_buf1, &response->binary);
            }
            break;
        case MYMPD_API_DATABASE_ALBUMS_GET:         
            if (json_get_uint(request->data, "$.params.offset", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.limit", 0, MAX_MPD_RESULTS, &uint_buf2, &error) == true &&
                json_get_string(request->data, "$.params.expression", 0, 200, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.sort", 1, 200, &sds_buf2, vcb_ismpdtag_or_any, &error) == true &&
                json_get_bool(request->data, "$.params.sortdesc", &bool_buf1, &error) == true)
            {
                response->data = mpd_client_put_firstsong_in_albums(mympd_state, response->data, request->method, request->id, 
                    sds_buf1, sds_buf2, bool_buf1, uint_buf1, uint_buf2);
            }
            break;
        case MYMPD_API_DATABASE_TAG_LIST:
            if (json_get_uint(request->data, "$.params.offset", 0, MAX_MPD_PLAYLIST_LENGTH, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.limit", 0, MAX_MPD_RESULTS, &uint_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, 200, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.tag", 1, 200, &sds_buf2, vcb_ismpdtag_or_any, &error) == true)
            {
                response->data = mpd_client_put_db_tag(mympd_state, response->data, request->method, request->id,
                    sds_buf1, sds_buf2, uint_buf1, uint_buf2);
            }
            break;
        case MYMPD_API_DATABASE_TAG_ALBUM_TITLE_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);          
            if (json_get_string(request->data, "$.params.album", 1, 200, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.albumartist", 1, 200, &sds_buf2, vcb_isname, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, 20, &error) == true)
            {
                response->data = mpd_client_put_songs_in_album(mympd_state, response->data, request->method, request->id, sds_buf1, sds_buf2, &tagcols);
            }
            break;
        }
        case INTERNAL_API_TIMER_STARTPLAY:
            if (json_get_uint(request->data, "$.params.volume", 0, 100, &uint_buf1, &error) == true &&
                json_get_string(request->data, "$.params.plist", 1, 200, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.jukeboxMode", 0, 2, &uint_buf2, &error) == true)
            {
                response->data = mpd_client_timer_startplay(mympd_state, response->data, request->method, request->id, uint_buf1, sds_buf1, uint_buf2);
            }
            break;
        case MYMPD_API_URLHANDLERS:
            response->data = mpd_client_put_urlhandlers(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_PARTITION_LIST:
            response->data = mpd_client_put_partitions(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_PARTITION_NEW:
            if (json_get_string(request->data, "$.params.name", 1, 200, &sds_buf1, vcb_isname, &error) == true) {
                rc = mpd_run_newpartition(mympd_state->mpd_state->conn, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_newpartition");
            }
            break;
        case MYMPD_API_PARTITION_SWITCH:
            if (json_get_string(request->data, "$.params.name", 1, 200, &sds_buf1, vcb_isname, &error) == true) {
                rc = mpd_run_switch_partition(mympd_state->mpd_state->conn, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_switch_partition");
            }
            break;
        case MYMPD_API_PARTITION_RM:
            if (json_get_string(request->data, "$.params.name", 1, 200, &sds_buf1, vcb_isname, &error) == true) {
                rc = mpd_run_delete_partition(mympd_state->mpd_state->conn, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_delete_partition");
            }
            break;
        case MYMPD_API_PARTITION_OUTPUT_MOVE:
            if (json_get_string(request->data, "$.params.name", 1, 200, &sds_buf1, vcb_isname, &error) == true) {
                rc = mpd_run_move_output(mympd_state->mpd_state->conn, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_move_output");
            }
            break;
        case MYMPD_API_MOUNT_LIST:
            response->data = mpd_client_put_mounts(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_MOUNT_NEIGHBOR_LIST:
            response->data = mpd_client_put_neighbors(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_MOUNT_MOUNT:
            if (json_get_string(request->data, "$.params.mountUrl", 1, 200, &sds_buf1, vcb_isuri, &error) == true &&
                json_get_string(request->data, "$.params.mountPoint", 1, 200, &sds_buf2, vcb_isfilepath, &error) == true)
            {
                rc = mpd_run_mount(mympd_state->mpd_state->conn, sds_buf2, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_mount");
            }
            break;
        case MYMPD_API_MOUNT_UNMOUNT:
            if (json_get_string(request->data, "$.params.mountPoint", 1, 200, &sds_buf1, vcb_isfilepath, &error) == true) {
                rc = mpd_run_unmount(mympd_state->mpd_state->conn, sds_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_unmount");
            }
            break;
        default:
            response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "general", "error", "Unknown request");
            MYMPD_LOG_ERROR("Unknown API request: %.*s", sdslen(request->data), request->data);
    }
   
    FREE_SDS(sds_buf1);
    FREE_SDS(sds_buf2);
    FREE_SDS(sds_buf3);
    FREE_SDS(sds_buf4);
    FREE_SDS(sds_buf5);
    FREE_SDS(sds_buf6);

    #ifdef DEBUG
    MEASURE_END
    MEASURE_PRINT(request->method)
    #endif

    if (async == true) {
        FREE_SDS(error);
        return;
    }

    if (sdslen(error) > 0) {
        response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
            "general", "error", error);
        MYMPD_LOG_ERROR("Error processing method \"%s\"", request->method);
    }
    FREE_SDS(error);
    if (sdslen(response->data) == 0) {
        response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, true, 
            "general", "error", "No response for method %{method}", 2, "method", request->method);
        MYMPD_LOG_ERROR("No response for method \"%s\"", request->method);
    }
    if (request->conn_id == -2) {
        MYMPD_LOG_DEBUG("Push response to mympd_script_queue for thread %ld: %s", request->id, response->data);
        tiny_queue_push(mympd_script_queue, response, request->id);
    }
    else if (request->conn_id > -1) {
        MYMPD_LOG_DEBUG("Push response to web_server_queue for connection %lld: %s", request->conn_id, response->data);
        tiny_queue_push(web_server_queue, response, 0);
    }
    else {
        free_result(response);
    }
    free_request(request);
}
