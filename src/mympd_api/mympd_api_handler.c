/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_handler.h"

#include "../../dist/src/frozen/frozen.h"
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
    int je;
    int int_buf1;
    int int_buf2; 
    bool bool_buf1;
    bool bool_buf2;
    bool rc;
    float float_buf;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    char *p_charbuf4 = NULL;
    char *p_charbuf5 = NULL;
    char *p_charbuf6 = NULL;
    sds sds_buf1 = NULL;
    sds sds_buf2 = NULL;
    sds sds_buf3 = NULL;
    sds sds_buf4 = NULL;
    sds sds_buf5 = NULL;
    sds sds_buf6 = NULL;
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
            if (json_get_bool(request->data, "$.params.replace", &bool_buf1) == true &&
                json_get_uint(request->data, "$.params.oldpos", 0, 99, &uint_buf1) == true &&
                json_get_string_max(request->data, "$.params.name", &sds_buf1, vcb_isname) == true &&
                json_get_string_max(request->data, "$.params.ligature", &sds_buf2, vcb_isalnum) == true &&
                json_get_string_max(request->data, "$.params.bgcolor", &sds_buf3, vcb_ishexcolor) == true &&
                json_get_string_max(request->data, "$.params.color", &sds_buf4, vcb_ishexcolor) == true &&
                json_get_string_max(request->data, "$.params.image", &sds_buf5, vcb_isfilepath) == true &&
                json_get_string_max(request->data, "$.params.cmd", &sds_buf6, vcb_isalnum) == true &&
                json_get_array_string(request->data, "$.params.options", &options, vcb_isname, 10) == true)
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
            if (json_get_uint(request->data, "$.params.from", 0, 99, &uint_buf1) == true &&
                json_get_uint(request->data, "$.params.to", 0, 99, &uint_buf2) == true)
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
            if (json_get_uint(request->data, "$.params.pos", 0, 99, &uint_buf1) == true) {
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
            if (json_get_uint(request->data, "$.params.pos", 0, 99, &uint_buf1) == true) {
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
            if (json_get_string(request->data, "$.params.script", 1, 200, &sds_buf1, vcb_isfilename) == true &&
                json_get_string(request->data, "$.params.oldscript", 0, 200, &sds_buf2, vcb_isfilename) == true &&
                json_get_int(request->data, "$.params.order", 0, 99, &int_buf1) == true &&
                json_get_string(request->data, "$.params.content", 0, 5000, &sds_buf3, vcb_istext) == true &&
                json_get_array_string(request->data, "$.params.arguments", &arguments, vcb_isalnum, 10) == true)
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
            if (json_get_string(request->data, "$.params.script", 1, 200, &sds_buf1, vcb_isfilename) == true) {
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
            if (json_get_string(request->data, "$.params.script", 1, 200, &sds_buf1, vcb_isfilename) == true) {
                response->data = mympd_api_script_get(mympd_state->config, response->data, request->method, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_SCRIPT_LIST: {
            if (json_get_bool(request->data, "$.params.all", &bool_buf1) == true) {
                response->data = mympd_api_script_list(mympd_state->config, response->data, request->method, request->id, bool_buf1);
            }
            break;
        }

        case MYMPD_API_SCRIPT_EXECUTE: {
            //malloc list - it is used in another thread
            struct list *arguments = (struct list *) malloc(sizeof(struct list));
            assert(arguments);
            list_init(arguments);
            if (json_get_string(request->data, "$.params.script", 1, 200, &sds_buf1, vcb_isfilename) == true && 
                json_get_object_string(request->data, "$.params.arguments", arguments, vcb_isname, 10) == true) {
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
        case INTERNAL_API_SCRIPT_POST_EXECUTE:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {script: %Q}}", &p_charbuf1);
            if (je == 1 && strlen(p_charbuf1) > 0) {
                struct list *arguments = (struct list *) malloc(sizeof(struct list));
                assert(arguments);
                list_init(arguments);
                void *h = NULL;
                struct json_token key;
                struct json_token val;
                while ((h = json_next_key(request->data, (int)sdslen(request->data), h, ".params.arguments", &key, &val)) != NULL) {
                    list_push_len(arguments, key.ptr, key.len, 0, val.ptr, val.len, NULL);
                }
                rc = mympd_api_script_start(mympd_state->config, p_charbuf1, arguments, false);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "script");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "script", "error", "Can't create mympd_script thread");
                }
            }
            break;
        #endif
        case MYMPD_API_COLS_SAVE: {
            sds cols = sdsnewlen("[", 1);
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {table: %Q}}", &p_charbuf1);
            if (je == 1) {
                bool error = false;
                cols = json_to_cols(cols, request->data, sdslen(request->data), &error);
                if (error == false) {
                    cols = sdscatlen(cols, "]", 1);
                    if (mympd_api_cols_save(mympd_state, p_charbuf1, cols)) {
                        response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
                    }
                    else {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, true,
                            "general", "error", "Unknown table %{table}", 2, "table", p_charbuf1);
                        MYMPD_LOG_ERROR("MYMPD_API_COLS_SAVE: Unknown table %s", p_charbuf1);
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
        case MYMPD_API_SETTINGS_SET: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            rc = true;
            while ((h = json_next_key(request->data, (int)sdslen(request->data), h, ".params", &key, &val)) != NULL) {
                rc = mympd_api_settings_set(mympd_state, &key, &val);
                if (rc == false) {
                    break;
                }
            }
            if (rc == true) {
                if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                    //feature detection
                    mpd_client_mpd_features(mympd_state);
                }
                //respond with ok
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
            }
            else {
                sds value = sdsnewlen(key.ptr, key.len);
                response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, true,
                    "general", "error", "Can't save settinsds sdsbuf1 = NULL;g %{setting}", 2, "setting", value);
                sdsfree(value);
            }
            break;
        }
        case MYMPD_API_PLAYER_OPTIONS_SET: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            rc = true;
            bool jukebox_changed = false;
            bool check_mpd_error = false;
            while ((h = json_next_key(request->data, (int)sdslen(request->data), h, ".params", &key, &val)) != NULL) {
                rc = mpdclient_api_options_set(mympd_state, &key, &val, &jukebox_changed, &check_mpd_error);
                if (check_mpd_error == true && 
                    check_error_and_recover2(mympd_state->mpd_state, NULL, request->method, request->id, false) == false)
                {
                    break;
                }
                if (rc == false) {
                    break;
                }
            }
            if (rc == true) {
                //respond with ok
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
                if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                    //feature detection
                    mpd_client_mpd_features(mympd_state);
                    
                    if (jukebox_changed == true) {
                        MYMPD_LOG_DEBUG("Jukebox options changed, clearing jukebox queue");
                        list_free(&mympd_state->jukebox_queue);
                        mympd_state->jukebox_enforce_unique = true;
                    }
                    if (mympd_state->jukebox_mode != JUKEBOX_OFF) {
                        //enable jukebox
                        mpd_client_jukebox(mympd_state, 0);
                    }
                }
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
            response->data = mympd_api_settings_put(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_CONNECTION_SAVE: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            rc = true;
            bool mpd_host_changed = false;
            while ((h = json_next_key(request->data, (int)sdslen(request->data), h, ".params", &key, &val)) != NULL) {
                rc = mympd_api_connection_save(mympd_state, &key, &val, &mpd_host_changed);
                if (rc == false) {
                    break;
                }
            }
            if (rc == true) {
                if (mpd_host_changed == true) {
                    //reconnect to new mpd
                    MYMPD_LOG_DEBUG("MPD host has changed, disconnecting");
                    mympd_state->mpd_state->conn_state = MPD_DISCONNECT_INSTANT;
                }
                else if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                    //feature detection
                    mpd_client_mpd_features(mympd_state);
                }
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "general");
            }
            else {
                sds value = sdsnewlen(key.ptr, key.len);
                response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, true,
                    "mpd", "error", "Can't save setting %{setting}", 2, "setting", value);
                sdsfree(value);
            }
            break;
        }
        case MYMPD_API_COVERCACHE_CROP:
            //TODO: error checking
            clear_covercache(mympd_state->config->workdir, mympd_state->covercache_keep_days);
            response->data = jsonrpc_respond_message(response->data, request->method, request->id, false, 
                        "general", "info", "Successfully croped covercache");
            break;
        case MYMPD_API_COVERCACHE_CLEAR:
            clear_covercache(mympd_state->config->workdir, 0);
            //TODO: error checking
            response->data = jsonrpc_respond_message(response->data, request->method, request->id, false, 
                        "general", "info", "Successfully cleared covercache");
            break;
        case INTERNAL_API_TIMER_SET:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {timeout: %d, interval: %d, handler: %Q}}", &int_buf1, &int_buf2, &p_charbuf1);
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {timerid: %d, interval: %d}}", &int_buf1, &int_buf2);
            if (je == 2 && timer_def != NULL) {
                if (int_buf1 == 0) {
                    mympd_state->timer_list.last_id++;
                    int_buf1 = mympd_state->timer_list.last_id;
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {timerid: %d}}", &int_buf1);
            if (je == 1) {
                response->data = timer_get(mympd_state, response->data, request->method, request->id, int_buf1);
            }
            break;
        case MYMPD_API_TIMER_RM:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {timerid: %d}}", &int_buf1);
            if (je == 1) {
                remove_timer(&mympd_state->timer_list, int_buf1);
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "timer");
            }
            break;
        case MYMPD_API_TIMER_TOGGLE:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {timerid: %d}}", &int_buf1);
            if (je == 1) {
                toggle_timer(&mympd_state->timer_list, int_buf1);
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "timer");
            }
            break;
        case MYMPD_API_LYRICS_GET:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                if (p_charbuf1 == NULL || validate_uri(p_charbuf1) == false) {
                    MYMPD_LOG_ERROR("Invalid URI: %s", p_charbuf1);
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "lyrics", "error", "Invalid uri");
                }
                else {
                    response->data = mpd_client_lyrics_get(mympd_state, response->data, request->method, request->id, p_charbuf1);
                }
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {pos: %u}}", &uint_buf1);
            if (je == 1) {
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
            struct t_tags *tagcols = (struct t_tags *)malloc(sizeof(struct t_tags));
            assert(tagcols);
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {offset: %u, limit: %u, cols: %M}}", &uint_buf1, &uint_buf2, json_to_tags, tagcols);
            if (je == 3) {
                response->data = mpd_client_put_jukebox_list(mympd_state, response->data, request->method, request->id, uint_buf1, uint_buf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MYMPD_API_TRIGGER_LIST:
            response->data = trigger_list(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_TRIGGER_GET:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {id: %d}}", &int_buf1);
            if (je == 1) {
                response->data = trigger_get(mympd_state, response->data, request->method, request->id, int_buf1);
            }
            break;
        case MYMPD_API_TRIGGER_SAVE:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {id: %d, name: %Q, event: %d, script: %Q}}", 
                &int_buf1, &p_charbuf1, &int_buf2, &p_charbuf2);
            if (je == 4 && validate_string_not_empty(p_charbuf1) == true && validate_string_not_empty(p_charbuf2) == true) {
                struct list *arguments = (struct list *) malloc(sizeof(struct list));
                assert(arguments);
                list_init(arguments);
                void *h = NULL;
                struct json_token key;
                struct json_token val;
                while ((h = json_next_key(request->data, (int)sdslen(request->data), h, ".params.arguments", &key, &val)) != NULL) {
                    list_push_len(arguments, key.ptr, key.len, 0, val.ptr, val.len, NULL);
                }
                //add new entry
                rc = list_push(&mympd_state->triggers, p_charbuf1, int_buf2, p_charbuf2, arguments);
                if (rc == true) {
                    if (int_buf1 >= 0) {
                        //delete old entry
                        rc = delete_trigger(mympd_state, int_buf1);
                    }
                    if (rc == true) {
                        response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "trigger");
                    }
                    else {
                        response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "trigger", "error", "Could not save trigger");
                    }
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "trigger", "error", "Could not save trigger");
                }
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "trigger", "error", "Invalid trigger name");
            }
            break;
        case MYMPD_API_TRIGGER_RM:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {id: %u}}", &uint_buf1);
            if (je == 1) {
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
        case MYMPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {outputId: %u}}", &uint_buf1);
            if (je == 1) {
                void *h = NULL;
                struct json_token key;
                struct json_token val;
                while ((h = json_next_key(request->data, (int)sdslen(request->data), h, ".params.attributes", &key, &val)) != NULL) {
                    sds attribute = sdsnewlen(key.ptr, key.len);
                    sds value = sdsnewlen(val.ptr, val.len);
                    rc = mpd_run_output_set(mympd_state->mpd_state->conn, uint_buf1, attribute, value);
                    sdsfree(attribute);
                    sdsfree(value);
                    response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_output_set");
                    if (rc == false) {
                        break;
                    }
                }
            }
            break;
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {channel: %Q, message: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                uint_buf1 = mpd_run_send_message(mympd_state->mpd_state->conn, p_charbuf1, p_charbuf2);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, (uint_buf1 == 0 ? false : true), "mpd_run_send_message");
            }
            break;
        case MYMPD_API_LIKE:
            if (mympd_state->mpd_state->feat_stickers == false) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "sticker", "error", "MPD stickers are disabled");
                MYMPD_LOG_ERROR("MPD stickers are disabled");
                break;
            }
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {uri: %Q, like: %d}}", &p_charbuf1, &int_buf1);
            if (je == 2 && strlen(p_charbuf1) > 0) {
                if (int_buf1 < 0 || int_buf1 > 2) {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "sticker", "error", "Failed to set like, invalid like value");
                    break;
                }
                if (is_streamuri(p_charbuf1) == true) {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "sticker", "error", "Failed to set like, invalid song uri");
                    break;
                }
                rc = mpd_client_sticker_like(mympd_state, p_charbuf1, int_buf1);
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                if (strcmp(p_charbuf1, "") == 0) {
                    FREE_PTR(p_charbuf1);
                }
                if (request->cmd_id == MYMPD_API_DATABASE_UPDATE) {
                    uint_buf1 = mpd_run_update(mympd_state->mpd_state->conn, p_charbuf1);
                    response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, (uint_buf1 == 0 ? false : true), "mpd_run_update");
                }
                else {
                    uint_buf1 = mpd_run_rescan(mympd_state->mpd_state->conn, p_charbuf1);
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
                je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q, sticker: %Q, maxentries: %d, minvalue: %d, sort: %Q}}", &p_charbuf1, &p_charbuf2, &int_buf1, &int_buf2, &p_charbuf3);
                if (je == 5) {
                    rc = mpd_shared_smartpls_save(mympd_state->config->workdir, "sticker", p_charbuf1, p_charbuf2, int_buf1, int_buf2, p_charbuf3);
                }
            }
            else if (request->cmd_id == MYMPD_API_SMARTPLS_NEWEST_SAVE) {
                je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q, timerange: %d, sort: %Q}}", &p_charbuf1, &int_buf1, &p_charbuf2);
                if (je == 3) {
                    rc = mpd_shared_smartpls_save(mympd_state->config->workdir, "newest", p_charbuf1, NULL, 0, int_buf1, p_charbuf2);
                }
            }            
            else if (request->cmd_id == MYMPD_API_SMARTPLS_SEARCH_SAVE) {
                je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q, expression: %Q, sort: %Q}}", &p_charbuf1, &p_charbuf2, &p_charbuf3);
                if (je == 3) {
                    rc = mpd_shared_smartpls_save(mympd_state->config->workdir, "search", p_charbuf1, p_charbuf2, 0, 0, p_charbuf3);
                }
            }
            if (rc == true) {
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id, "playlist");
                mpd_client_smartpls_update(p_charbuf1);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "playlist", "error", "Failed to save smart playlist");
            }
            break;
        case MYMPD_API_SMARTPLS_GET:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_client_smartpls_put(mympd_state->config, response->data, request->method, request->id, p_charbuf1);
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {songId:%u}}", &uint_buf1);
            if (je == 1) {
                rc = mpd_run_delete_id(mympd_state->mpd_state->conn, uint_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_delete_id");
            }
            break;
        case MYMPD_API_QUEUE_RM_RANGE:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {start: %u, end: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                rc = mpd_run_delete_range(mympd_state->mpd_state->conn, uint_buf1, uint_buf2);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_delete_range");
            }
            break;
        case MYMPD_API_QUEUE_MOVE_SONG:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {from: %u, to: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {trackid: %u}}", &uint_buf1);
            if (je == 1) {
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q, from: %u, to: %u }}", &p_charbuf1, &uint_buf1, &uint_buf2);
            if (je == 3) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2) {
                    uint_buf2--;
                }
                rc = mpd_run_playlist_move(mympd_state->mpd_state->conn, p_charbuf1, uint_buf1, uint_buf2);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_move");
            }
            break;
        case MYMPD_API_PLAYER_PLAY_SONG:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: { songId: %u}}", &uint_buf1);
            if (je == 1) {
                rc = mpd_run_play_id(mympd_state->mpd_state->conn, uint_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_play_id");
            }
            break;
        case MYMPD_API_PLAYER_OUTPUT_LIST:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {partition: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_client_put_partition_outputs(mympd_state, response->data, request->method, request->id, p_charbuf1);
            }
            else {
                response->data = mpd_client_put_outputs(mympd_state, response->data, request->method, request->id);
            }
            break;
        case MYMPD_API_PLAYER_OUTPUT_TOGGLE:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {outputId: %u, state: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {volume: %u}}", &uint_buf1);
            if (je == 1) {
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {seek: %f, relative: %B}}", &float_buf, &bool_buf1);
            if (je == 2) {
                rc = mpd_run_seek_current(mympd_state->mpd_state->conn, float_buf, bool_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_seek_current");
            }
            break;
        case MYMPD_API_QUEUE_LIST: {
            struct t_tags *tagcols = (struct t_tags *)malloc(sizeof(struct t_tags));
            assert(tagcols);
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {offset: %u, limit: %u, cols: %M}}", &uint_buf1, &uint_buf2, json_to_tags, tagcols);
            if (je == 3) {
                response->data = mpd_client_put_queue(mympd_state, response->data, request->method, request->id, uint_buf1, uint_buf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MYMPD_API_QUEUE_LAST_PLAYED: {
            struct t_tags *tagcols = (struct t_tags *)malloc(sizeof(struct t_tags));
            assert(tagcols);
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {offset: %u, limit: %u, cols: %M}}", &uint_buf1, &uint_buf2, json_to_tags, tagcols);
            if (je == 3) {
                response->data = mpd_client_put_last_played_songs(mympd_state, response->data, request->method, request->id, uint_buf1, uint_buf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MYMPD_API_PLAYER_CURRENT_SONG: {
            response->data = mpd_client_put_current_song(mympd_state, response->data, request->method, request->id);
            break;
        }
        case MYMPD_API_DATABASE_SONGDETAILS:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1 && strlen(p_charbuf1) > 0) {
                response->data = mpd_client_put_songdetails(mympd_state, response->data, request->method, request->id, p_charbuf1);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "database", "error", "Invalid API request");
            }
            break;
        case MYMPD_API_DATABASE_COMMENTS:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1 && strlen(p_charbuf1) > 0) {
                response->data = mpd_client_read_comments(mympd_state, response->data, request->method, request->id, p_charbuf1);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "database", "error", "Invalid API request");
            }
            break;
        case MYMPD_API_DATABASE_FINGERPRINT:
            if (mympd_state->mpd_state->feat_fingerprint == true) {
                je = json_scanf(request->data, (int)sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
                if (je == 1 && strlen(p_charbuf1) > 0) {
                    response->data = mpd_client_put_fingerprint(mympd_state, response->data, request->method, request->id, p_charbuf1);
                }
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "database", "error", "Fingerprint command not supported");
            }
            break;

        case MYMPD_API_PLAYLIST_RENAME:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q, newName: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                response->data = mpd_client_playlist_rename(mympd_state, response->data, request->method, request->id, p_charbuf1, p_charbuf2);
            }
            break;            
        case MYMPD_API_PLAYLIST_LIST:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {offset: %u, limit: %u, searchstr: %Q}}", &uint_buf1, &uint_buf2, &p_charbuf1);
            if (je == 3) {
                response->data = mpd_client_put_playlists(mympd_state, response->data, request->method, request->id, uint_buf1, uint_buf2, p_charbuf1);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_LIST: {
            struct t_tags *tagcols = (struct t_tags *)malloc(sizeof(struct t_tags));
            assert(tagcols);
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q, offset: %u, limit: %u searchstr: %Q, cols: %M}}", 
                &p_charbuf1, &uint_buf1, &uint_buf2, &p_charbuf2, json_to_tags, tagcols);
            if (je == 5) {
                response->data = mpd_client_put_playlist_list(mympd_state, response->data, request->method, request->id, p_charbuf1, uint_buf1, uint_buf2, p_charbuf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MYMPD_API_PLAYLIST_ADD_URI:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q, uri: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                rc = mpd_run_playlist_add(mympd_state->mpd_state->conn, p_charbuf1, p_charbuf2);
                if (check_error_and_recover2(mympd_state->mpd_state, &response->data, request->method, request->id, false) == true && rc == true) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->method, request->id, false, 
                    "playlist", "info", "Added %{uri} to playlist %{playlist}", 4, "uri", p_charbuf2, "playlist", p_charbuf1);
                }
                else if (rc == false) {
                    response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_add");
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CLEAR:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_playlist_clear(mympd_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_clear");
            }
            break;
        case MYMPD_API_PLAYLIST_RM_ALL:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {type: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_client_playlist_delete_all(mympd_state, response->data, request->method, request->id, p_charbuf1);
            }
            break;
        case MYMPD_API_PLAYLIST_RM_SONG:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q, pos: %u}}", &p_charbuf1, &uint_buf1);
            if (je == 2) {
                rc = mpd_run_playlist_delete(mympd_state->mpd_state->conn, p_charbuf1, uint_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_delete");
            }
            break;
        case MYMPD_API_PLAYLIST_SHUFFLE:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_shared_playlist_shuffle_sort(mympd_state->mpd_state, response->data, request->method, request->id, p_charbuf1, "shuffle");
            }
            break;
        case MYMPD_API_PLAYLIST_SORT:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q, tag:%Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                response->data = mpd_shared_playlist_shuffle_sort(mympd_state->mpd_state, response->data, request->method, request->id, p_charbuf1, p_charbuf2);
            }
            break;
        case MYMPD_API_DATABASE_FILESYSTEM_LIST: {
            struct t_tags *tagcols = (struct t_tags *)malloc(sizeof(struct t_tags));
            assert(tagcols);
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {offset:%u, limit:%u, searchstr:%Q, path:%Q, cols: %M}}", 
                &uint_buf1, &uint_buf2, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 5) {
                response->data = mpd_client_put_filesystem(mympd_state, response->data, request->method, request->id, p_charbuf2, uint_buf1, uint_buf2, p_charbuf1, tagcols);
            }
            free(tagcols);
            break;
        }
        case MYMPD_API_QUEUE_ADD_URI_AFTER:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {uri:%Q, to:%d}}", &p_charbuf1, &int_buf1);
            if (je == 2) {
                rc = mpd_run_add_id_to(mympd_state->mpd_state->conn, p_charbuf1, int_buf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_add_id_to");
            }
            break;
        case MYMPD_API_QUEUE_REPLACE_URI:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {uri:%Q }}", &p_charbuf1);
            if (je == 1 && strlen(p_charbuf1) > 0) {
                rc = mpd_client_queue_replace_with_song(mympd_state, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_client_queue_replace_with_song");
            }
            break;
        case MYMPD_API_QUEUE_ADD_URI:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {uri:%Q}}", &p_charbuf1);
            if (je == 1 && strlen(p_charbuf1) > 0) {
                rc = mpd_run_add(mympd_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_add");
            }
            break;
        case MYMPD_API_QUEUE_ADD_PLAY_URI:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                int_buf1 = mpd_run_add_id(mympd_state->mpd_state->conn, p_charbuf1);
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_client_queue_replace_with_playlist(mympd_state, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_client_queue_replace_with_playlist");
            }
            break;
        case MYMPD_API_QUEUE_ADD_RANDOM:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {mode:%u, plist:%Q, quantity:%u}}", &uint_buf1, &p_charbuf1, &uint_buf2);
            if (je == 3) {
                if (uint_buf2 > 999) {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, 
                        "queue", "error", "Number of songs to high");
                    break;
                }
                rc = mpd_client_jukebox_add_to_queue(mympd_state, uint_buf2, uint_buf1, p_charbuf1, true);
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_load(mympd_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_load");
            }
            break;
        case MYMPD_API_QUEUE_SAVE:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_save(mympd_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_save");
            }
            break;
        case MYMPD_API_QUEUE_SEARCH: {
            struct t_tags *tagcols = (struct t_tags *)malloc(sizeof(struct t_tags));
            assert(tagcols);
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {offset: %u, limit: %u, filter: %Q, searchstr: %Q, cols: %M}}", 
                &uint_buf1, &uint_buf2, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 5) {
                response->data = mpd_client_search_queue(mympd_state, response->data, request->method, request->id, p_charbuf1, uint_buf1, uint_buf2, p_charbuf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MYMPD_API_DATABASE_SEARCH: {
            struct t_tags *tagcols = (struct t_tags *)malloc(sizeof(struct t_tags));
            assert(tagcols);
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {searchstr:%Q, filter:%Q, plist:%Q, offset:%u, limit:%u, cols: %M, replace:%B}}", 
                &p_charbuf1, &p_charbuf2, &p_charbuf3, &uint_buf1, &uint_buf2, json_to_tags, tagcols, &bool_buf1);
            if (je == 7) {
                if (bool_buf1 == true) {
                    rc = mpd_run_clear(mympd_state->mpd_state->conn);
                    if (rc == false) {
                        MYMPD_LOG_ERROR("Clearing queue failed");
                    }
                    check_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0);
                }
                response->data = mpd_shared_search(mympd_state->mpd_state, response->data, request->method, request->id, 
                    p_charbuf1, p_charbuf2, p_charbuf3, uint_buf1, uint_buf2, tagcols, mympd_state->sticker_cache);
            }
            free(tagcols);
            break;
        }
        case MYMPD_API_DATABASE_SEARCH_ADV: {
            struct t_tags *tagcols = (struct t_tags *)malloc(sizeof(struct t_tags));
            assert(tagcols);
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {expression:%Q, sort:%Q, sortdesc:%B, plist:%Q, offset:%u, limit:%u, cols: %M, replace:%B}}", 
                &p_charbuf1, &p_charbuf2, &bool_buf1, &p_charbuf3, &uint_buf1, &uint_buf2, json_to_tags, tagcols, &bool_buf2);
            if (je == 8) {
                if (bool_buf2 == true) {
                    rc = mpd_run_clear(mympd_state->mpd_state->conn);
                    if (rc == false) {
                        MYMPD_LOG_ERROR("Clearing queue failed");
                    }
                    check_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0);
                }
                response->data = mpd_shared_search_adv(mympd_state->mpd_state, response->data, request->method, request->id, 
                    p_charbuf1, p_charbuf2, bool_buf1, NULL, p_charbuf3, uint_buf1, uint_buf2, tagcols, mympd_state->sticker_cache);
            }
            free(tagcols);
            break;
        }
        case MYMPD_API_QUEUE_SHUFFLE:
            rc = mpd_run_shuffle(mympd_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_shuffle");
            break;
        case MYMPD_API_PLAYLIST_RM:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {plist: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_client_playlist_delete(mympd_state, response->data, request->method, request->id, p_charbuf1);
            }
            break;
        case MYMPD_API_DATABASE_STATS:
            response->data = mpd_client_put_stats(mympd_state, response->data, request->method, request->id);
            break;
        case INTERNAL_API_ALBUMART:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_client_getcover(mympd_state, response->data, request->method, request->id, p_charbuf1, &response->binary);
            }
            break;
        case MYMPD_API_DATABASE_ALBUMS_GET:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {offset: %u, limit: %u, expression: %Q, sort: %Q, sortdesc: %B}}", 
                &uint_buf1, &uint_buf2, &p_charbuf1, &p_charbuf2, &bool_buf1);
            if (je == 5) {
                response->data = mpd_client_put_firstsong_in_albums(mympd_state, response->data, request->method, request->id, 
                    p_charbuf1, p_charbuf2, bool_buf1, uint_buf1, uint_buf2);
            }
            break;
        case MYMPD_API_DATABASE_TAG_LIST:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {offset: %u, limit: %u, searchstr: %Q, tag: %Q}}", 
                &uint_buf1, &uint_buf2, &p_charbuf1, &p_charbuf2);
            if (je == 4) {
                response->data = mpd_client_put_db_tag(mympd_state, response->data, request->method, request->id,
                    p_charbuf1, p_charbuf2, uint_buf1, uint_buf2);
            }
            break;
        case MYMPD_API_DATABASE_TAG_ALBUM_TITLE_LIST: {
            struct t_tags *tagcols = (struct t_tags *)malloc(sizeof(struct t_tags));
            assert(tagcols);
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {album: %Q, albumartist: %Q, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 3) {
                response->data = mpd_client_put_songs_in_album(mympd_state, response->data, request->method, request->id, p_charbuf1, p_charbuf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case INTERNAL_API_TIMER_STARTPLAY:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {volume:%u, plist:%Q, jukeboxMode:%u}}", &uint_buf1, &p_charbuf1, &uint_buf2);
            if (je == 3) {
                response->data = mpd_client_timer_startplay(mympd_state, response->data, request->method, request->id, uint_buf1, p_charbuf1, uint_buf2);
            }
            break;
        case MYMPD_API_URLHANDLERS:
            response->data = mpd_client_put_urlhandlers(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_PARTITION_LIST:
            response->data = mpd_client_put_partitions(mympd_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_PARTITION_NEW:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {name: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_newpartition(mympd_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_newpartition");
            }
            break;
        case MYMPD_API_PARTITION_SWITCH:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {name: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_switch_partition(mympd_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_switch_partition");
            }
            break;
        case MYMPD_API_PARTITION_RM:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {name: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_delete_partition(mympd_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_delete_partition");
            }
            break;
        case MYMPD_API_PARTITION_OUTPUT_MOVE:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {name: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_move_output(mympd_state->mpd_state->conn, p_charbuf1);
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
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {mountUrl: %Q, mountPoint: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                rc = mpd_run_mount(mympd_state->mpd_state->conn, p_charbuf2, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_mount");
            }
            break;
        case MYMPD_API_MOUNT_UNMOUNT:
            je = json_scanf(request->data, (int)sdslen(request->data), "{params: {mountPoint: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_unmount(mympd_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mympd_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_unmount");
            }
            break;
        default:
            response->data = jsonrpc_respond_message(response->data, request->method, request->id, true, "general", "error", "Unknown request");
            MYMPD_LOG_ERROR("Unknown API request: %.*s", sdslen(request->data), request->data);
    }
    FREE_PTR(p_charbuf1);
    FREE_PTR(p_charbuf2);
    FREE_PTR(p_charbuf3);                    
    FREE_PTR(p_charbuf4);
    FREE_PTR(p_charbuf5);
    FREE_PTR(p_charbuf6);
    
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
        return;
    }

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
