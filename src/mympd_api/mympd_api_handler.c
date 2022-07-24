/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_handler.h"

#include "../lib/album_cache.h"
#include "../lib/api.h"
#include "../lib/covercache.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/lua_mympd_state.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"
#include "../lib/smartpls.h"
#include "../lib/sticker_cache.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_client/mpd_client_connection.h"
#include "../mpd_client/mpd_client_errorhandler.h"
#include "../mpd_client/mpd_client_features.h"
#include "../mpd_client/mpd_client_jukebox.h"
#include "../mpd_client/mpd_client_playlists.h"
#include "../mpd_client/mpd_client_search.h"
#include "../mpd_client/mpd_client_tags.h"
#include "../mpd_worker/mpd_worker.h"
#include "mympd_api_albumart.h"
#include "mympd_api_browse.h"
#include "mympd_api_filesystem.h"
#include "mympd_api_home.h"
#include "mympd_api_last_played.h"
#include "mympd_api_lyrics.h"
#include "mympd_api_mounts.h"
#include "mympd_api_outputs.h"
#include "mympd_api_partitions.h"
#include "mympd_api_pictures.h"
#include "mympd_api_playlists.h"
#include "mympd_api_queue.h"
#include "mympd_api_scripts.h"
#include "mympd_api_settings.h"
#include "mympd_api_smartpls.h"
#include "mympd_api_song.h"
#include "mympd_api_stats.h"
#include "mympd_api_status.h"
#include "mympd_api_sticker.h"
#include "mympd_api_timer.h"
#include "mympd_api_timer_handlers.h"
#include "mympd_api_trigger.h"
#include "mympd_api_webradios.h"

#include <stdlib.h>
#include <string.h>

//private definitions
static bool check_start_play(struct t_mympd_state *mympd_state, bool play, sds *buffer, enum mympd_cmd_ids cmd_id, long id);

//public functions

void mympd_api_handler(struct t_mympd_state *mympd_state, struct t_work_request *request) {
    unsigned uint_buf1;
    unsigned uint_buf2;
    long long_buf1;
    long long_buf2;
    int int_buf1;
    int int_buf2;
    bool bool_buf1;
    bool rc;
    bool result;
    sds sds_buf1 = NULL;
    sds sds_buf2 = NULL;
    sds sds_buf3 = NULL;
    sds sds_buf4 = NULL;
    sds sds_buf5 = NULL;
    sds sds_buf6 = NULL;
    sds sds_buf7 = NULL;
    sds sds_buf8 = NULL;
    sds sds_buf9 = NULL;
    sds sds_buf0 = NULL;
    sds error = sdsempty();
    bool async = false;

    #ifdef DEBUG
    MEASURE_INIT
    MEASURE_START
    #endif

    MYMPD_LOG_INFO("MYMPD API request (%lld)(%ld) %s: %s", request->conn_id, request->id, request->method, request->data);

    //shortcut
    struct t_config *config = mympd_state->config;

    //create response struct
    struct t_work_response *response = create_response(request);

    switch(request->cmd_id) {
        case MYMPD_API_LOGLEVEL:
            if (json_get_int(request->data, "$.params.loglevel", 0, 7, &int_buf1, &error) == true) {
                MYMPD_LOG_INFO("Setting loglevel to %d", int_buf1);
                loglevel = int_buf1;
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_GENERAL);
            }
            break;
        case MYMPD_API_SMARTPLS_UPDATE_ALL:
        case MYMPD_API_SMARTPLS_UPDATE:
        case INTERNAL_API_CACHES_CREATE:
            if (worker_threads > 5) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Too many worker threads are already running");
                break;
            }
            if (request->cmd_id == INTERNAL_API_CACHES_CREATE) {
                if (mympd_state->album_cache.building == true ||
                    mympd_state->sticker_cache.building == true)
                {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Cache update is already running");
                    break;
                }
                mympd_state->album_cache.building = true;
                mympd_state->sticker_cache.building = true;
            }
            async = true;
            free_response(response);
            mpd_worker_start(mympd_state, request);
            break;
        case MYMPD_API_PICTURE_LIST:
            if (json_get_string(request->data, "$.params.type", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                response->data = mympd_api_settings_picture_list(mympd_state->config->workdir, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_HOME_ICON_SAVE: {
            if (mympd_state->home_list.length > LIST_HOME_ICONS_MAX) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_HOME, JSONRPC_SEVERITY_ERROR, "Too many home icons");
                break;
            }
            struct t_list options;
            list_init(&options);
            if (json_get_bool(request->data, "$.params.replace", &bool_buf1, &error) == true &&
                json_get_long(request->data, "$.params.oldpos", 0, LIST_HOME_ICONS_MAX, &long_buf1, &error) == true &&
                json_get_string_max(request->data, "$.params.name", &sds_buf1, vcb_isname, &error) == true &&
                json_get_string_max(request->data, "$.params.ligature", &sds_buf2, vcb_isalnum, &error) == true &&
                json_get_string(request->data, "$.params.bgcolor", 1, 7, &sds_buf3, vcb_ishexcolor, &error) == true &&
                json_get_string(request->data, "$.params.color", 1, 7, &sds_buf4, vcb_ishexcolor, &error) == true &&
                json_get_string(request->data, "$.params.image", 0, FILEPATH_LEN_MAX, &sds_buf5, vcb_isuri, &error) == true &&
                json_get_string_max(request->data, "$.params.cmd", &sds_buf6, vcb_isalnum, &error) == true &&
                json_get_array_string(request->data, "$.params.options", &options, vcb_isname, 10, &error) == true)
            {
                rc = mympd_api_home_icon_save(&mympd_state->home_list, bool_buf1, long_buf1, sds_buf1, sds_buf2, sds_buf3, sds_buf4, sds_buf5, sds_buf6, &options);
                if (rc == true) {
                    response->data = mympd_api_home_icon_list(&mympd_state->home_list, response->data, request->cmd_id, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_HOME, JSONRPC_SEVERITY_ERROR, "Can not save home icon");
                }
            }
            list_clear(&options);
            break;
        }
        case MYMPD_API_HOME_ICON_MOVE:
            if (json_get_long(request->data, "$.params.from", 0, LIST_HOME_ICONS_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.to", 0, LIST_HOME_ICONS_MAX, &long_buf2, &error) == true)
            {
                rc = mympd_api_home_icon_move(&mympd_state->home_list, long_buf1, long_buf2);
                if (rc == true) {
                    response->data = mympd_api_home_icon_list(&mympd_state->home_list, response->data, request->cmd_id, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_HOME, JSONRPC_SEVERITY_ERROR, "Can not move home icon");
                }
            }
            break;
        case MYMPD_API_HOME_ICON_RM:
            if (json_get_long(request->data, "$.params.pos", 0, LIST_HOME_ICONS_MAX, &long_buf1, &error) == true) {
                rc = mympd_api_home_icon_delete(&mympd_state->home_list, long_buf1);
                if (rc == true) {
                    response->data = mympd_api_home_icon_list(&mympd_state->home_list, response->data, request->cmd_id, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_HOME, JSONRPC_SEVERITY_ERROR, "Can not delete home icon");
                }
            }
            break;
        case MYMPD_API_HOME_ICON_GET:
            if (json_get_long(request->data, "$.params.pos", 0, LIST_HOME_ICONS_MAX, &long_buf1, &error) == true) {
                response->data = mympd_api_home_icon_get(&mympd_state->home_list, response->data, request->id, long_buf1);
            }
            break;
        case MYMPD_API_HOME_ICON_LIST:
            response->data = mympd_api_home_icon_list(&mympd_state->home_list, response->data, request->cmd_id, request->id);
            break;
        #ifdef ENABLE_LUA
        case MYMPD_API_SCRIPT_SAVE: {
            struct t_list arguments;
            list_init(&arguments);
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.oldscript", 0, FILENAME_LEN_MAX, &sds_buf2, vcb_isfilename, &error) == true &&
                json_get_int(request->data, "$.params.order", 0, LIST_TIMER_MAX, &int_buf1, &error) == true &&
                json_get_string(request->data, "$.params.content", 0, CONTENT_LEN_MAX, &sds_buf3, vcb_istext, &error) == true &&
                json_get_array_string(request->data, "$.params.arguments", &arguments, vcb_isalnum, 10, &error) == true)
            {
                rc = mympd_api_script_save(config->workdir, sds_buf1, sds_buf2, int_buf1, sds_buf3, &arguments);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_SCRIPT);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Could not save script");
                }
            }
            list_clear(&arguments);
            break;
        }
        case MYMPD_API_SCRIPT_RM:
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mympd_api_script_delete(config->workdir, sds_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_SCRIPT);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Could not delete script");
                }
            }
            break;
        case MYMPD_API_SCRIPT_GET:
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                response->data = mympd_api_script_get(config->workdir, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_SCRIPT_LIST: {
            if (json_get_bool(request->data, "$.params.all", &bool_buf1, &error) == true) {
                response->data = mympd_api_script_list(config->workdir, response->data, request->id, bool_buf1);
            }
            break;
        }
        case MYMPD_API_SCRIPT_EXECUTE: {
            //malloc list - it is used in another thread
            struct t_list *arguments = list_new();
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_object_string(request->data, "$.params.arguments", arguments, vcb_isname, 10, &error) == true)
            {
                rc = mympd_api_script_start(config->workdir, sds_buf1, config->lualibs, arguments, true);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_SCRIPT);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Can't create mympd_script thread");
                }
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Invalid script name");
                list_free(arguments);
            }
            break;
        }
        case INTERNAL_API_SCRIPT_POST_EXECUTE: {
            //malloc list - it is used in another thread
            struct t_list *arguments = list_new();
            if (json_get_string(request->data, "$.params.script", 1, CONTENT_LEN_MAX, &sds_buf1, vcb_istext, &error) == true &&
                json_get_object_string(request->data, "$.params.arguments", arguments, vcb_isname, 10, &error) == true)
            {
                rc = mympd_api_script_start(config->workdir, sds_buf1, config->lualibs, arguments, false);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_SCRIPT);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Can't create mympd_script thread");
                }
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Invalid script content");
                list_clear(arguments);
                FREE_PTR(arguments);
            }
            break;
        }
        #endif
        case MYMPD_API_COLS_SAVE: {
            if (json_get_string(request->data, "$.params.table", 1, NAME_LEN_MAX, &sds_buf1, vcb_isalnum, &error) == true) {
                rc = false;
                sds cols = sdsnewlen("[", 1);
                cols = json_get_cols_as_string(request->data, cols, &rc);
                if (rc == true) {
                    cols = sdscatlen(cols, "]", 1);
                    if (mympd_api_settings_cols_save(mympd_state, sds_buf1, cols) == true) {
                        response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_GENERAL);
                    }
                    else {
                        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Unknown table %{table}", 2, "table", sds_buf1);
                        MYMPD_LOG_ERROR("MYMPD_API_COLS_SAVE: Unknown table %s", sds_buf1);
                    }
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Invalid column");
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
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_GENERAL);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Can't save settings");
            }
            break;
        }
        case MYMPD_API_PLAYER_OPTIONS_SET: {
            if (mympd_state->mpd_state->conn_state != MPD_CONNECTED) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Can't set playback options: MPD not connected");
                break;
            }
            if (json_iterate_object(request->data, "$.params", mympd_api_settings_mpd_options_set, mympd_state, NULL, 100, &error) == true) {
                if (mympd_state->jukebox_mode != JUKEBOX_OFF) {
                    //start jukebox
                    mpd_client_jukebox(mympd_state);
                }
                //respond with ok
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_MPD);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Can't set playback options");
            }
            break;
        }
        case MYMPD_API_SETTINGS_GET:
            response->data = mympd_api_settings_get(mympd_state, response->data, request->id);
            break;
        case MYMPD_API_CONNECTION_SAVE: {
            sds old_mpd_settings = sdscatfmt(sdsempty(), "%S%i%S", mympd_state->mpd_state->mpd_host, mympd_state->mpd_state->mpd_port, mympd_state->mpd_state->mpd_pass);

            if (json_iterate_object(request->data, "$.params", mympd_api_settings_connection_save, mympd_state, NULL, 100, &error) == true) {
                sds new_mpd_settings = sdscatfmt(sdsempty(), "%S%i%S", mympd_state->mpd_state->mpd_host, mympd_state->mpd_state->mpd_port, mympd_state->mpd_state->mpd_pass);
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
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_MPD);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Can't save settings");
            }
            FREE_SDS(old_mpd_settings);
            break;
        }
        case MYMPD_API_COVERCACHE_CROP:
            int_buf1 = covercache_clear(config->cachedir, mympd_state->covercache_keep_days);
            if (int_buf1 >= 0) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_INFO, "Successfully croped covercache");
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Error in cropping covercache");
            }
            break;
        case MYMPD_API_COVERCACHE_CLEAR:
            int_buf1 = covercache_clear(config->cachedir, 0);
            if (int_buf1 >= 0) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_INFO, "Successfully cleared covercache");
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Error in clearing the covercache");
            }
            break;
        case MYMPD_API_TIMER_SAVE: {
            if (mympd_state->timer_list.length > LIST_TIMER_MAX) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_TIMER, JSONRPC_SEVERITY_ERROR, "Too many timers defined");
                break;
            }
            struct t_timer_definition *timer_def = malloc_assert(sizeof(struct t_timer_definition));
            timer_def = mympd_api_timer_parse(timer_def, request->data, &error);
            if (timer_def != NULL &&
                json_get_int(request->data, "$.params.interval", -1, TIMER_INTERVAL_MAX, &int_buf2, &error) == true &&
                json_get_int(request->data, "$.params.timerid", 0, USER_TIMER_ID_MAX, &int_buf1, &error) == true)
            {
                if (int_buf1 == 0) {
                    mympd_state->timer_list.last_id++;
                    int_buf1 = mympd_state->timer_list.last_id;
                }
                else if (int_buf1 < USER_TIMER_ID_MIN) {
                    //user defined timers must be gt 100
                    MYMPD_LOG_ERROR("Timer id must be greater or equal %d, but id is: \"%d\"", USER_TIMER_ID_MAX, int_buf1);
                    error = sdscat(error, "Invalid timer id");
                    break;
                }
                if (int_buf2 > 0 && int_buf2 < TIMER_INTERVAL_MIN) {
                    //interval must be gt 5 seconds
                    MYMPD_LOG_ERROR("Timer interval must be greater or equal %d, but id is: \"%d\"", TIMER_INTERVAL_MIN, int_buf2);
                    error = sdscat(error, "Invalid timer id");
                    break;
                }

                time_t start = mympd_api_timer_calc_starttime(timer_def->start_hour, timer_def->start_minute, int_buf2);
                rc = mympd_api_timer_replace(&mympd_state->timer_list, start, int_buf2, timer_handler_select, int_buf1, timer_def);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_TIMER);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_TIMER, JSONRPC_SEVERITY_ERROR, "Adding timer failed");
                    mympd_api_timer_free_definition(timer_def);
                }
            }
            else if (timer_def != NULL) {
                MYMPD_LOG_ERROR("No timer id or interval, discarding timer definition");
                mympd_api_timer_free_definition(timer_def);
            }
            break;
        }
        case MYMPD_API_TIMER_LIST:
            response->data = mympd_api_timer_list(&mympd_state->timer_list, response->data, request->id);
            break;
        case MYMPD_API_TIMER_GET:
            if (json_get_int(request->data, "$.params.timerid", USER_TIMER_ID_MIN, USER_TIMER_ID_MAX, &int_buf1, &error) == true) {
                response->data = mympd_api_timer_get(&mympd_state->timer_list, response->data, request->id, int_buf1);
            }
            break;
        case MYMPD_API_TIMER_RM:
            if (json_get_int(request->data, "$.params.timerid", USER_TIMER_ID_MIN, USER_TIMER_ID_MAX, &int_buf1, &error) == true) {
                mympd_api_timer_remove(&mympd_state->timer_list, int_buf1);
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_TIMER);
            }
            break;
        case MYMPD_API_TIMER_TOGGLE:
            if (json_get_int(request->data, "$.params.timerid", USER_TIMER_ID_MIN, USER_TIMER_ID_MAX, &int_buf1, &error) == true) {
                mympd_api_timer_toggle(&mympd_state->timer_list, int_buf1);
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_TIMER);
            }
            break;
        case MYMPD_API_LYRICS_GET:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mympd_api_lyrics_get(&mympd_state->lyrics, mympd_state->music_directory_value, response->data, request->id, sds_buf1);
            }
            break;
        case INTERNAL_API_STATE_SAVE:
            mympd_api_last_played_file_save(&mympd_state->last_played, mympd_state->last_played_count, mympd_state->config->workdir);
            mympd_api_trigger_file_save(&mympd_state->trigger_list, mympd_state->config->workdir);
            mympd_api_home_file_save(&mympd_state->home_list, mympd_state->config->workdir);
            mympd_api_timer_file_save(&mympd_state->timer_list, mympd_state->config->workdir);
            response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_GENERAL);
            break;
        case MYMPD_API_JUKEBOX_RM:
            if (json_get_long(request->data, "$.params.pos", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true) {
                rc = mpd_client_rm_jukebox_entry(&mympd_state->jukebox_queue, long_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_JUKEBOX);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_ERROR, "Could not remove song from jukebox queue");
                }
            }
            break;
        case MYMPD_API_JUKEBOX_CLEAR:
            mpd_client_clear_jukebox(&mympd_state->jukebox_queue);
            response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_JUKEBOX);
            break;
        case MYMPD_API_JUKEBOX_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                response->data = mpd_client_get_jukebox_list(mympd_state, response->data, request->cmd_id, request->id, long_buf1, long_buf2, sds_buf1, &tagcols);
            }
            break;
        }
        case MYMPD_API_TRIGGER_LIST:
            response->data = mympd_api_trigger_list(&mympd_state->trigger_list, response->data, request->id);
            break;
        case MYMPD_API_TRIGGER_GET:
            if (json_get_long(request->data, "$.params.id", 0, LIST_TRIGGER_MAX, &long_buf1, &error) == true) {
                response->data = mympd_api_trigger_get(&mympd_state->trigger_list, response->data, request->id, long_buf1);
            }
            break;
        case MYMPD_API_TRIGGER_SAVE: {
            if (mympd_state->trigger_list.length > LIST_TRIGGER_MAX) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_TRIGGER, JSONRPC_SEVERITY_ERROR, "Too many triggers defined");
                break;
            }
            //malloc list - it is used in trigger list
            struct t_list *arguments = list_new();

            if (json_get_string(request->data, "$.params.name", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.script", 0, FILENAME_LEN_MAX, &sds_buf2, vcb_isfilename, &error) == true &&
                json_get_int(request->data, "$.params.id", -1, LIST_TRIGGER_MAX, &int_buf1, &error) == true &&
                json_get_int_max(request->data, "$.params.event", &int_buf2, &error) == true &&
                json_get_object_string(request->data, "$.params.arguments", arguments, vcb_isname, 10, &error) == true)
            {
                rc = list_push(&mympd_state->trigger_list, sds_buf1, int_buf2, sds_buf2, arguments);
                if (rc == true) {
                    if (int_buf1 >= 0) {
                        //delete old entry
                        mympd_api_trigger_delete(&mympd_state->trigger_list, int_buf1);
                    }
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_TRIGGER);
                    break;
                }
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_TRIGGER, JSONRPC_SEVERITY_ERROR, "Could not save trigger");
            }
            list_clear(arguments);
            FREE_PTR(arguments);
            break;
        }
        case MYMPD_API_TRIGGER_RM:
            if (json_get_long(request->data, "$.params.id", 0, LIST_TRIGGER_MAX, &long_buf1, &error) == true) {
                rc = mympd_api_trigger_delete(&mympd_state->trigger_list, long_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_TRIGGER);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_TRIGGER, JSONRPC_SEVERITY_ERROR, "Could not delete trigger");
                }
            }
            break;
        case INTERNAL_API_SCRIPT_INIT: {
            struct t_list *lua_mympd_state = list_new();
            rc = mympd_api_status_lua_mympd_state_set(mympd_state, lua_mympd_state);
            if (rc == true) {
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_SCRIPT);
                response->extra = lua_mympd_state;
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Error getting mympd state for script execution");
                MYMPD_LOG_ERROR("Error getting mympd state for script execution");
                FREE_PTR(lua_mympd_state);
            }
            break;
        }
        case MYMPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET: {
            struct t_list attributes;
            list_init(&attributes);
            if (json_get_uint(request->data, "$.params.outputId", 0, MPD_OUTPUT_ID_MAX, &uint_buf1, &error) == true &&
                json_get_object_string(request->data, "$.params.attributes", &attributes, vcb_isalnum, 10, &error) == true)
            {
                struct t_list_node *current = attributes.head;
                while (current != NULL) {
                    rc = mpd_run_output_set(mympd_state->mpd_state->conn, uint_buf1, current->key, current->value_p);
                    response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_output_set", &result);
                    if (result == false) {
                        break;
                    }
                    current = current->next;
                }
            }
            list_clear(&attributes);
            break;
        }
        case INTERNAL_API_STICKERCACHE_CREATED:
            if (request->extra != NULL) {
                sticker_cache_free(&mympd_state->sticker_cache);
                mympd_state->sticker_cache.cache = (rax *) request->extra;
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_STICKER);
                MYMPD_LOG_INFO("Sticker cache was replaced");
            }
            else {
                MYMPD_LOG_ERROR("Sticker cache is NULL");
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "Sticker cache is NULL");
            }
            mympd_state->sticker_cache.building = false;
            break;
        case INTERNAL_API_ALBUMCACHE_CREATED:
            if (request->extra != NULL) {
                //first clear the jukebox queue - it has references to the album cache
                MYMPD_LOG_INFO("Clearing jukebox queue");
                mpd_client_clear_jukebox(&mympd_state->jukebox_queue);
                //free the old album cache and replace it with the freshly generated one
                album_cache_free(&mympd_state->album_cache);
                mympd_state->album_cache.cache = (rax *) request->extra;
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_DATABASE);
                MYMPD_LOG_INFO("Album cache was replaced");
                //send notification
                sds buffer = jsonrpc_event(sdsempty(), JSONRPC_EVENT_UPDATE_ALBUM_CACHE);
                ws_notify(buffer);
                FREE_SDS(buffer);
            }
            else {
                MYMPD_LOG_ERROR("Album cache is NULL");
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Album cache is NULL");
            }
            mympd_state->album_cache.building = false;
            break;
        case MYMPD_API_MESSAGE_SEND:
            if (json_get_string(request->data, "$.params.channel", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.message", 1, CONTENT_LEN_MAX, &sds_buf2, vcb_isname, &error) == true)
            {
                bool_buf1 = mpd_run_send_message(mympd_state->mpd_state->conn, sds_buf1, sds_buf2);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, bool_buf1, "mpd_run_send_message", &result);
            }
            break;
        case MYMPD_API_LIKE:
            if (mympd_state->mpd_state->feat_mpd_stickers == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "MPD stickers are disabled");
                MYMPD_LOG_ERROR("MPD stickers are disabled");
                break;
            }
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true &&
                json_get_int(request->data, "$.params.like", 0, 2, &int_buf1, &error) == true)
            {
                rc = sticker_set_like(&mympd_state->sticker_queue, sds_buf1, int_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_STICKER);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "Failed to set like, unknown error");
                }
                //mympd_feedback trigger
                mympd_api_trigger_execute_feedback(&mympd_state->trigger_list, sds_buf1, int_buf1);
            }
            break;
        case MYMPD_API_PLAYER_STATE:
            response->data = mympd_api_status_get(mympd_state, response->data, request->id);
            break;
        case MYMPD_API_PLAYER_CLEARERROR:
            rc = mpd_run_clearerror(mympd_state->mpd_state->conn);
            response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_clearerror", &result);
            break;
        case MYMPD_API_DATABASE_UPDATE:
        case MYMPD_API_DATABASE_RESCAN: {
            long update_id = mympd_api_status_updatedb_id(mympd_state->mpd_state);
            if (update_id == -1) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error getting MPD status");
                break;
            }
            if (update_id > 0) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_INFO, "Database update already started");
                break;
            }
            if (json_get_string(request->data, "$.params.uri", 0, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                if (sdslen(sds_buf1) == 0) {
                    //path should be NULL to scan root directory
                    FREE_SDS(sds_buf1);
                    sds_buf1 = NULL;
                }
                if (request->cmd_id == MYMPD_API_DATABASE_UPDATE) {
                    uint_buf1 = mpd_run_update(mympd_state->mpd_state->conn, sds_buf1);
                    response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, (uint_buf1 == 0 ? false : true), "mpd_run_update", &result);
                }
                else {
                    uint_buf1 = mpd_run_rescan(mympd_state->mpd_state->conn, sds_buf1);
                    response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, (uint_buf1 == 0 ? false : true), "mpd_run_rescan", &result);
                }
            }
            break;
        }
        case MYMPD_API_SMARTPLS_STICKER_SAVE:
        case MYMPD_API_SMARTPLS_NEWEST_SAVE:
        case MYMPD_API_SMARTPLS_SEARCH_SAVE:
            if (mympd_state->mpd_state->feat_mpd_smartpls == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Smart playlists are not supported");
                break;
            }
            rc = false;
            if (request->cmd_id == MYMPD_API_SMARTPLS_STICKER_SAVE) {
                if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                    json_get_string(request->data, "$.params.sticker", 1, NAME_LEN_MAX, &sds_buf2, vcb_isalnum, &error) == true &&
                    json_get_int(request->data, "$.params.maxentries", 0, MPD_PLAYLIST_LENGTH_MAX, &int_buf1, &error) == true &&
                    json_get_int(request->data, "$.params.minvalue", 0, 100, &int_buf2, &error) == true &&
                    json_get_string(request->data, "$.params.sort", 0, 100, &sds_buf3, vcb_ismpdsort, &error) == true)
                {
                    rc = smartpls_save_sticker(config->workdir, sds_buf1, sds_buf2, int_buf1, int_buf2, sds_buf3);
                }
            }
            else if (request->cmd_id == MYMPD_API_SMARTPLS_NEWEST_SAVE) {
                if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                    json_get_int(request->data, "$.params.timerange", 0, JSONRPC_INT_MAX, &int_buf1, &error) == true &&
                    json_get_string(request->data, "$.params.sort", 0, 100, &sds_buf2, vcb_ismpdsort, &error) == true)
                {
                    rc = smartpls_save_newest(config->workdir, sds_buf1, int_buf1, sds_buf2);
                }
            }
            else if (request->cmd_id == MYMPD_API_SMARTPLS_SEARCH_SAVE) {
                if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                    json_get_string(request->data, "$.params.expression", 1, EXPRESSION_LEN_MAX, &sds_buf2, vcb_isname, &error) == true &&
                    json_get_string(request->data, "$.params.sort", 0, 100, &sds_buf3, vcb_ismpdsort, &error) == true)
                {
                    rc = smartpls_save_search(config->workdir, sds_buf1, sds_buf2, sds_buf3);
                }
            }
            if (rc == true) {
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_PLAYLIST);
                //update currently saved smart playlist
                smartpls_update(sds_buf1);
            }
            break;
        case MYMPD_API_SMARTPLS_GET:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                response->data = mympd_api_smartpls_get(config->workdir, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_PLAYER_PAUSE:
            rc = mpd_run_pause(mympd_state->mpd_state->conn, true);
            response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_pause", &result);
            break;
        case MYMPD_API_PLAYER_RESUME:
            rc = mpd_run_pause(mympd_state->mpd_state->conn, false);
            response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_pause", &result);
            break;
        case MYMPD_API_PLAYER_PREV:
            rc = mpd_run_previous(mympd_state->mpd_state->conn);
            response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_previous", &result);
            break;
        case MYMPD_API_PLAYER_NEXT:
            rc = mpd_run_next(mympd_state->mpd_state->conn);
            response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_next", &result);
            break;
        case MYMPD_API_PLAYER_PLAY:
            rc = mpd_run_play(mympd_state->mpd_state->conn);
            response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_play", &result);
            break;
        case MYMPD_API_PLAYER_STOP:
            rc = mpd_run_stop(mympd_state->mpd_state->conn);
            response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_stop", &result);
            break;
        case MYMPD_API_QUEUE_CLEAR:
            rc = mpd_run_clear(mympd_state->mpd_state->conn);
            response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_clear", &result);
            break;
        case MYMPD_API_QUEUE_CROP:
            response->data = mympd_api_queue_crop(mympd_state, response->data, request->cmd_id, request->id, false);
            break;
        case MYMPD_API_QUEUE_CROP_OR_CLEAR:
            response->data = mympd_api_queue_crop(mympd_state, response->data, request->cmd_id, request->id, true);
            break;
        case MYMPD_API_QUEUE_RM_SONG:
            if (json_get_uint_max(request->data, "$.params.songId", &uint_buf1, &error) == true) {
                rc = mpd_run_delete_id(mympd_state->mpd_state->conn, uint_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_delete_id", &result);
            }
            break;
        case MYMPD_API_QUEUE_RM_RANGE:
            if (json_get_uint(request->data, "$.params.start", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_int(request->data, "$.params.end", -1, MPD_PLAYLIST_LENGTH_MAX, &int_buf1, &error) == true)
            {
                //map -1 to UINT_MAX for open ended range
                uint_buf2 = int_buf1 < 0 ? UINT_MAX : (unsigned)int_buf1;
                rc = mpd_run_delete_range(mympd_state->mpd_state->conn, uint_buf1, uint_buf2);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_delete_range", &result);
            }
            break;
        case MYMPD_API_QUEUE_MOVE_SONG:
            if (json_get_uint(request->data, "$.params.from", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf2, &error) == true)
            {
                if (uint_buf1 < uint_buf2) {
                    uint_buf2--;
                }
                rc = mpd_run_move(mympd_state->mpd_state->conn, uint_buf1, uint_buf2);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_move", &result);
            }
            break;
        case MYMPD_API_QUEUE_PRIO_SET:
            if (json_get_uint_max(request->data, "$.params.songId", &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.priority", 0, MPD_QUEUE_PRIO_MAX, &uint_buf2, &error) == true)
            {
                rc = mympd_api_queue_prio_set(mympd_state, uint_buf1, uint_buf2);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_QUEUE);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Failed to set song priority");
                }
            }
            break;
        case MYMPD_API_QUEUE_PRIO_SET_HIGHEST:
            if (json_get_uint_max(request->data, "$.params.songId", &uint_buf1, &error) == true) {
                rc = mympd_api_queue_prio_set_highest(mympd_state, uint_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_QUEUE);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Failed to set song priority");
                }
            }
            break;
        case MYMPD_API_PLAYER_PLAY_SONG:
            if (json_get_uint_max(request->data, "$.params.songId", &uint_buf1, &error) == true) {
                rc = mpd_run_play_id(mympd_state->mpd_state->conn, uint_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_play_id", &result);
            }
            break;
        case MYMPD_API_PLAYER_OUTPUT_LIST:
            if (json_get_string(request->data, "$.params.partition", 0, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true) {
                if (sdslen(sds_buf1) == 0) {
                    response->data = mympd_api_output_list(mympd_state->mpd_state, response->data, request->id);
                }
                else {
                    response->data = mympd_api_partition_output_list(mympd_state->mpd_state, response->data, request->id, sds_buf1);
                }
            }
            break;
        case MYMPD_API_PLAYER_OUTPUT_TOGGLE:
            if (json_get_uint(request->data, "$.params.outputId", 0, MPD_OUTPUT_ID_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.state", 0, 1, &uint_buf2, &error) == true)
            {
                if (uint_buf2 == 1) {
                    rc = mpd_run_enable_output(mympd_state->mpd_state->conn, uint_buf1);
                    response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_enable_output", &result);
                }
                else {
                    rc = mpd_run_disable_output(mympd_state->mpd_state->conn, uint_buf1);
                    response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_disable_output", &result);
                }
            }
            break;
        case MYMPD_API_PLAYER_VOLUME_SET:
            if (json_get_uint(request->data, "$.params.volume", 0, 100, &uint_buf1, &error) == true) {
                if (uint_buf1 > mympd_state->volume_max || uint_buf1 < mympd_state->volume_min) {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYER, JSONRPC_SEVERITY_ERROR, "Invalid volume level");
                }
                else {
                    rc = mpd_run_set_volume(mympd_state->mpd_state->conn, uint_buf1);
                    response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_set_volume", &result);
                }
            }
            break;
        case MYMPD_API_PLAYER_VOLUME_GET:
            response->data = mympd_api_status_volume_get(mympd_state->mpd_state, response->data, request->id);
            break;
        case MYMPD_API_PLAYER_SEEK_CURRENT:
            if (json_get_int_max(request->data, "$.params.seek", &int_buf1, &error) == true &&
                json_get_bool(request->data, "$.params.relative", &bool_buf1, &error) == true)
            {
                rc = mpd_run_seek_current(mympd_state->mpd_state->conn, (float)int_buf1, bool_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_seek_current", &result);
            }
            break;
        case MYMPD_API_QUEUE_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                response->data = mympd_api_queue_list(mympd_state, response->data, request->id, long_buf1, long_buf2, &tagcols);
            }
            break;
        }
        case MYMPD_API_LAST_PLAYED_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                response->data = mympd_api_last_played_list(mympd_state, response->data, request->id, long_buf1, long_buf2, sds_buf1, &tagcols);
            }
            break;
        }
        case MYMPD_API_PLAYER_CURRENT_SONG: {
            response->data = mympd_api_status_current_song(mympd_state, response->data, request->id);
            break;
        }
        case MYMPD_API_SONG_DETAILS:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mympd_api_song_details(mympd_state, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_SONG_COMMENTS:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mympd_api_song_comments(mympd_state, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_SONG_FINGERPRINT:
            if (mympd_state->mpd_state->feat_mpd_fingerprint == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Fingerprint command not supported");
                break;
            }
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mympd_api_song_fingerprint(mympd_state, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_PLAYLIST_RENAME:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.newName", 1, FILENAME_LEN_MAX, &sds_buf2, vcb_isfilename, &error) == true)
            {
                response->data = mympd_api_playlist_rename(mympd_state, response->data, request->id, sds_buf1, sds_buf2);
            }
            break;
        case MYMPD_API_PLAYLIST_RM:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_bool(request->data, "$.params.smartplsOnly", &bool_buf1, &error) == true)
            {
                response->data = mympd_api_playlist_delete(mympd_state, response->data, request->id, sds_buf1, bool_buf1);
            }
            break;
        case MYMPD_API_PLAYLIST_RM_ALL:
            if (json_get_string(request->data, "$.params.type", 1, NAME_LEN_MAX, &sds_buf1, vcb_isalnum, &error) == true) {
                response->data = mympd_api_playlist_delete_all(mympd_state, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_PLAYLIST_LIST:
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_uint(request->data, "$.params.type", 0, 2, &uint_buf1, &error) == true)
            {
                response->data = mympd_api_playlist_list(mympd_state, response->data, request->cmd_id, long_buf1, long_buf2, sds_buf1, uint_buf1);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, NAME_LEN_MAX, &sds_buf2, vcb_isname, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                response->data = mympd_api_playlist_content_list(mympd_state, response->data, request->id, sds_buf1, long_buf1, long_buf2, sds_buf2, &tagcols);
            }
            break;
        }
        case MYMPD_API_PLAYLIST_CONTENT_APPEND_URI:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf2, vcb_isuri, &error) == true)
            {
                rc = mpd_run_playlist_add(mympd_state->mpd_state->conn, sds_buf1, sds_buf2);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_playlist_add", &result);
                if (result == true) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Updated the playlist %{playlist}", 2, "playlist", sds_buf1);
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_INSERT_URI:
            if (mympd_state->mpd_state->feat_mpd_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf2, vcb_isuri, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true)
            {
                rc = mpd_run_playlist_add_to(mympd_state->mpd_state->conn, sds_buf1, sds_buf2, uint_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_playlist_add_to", &result);
                if (result == true) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Updated the playlist %{playlist}", 2, "playlist", sds_buf1);
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_REPLACE_URI:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf2, vcb_isuri, &error) == true)
            {
                rc = mpd_run_playlist_clear(mympd_state->mpd_state->conn, sds_buf1);
                if (rc == false) {
                    response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_playlist_clear", &result);
                    break;
                }
                rc = mpd_run_playlist_add(mympd_state->mpd_state->conn, sds_buf1, sds_buf2);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_playlist_add", &result);
                if (result == true) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Replaced the playlist %{playlist}", 2, "playlist", sds_buf1);
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_INSERT_SEARCH:
            if (mympd_state->mpd_state->feat_mpd_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf2, vcb_isname, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true)
            {
                result = mpd_client_search_add_to_plist(mympd_state->mpd_state, sds_buf2, sds_buf1, uint_buf1);
                if (result == true) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Updated the playlist %{playlist}", 2, "playlist", sds_buf1);
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_REPLACE_SEARCH:
        case MYMPD_API_PLAYLIST_CONTENT_APPEND_SEARCH:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf2, vcb_isname, &error) == true)
            {
                if (request->cmd_id == MYMPD_API_PLAYLIST_CONTENT_REPLACE_SEARCH) {
                    rc = mpd_run_playlist_clear(mympd_state->mpd_state->conn, sds_buf1);
                    if (rc == false) {
                        response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_playlist_clear", &result);
                        break;
                    }
                }
                result = mpd_client_search_add_to_plist(mympd_state->mpd_state, sds_buf2, sds_buf1, UINT_MAX);
                if (result == true) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Updated the playlist %{playlist}", 2, "playlist", sds_buf1);
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_CLEAR:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mpd_run_playlist_clear(mympd_state->mpd_state->conn, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_playlist_clear", &result);
                if (result == true) {
                    response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Cleared the playlist %{playlist}", 2, "playlist", sds_buf1);
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_MOVE_SONG:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.from", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf2, &error) == true)
            {
                if (uint_buf1 < uint_buf2) {
                    uint_buf2--;
                }
                rc = mpd_run_playlist_move(mympd_state->mpd_state->conn, sds_buf1, uint_buf1, uint_buf2);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_playlist_move", &result);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_RM_SONG:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.pos", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true)
            {
                rc = mpd_run_playlist_delete(mympd_state->mpd_state->conn, sds_buf1, uint_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_playlist_delete", &result);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_RM_RANGE:
            if (mympd_state->mpd_state->feat_mpd_playlist_rm_range == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.start", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_int(request->data, "$.params.end", -1, MPD_PLAYLIST_LENGTH_MAX, &int_buf1, &error) == true)
            {
                //map -1 to UINT_MAX for open ended range
                uint_buf2 = int_buf1 < 0 ? UINT_MAX : (unsigned)int_buf1;
                rc = mpd_run_playlist_delete_range(mympd_state->mpd_state->conn, sds_buf1, uint_buf1, uint_buf2);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_playlist_delete_range", &result);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_SHUFFLE:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mpd_client_playlist_shuffle(mympd_state->mpd_state, sds_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Shuffled playlist succesfully");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Shuffling playlist failed");
                }
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_SORT:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.tag", 1, NAME_LEN_MAX, &sds_buf2, vcb_ismpdtag, &error) == true)
            {
                rc = mpd_client_playlist_sort(mympd_state->mpd_state, sds_buf1, sds_buf2);
                if (rc == true) {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Sorted playlist succesfully");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Sorting playlist failed");
                }
            }
            break;
        case MYMPD_API_DATABASE_FILESYSTEM_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.path", 1, FILEPATH_LEN_MAX, &sds_buf2, vcb_isfilepath, &error) == true &&
                json_get_string(request->data, "$.params.type", 1, 5, &sds_buf3, vcb_isalnum, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                if (strcmp(sds_buf3, "plist") == 0) {
                    response->data = mympd_api_playlist_content_list(mympd_state, response->data, request->id, sds_buf2, long_buf1, long_buf2, sds_buf1, &tagcols);
                }
                else {
                    response->data = mympd_api_browse_filesystem(mympd_state, response->data, request->id, sds_buf2, long_buf1, long_buf2, sds_buf1, &tagcols);
                }
            }
            break;
        }
        case MYMPD_API_QUEUE_INSERT_URI:
            if (mympd_state->mpd_state->feat_mpd_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isuri, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.whence", 0, 2, &uint_buf2, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = mpd_run_add_whence(mympd_state->mpd_state->conn, sds_buf1, uint_buf1, uint_buf2);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_add_whence", &result);
                if (result == true &&
                    check_start_play(mympd_state, bool_buf1, &response->data, request->cmd_id, request->id) == true)
                {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_INFO, "Updated the queue");
                }
            }
            break;
        case MYMPD_API_QUEUE_REPLACE_URI:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isuri, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = mympd_api_queue_replace_with_song(mympd_state, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mympd_api_queue_replace_with_song", &result);
                if (result == true &&
                    check_start_play(mympd_state, bool_buf1, &response->data, request->cmd_id, request->id) == true)
                {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_INFO, "Replaced the queue");
                }
            }
            break;
        case MYMPD_API_QUEUE_APPEND_URI:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isuri, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = mpd_run_add(mympd_state->mpd_state->conn, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_add", &result);
                if (result == true &&
                    check_start_play(mympd_state, bool_buf1, &response->data, request->cmd_id, request->id) == true)
                {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_INFO, "Updated the queue");
                }
            }
            break;
        case MYMPD_API_QUEUE_APPEND_PLAYLIST:
            if (json_get_string(request->data, "$.params.plist", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isuri, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                sds_buf1 = resolv_mympd_uri(sds_buf1, mympd_state->mpd_state->mpd_host, config->http_host, config->http_port);
                rc = mpd_run_load(mympd_state->mpd_state->conn, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_load", &result);
                if (result == true &&
                    check_start_play(mympd_state, bool_buf1, &response->data, request->cmd_id, request->id) == true)
                {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_INFO, "Updated the queue");
                }
            }
            break;
        case MYMPD_API_QUEUE_INSERT_PLAYLIST:
            if (mympd_state->mpd_state->feat_mpd_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            if (json_get_string(request->data, "$.params.plist", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isuri, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.whence", 0, 2, &uint_buf2, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                sds_buf1 = resolv_mympd_uri(sds_buf1, mympd_state->mpd_state->mpd_host, config->http_host, config->http_port);
                rc = mpd_run_load_range_to(mympd_state->mpd_state->conn, sds_buf1, 0, UINT_MAX, uint_buf1, uint_buf2);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_load_range_to", &result);
                if (result == true &&
                    check_start_play(mympd_state, bool_buf1, &response->data, request->cmd_id, request->id) == true)
                {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_INFO, "Updated the queue");
                }
            }
            break;
        case MYMPD_API_QUEUE_REPLACE_PLAYLIST:
            if (json_get_string(request->data, "$.params.plist", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isuri, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                sds_buf1 = resolv_mympd_uri(sds_buf1, mympd_state->mpd_state->mpd_host, config->http_host, config->http_port);
                rc = mympd_api_queue_replace_with_playlist(mympd_state, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mympd_api_queue_replace_with_playlist", &result);
                if (result == true &&
                    check_start_play(mympd_state, bool_buf1, &response->data, request->cmd_id, request->id) == true)
                {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_INFO, "Replaced the queue");
                }
            }
            break;
        case MYMPD_API_QUEUE_INSERT_SEARCH:
            if (mympd_state->mpd_state->feat_mpd_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            if (json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.whence", 0, 2, &uint_buf2, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                result = mpd_client_search_add_to_queue(mympd_state->mpd_state, sds_buf1, uint_buf1, uint_buf2);
                if (result == true &&
                    check_start_play(mympd_state, bool_buf1, &response->data, request->cmd_id, request->id) == true)
                {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_INFO, "Updated the queue");
                }
            }
            break;
        case MYMPD_API_QUEUE_REPLACE_SEARCH:
        case MYMPD_API_QUEUE_APPEND_SEARCH:
            if (json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                if (request->cmd_id == MYMPD_API_QUEUE_REPLACE_SEARCH) {
                    rc = mpd_run_clear(mympd_state->mpd_state->conn);
                    if (rc == false) {
                        response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_clear", &result);
                        break;
                    }
                }
                result = mpd_client_search_add_to_queue(mympd_state->mpd_state, sds_buf1, UINT_MAX, MPD_POSITION_ABSOLUTE);
                if (result == true &&
                    check_start_play(mympd_state, bool_buf1, &response->data, request->cmd_id, request->id) == true)
                {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_INFO, "Updated the queue");
                }
            }
            break;
        case MYMPD_API_QUEUE_ADD_RANDOM:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.mode", 0, 2, &uint_buf1, &error) == true &&
                json_get_long(request->data, "$.params.quantity", 0, 1000, &long_buf1, &error) == true)
            {
                rc = mpd_client_jukebox_add_to_queue(mympd_state, long_buf1, uint_buf1, sds_buf1, true);
                if (rc == true) {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_INFO, "Successfully added random songs to queue");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Adding random songs to queue failed");
                }
            }
            break;
        case MYMPD_API_QUEUE_SAVE:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mpd_run_save(mympd_state->mpd_state->conn, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_save", &result);
            }
            break;
        case MYMPD_API_QUEUE_SEARCH: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.filter", 1, NAME_LEN_MAX, &sds_buf1, vcb_ismpdtag_or_any, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 1, NAME_LEN_MAX, &sds_buf2, vcb_isname, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                response->data = mympd_api_queue_search(mympd_state, response->data, request->id,
                    sds_buf1, long_buf1, long_buf2, sds_buf2, &tagcols);
            }
            break;
        }
        case MYMPD_API_QUEUE_SEARCH_ADV: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            if (json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.sort", 0, NAME_LEN_MAX, &sds_buf2, vcb_ismpdsort, &error) == true &&
                json_get_bool(request->data, "$.params.sortdesc", &bool_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.limit", 0, MPD_RESULTS_MAX, &uint_buf2, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                response->data = mympd_api_queue_search_adv(mympd_state, response->data, request->id,
                    sds_buf1, sds_buf2, bool_buf1, uint_buf1, uint_buf2, &tagcols);
            }
            break;
        }
        case MYMPD_API_DATABASE_SEARCH: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            if (json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.sort", 0, NAME_LEN_MAX, &sds_buf2, vcb_ismpdsort, &error) == true &&
                json_get_bool(request->data, "$.params.sortdesc", &bool_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.limit", 0, MPD_RESULTS_MAX, &uint_buf2, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                response->data = mpd_client_search_response(mympd_state->mpd_state, response->data, request->id,
                    sds_buf1, sds_buf2, bool_buf1, uint_buf1, uint_buf2, &tagcols, &mympd_state->sticker_cache, &result);
            }
            break;
        }
        case MYMPD_API_QUEUE_SHUFFLE:
            rc = mpd_run_shuffle(mympd_state->mpd_state->conn);
            response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_shuffle", &result);
            break;
        case MYMPD_API_DATABASE_STATS:
            response->data = mympd_api_stats_get(mympd_state, response->data, request->id);
            break;
        case INTERNAL_API_ALBUMART:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mympd_api_albumart_getcover(mympd_state, response->data, request->id, sds_buf1, &response->binary);
            }
            break;
        case MYMPD_API_DATABASE_ALBUMS_GET:
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.sort", 1, NAME_LEN_MAX, &sds_buf2, vcb_ismpdsort, &error) == true &&
                json_get_bool(request->data, "$.params.sortdesc", &bool_buf1, &error) == true)
            {
                response->data = mympd_api_browse_album_list(mympd_state, response->data, request->id,
                    sds_buf1, sds_buf2, bool_buf1, long_buf1, long_buf2);
            }
            break;
        case MYMPD_API_DATABASE_TAG_LIST:
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.tag", 1, NAME_LEN_MAX, &sds_buf2, vcb_ismpdtag_or_any, &error) == true &&
                json_get_bool(request->data, "$.params.sortdesc", &bool_buf1, &error) == true)
            {
                response->data = mympd_api_browse_tag_list(mympd_state, response->data, request->id,
                    sds_buf1, sds_buf2, long_buf1, long_buf2, bool_buf1);
            }
            break;
        case MYMPD_API_DATABASE_TAG_ALBUM_TITLE_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            struct t_list albumartists;
            list_init(&albumartists);
            if (json_get_string(request->data, "$.params.album", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_array_string(request->data, "$.params.albumartist", &albumartists, vcb_isname, 10, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                response->data = mympd_api_browse_album_songs(mympd_state, response->data, request->id, sds_buf1, &albumartists, &tagcols);
            }
            list_clear(&albumartists);
            break;
        }
        case INTERNAL_API_TIMER_STARTPLAY:
            if (json_get_uint(request->data, "$.params.volume", 0, 100, &uint_buf1, &error) == true &&
                json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.jukeboxMode", 0, 2, &uint_buf2, &error) == true)
            {
                rc = mympd_api_timer_startplay(mympd_state, uint_buf1, sds_buf1, uint_buf2);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_TIMER);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_TIMER, JSONRPC_SEVERITY_ERROR, "Error executing timer start play");
                }
            }
            break;
        case MYMPD_API_MOUNT_URLHANDLER_LIST:
            response->data = mympd_api_mounts_urlhandler_list(mympd_state, response->data, request->id);
            break;
        case MYMPD_API_PARTITION_LIST:
            response->data = mympd_api_partition_list(mympd_state, response->data, request->id);
            break;
        case MYMPD_API_PARTITION_NEW:
            if (json_get_string(request->data, "$.params.name", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true) {
                rc = mpd_run_newpartition(mympd_state->mpd_state->conn, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_newpartition", &result);
            }
            break;
        case MYMPD_API_PARTITION_SWITCH:
            if (json_get_string(request->data, "$.params.name", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true) {
                rc = mpd_run_switch_partition(mympd_state->mpd_state->conn, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_switch_partition", &result);
            }
            break;
        case MYMPD_API_PARTITION_RM:
            if (json_get_string(request->data, "$.params.name", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true) {
                rc = mpd_run_delete_partition(mympd_state->mpd_state->conn, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_delete_partition", &result);
            }
            break;
        case MYMPD_API_PARTITION_OUTPUT_MOVE: {
            struct t_list outputs;
            list_init(&outputs); 
            if (json_get_array_string(request->data, "$.params.outputs", &outputs, vcb_isname, 10, &error) == true) {
                struct t_list_node *current;
                while ((current = list_shift_first(&outputs)) != NULL) {
                    rc = mpd_run_move_output(mympd_state->mpd_state->conn, current->key);
                    list_node_free(current);
                    response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_move_output", &result);
                    if (result == false) {
                        break;
                    }
                }
            }
            list_clear(&outputs);
            break;
        }
        case MYMPD_API_MOUNT_LIST:
            response->data = mympd_api_mounts_list(mympd_state, response->data, request->id);
            break;
        case MYMPD_API_MOUNT_NEIGHBOR_LIST:
            response->data = mympd_api_mounts_neighbor_list(mympd_state, response->data, request->id);
            break;
        case MYMPD_API_MOUNT_MOUNT:
            if (json_get_string(request->data, "$.params.mountUrl", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isuri, &error) == true &&
                json_get_string(request->data, "$.params.mountPoint", 1, FILEPATH_LEN_MAX, &sds_buf2, vcb_isfilepath, &error) == true)
            {
                rc = mpd_run_mount(mympd_state->mpd_state->conn, sds_buf2, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_mount", &result);
            }
            break;
        case MYMPD_API_MOUNT_UNMOUNT:
            if (json_get_string(request->data, "$.params.mountPoint", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                rc = mpd_run_unmount(mympd_state->mpd_state->conn, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(mympd_state->mpd_state, response->data, request->cmd_id, request->id, rc, "mpd_run_unmount", &result);
            }
            break;
        case MYMPD_API_WEBRADIO_FAVORITE_LIST:
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true)
            {
                response->data = mympd_api_webradio_list(config->workdir, response->data, request->cmd_id, sds_buf1, long_buf1, long_buf2);
            }
            break;
        case MYMPD_API_WEBRADIO_FAVORITE_GET:
            if (json_get_string(request->data, "$.params.filename", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                response->data = mympd_api_webradio_get(config->workdir, response->data, request->cmd_id, sds_buf1);
            }
            break;
        case MYMPD_API_WEBRADIO_FAVORITE_SAVE:
            if (json_get_string(request->data, "$.params.name", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.streamUri", 1, FILEPATH_LEN_MAX, &sds_buf2, vcb_isuri, &error) == true &&
                json_get_string(request->data, "$.params.streamUriOld", 0, FILEPATH_LEN_MAX, &sds_buf3, vcb_isuri, &error) == true &&
                json_get_string(request->data, "$.params.genre", 0, FILENAME_LEN_MAX, &sds_buf4, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.image", 0, FILEPATH_LEN_MAX, &sds_buf5, vcb_isuri, &error) == true &&
                json_get_string(request->data, "$.params.homepage", 0, FILEPATH_LEN_MAX, &sds_buf6, vcb_isuri, &error) == true &&
                json_get_string(request->data, "$.params.country", 0, FILEPATH_LEN_MAX, &sds_buf7, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.language", 0, FILEPATH_LEN_MAX, &sds_buf8, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.codec", 0, FILEPATH_LEN_MAX, &sds_buf9, vcb_isalnum, &error) == true &&
                json_get_int(request->data, "$.params.bitrate", 0, 2048, &int_buf1, &error) == true &&
                json_get_string(request->data, "$.params.description", 0, CONTENT_LEN_MAX, &sds_buf0, vcb_isname, &error) == true
            ) {
                rc = mympd_api_webradio_save(config->workdir, sds_buf1, sds_buf2, sds_buf3, sds_buf4, sds_buf5, sds_buf6, sds_buf7,
                    sds_buf8, sds_buf9, int_buf1, sds_buf0);
                if (rc == true) {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_INFO, "Webradio favorite successfully saved");
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Could not save webradio favorite");
                }
            }
            break;
        case MYMPD_API_WEBRADIO_FAVORITE_RM:
            if (json_get_string(request->data, "$.params.filename", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mympd_api_webradio_delete(config->workdir, sds_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_DATABASE);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Could not delete webradio favorite");
                }
            }
            break;
        default:
            response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Unknown request");
            MYMPD_LOG_ERROR("Unknown API request: %.*s", (int)sdslen(request->data), request->data);
    }

    FREE_SDS(sds_buf1);
    FREE_SDS(sds_buf2);
    FREE_SDS(sds_buf3);
    FREE_SDS(sds_buf4);
    FREE_SDS(sds_buf5);
    FREE_SDS(sds_buf6);
    FREE_SDS(sds_buf7);
    FREE_SDS(sds_buf8);
    FREE_SDS(sds_buf9);
    FREE_SDS(sds_buf0);

    #ifdef DEBUG
    MEASURE_END
    MEASURE_PRINT(request->method)
    #endif

    if (async == true) {
        FREE_SDS(error);
        return;
    }

    if (sdslen(error) > 0) {
        response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
            JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, error);
        MYMPD_LOG_ERROR("Error processing method \"%s\"", request->method);
    }
    FREE_SDS(error);
    if (sdslen(response->data) == 0) {
        response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
            JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "No response for method %{method}", 2, "method", request->method);
        MYMPD_LOG_ERROR("No response for method \"%s\"", request->method);
    }
    if (request->conn_id == -2) {
        MYMPD_LOG_DEBUG("Push response to mympd_script_queue for thread %ld: %s", request->id, response->data);
        mympd_queue_push(mympd_script_queue, response, request->id);
    }
    else if (request->conn_id > -1) {
        MYMPD_LOG_DEBUG("Push response to web_server_queue for connection %lld: %s", request->conn_id, response->data);
        mympd_queue_push(web_server_queue, response, 0);
    }
    else {
        free_response(response);
    }
    free_request(request);
}

//private
static bool check_start_play(struct t_mympd_state *mympd_state, bool play, sds *buffer, enum mympd_cmd_ids cmd_id, long id) {
    if (play == true) {
        MYMPD_LOG_DEBUG("Start playing newly added songs");
        bool rc = mympd_api_queue_play_newly_inserted(mympd_state);
        if (rc == false) {
            *buffer = jsonrpc_respond_message(*buffer, cmd_id, id,
                JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Start playing newly added song failed");
        }
        return rc;
    }
    return true;
}
