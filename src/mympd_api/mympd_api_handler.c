/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/mympd_api_handler.h"

#include "src/lib/album_cache.h"
#include "src/lib/api.h"
#include "src/lib/covercache.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/list.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/lib/smartpls.h"
#include "src/lib/sticker_cache.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"
#include "src/mpd_client/connection.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/features.h"
#include "src/mpd_client/jukebox.h"
#include "src/mpd_client/partitions.h"
#include "src/mpd_client/playlists.h"
#include "src/mpd_client/presets.h"
#include "src/mpd_client/queue.h"
#include "src/mpd_worker/mpd_worker.h"
#include "src/mympd_api/albumart.h"
#include "src/mympd_api/browse.h"
#include "src/mympd_api/database.h"
#include "src/mympd_api/filesystem.h"
#include "src/mympd_api/home.h"
#include "src/mympd_api/jukebox.h"
#include "src/mympd_api/last_played.h"
#include "src/mympd_api/lyrics.h"
#include "src/mympd_api/mounts.h"
#include "src/mympd_api/outputs.h"
#include "src/mympd_api/partitions.h"
#include "src/mympd_api/pictures.h"
#include "src/mympd_api/playlists.h"
#include "src/mympd_api/queue.h"
#include "src/mympd_api/scripts.h"
#include "src/mympd_api/search.h"
#include "src/mympd_api/settings.h"
#include "src/mympd_api/smartpls.h"
#include "src/mympd_api/song.h"
#include "src/mympd_api/stats.h"
#include "src/mympd_api/status.h"
#include "src/mympd_api/timer.h"
#include "src/mympd_api/timer_handlers.h"
#include "src/mympd_api/trigger.h"
#include "src/mympd_api/volume.h"
#include "src/mympd_api/webradios.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * Central myMPD api handler function
 * @param partition_state pointer to partition state
 * @param request pointer to the jsonrpc request struct
 */
void mympd_api_handler(struct t_partition_state *partition_state, struct t_work_request *request) {
    //some buffer variables
    unsigned uint_buf1;
    unsigned uint_buf2;
    long long_buf1;
    long long_buf2;
    int int_buf1;
    int int_buf2;
    bool bool_buf1;
    bool rc;
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

    #ifdef MYMPD_DEBUG
        MEASURE_INIT
        MEASURE_START
    #endif

    const char *method = get_cmd_id_method_name(request->cmd_id);
    MYMPD_LOG_DEBUG(partition_state->name, "MYMPD API request (%lld)(%ld) %s: %s",
        request->conn_id, request->id, method, request->data);

    //shortcuts
    struct t_mympd_state *mympd_state = partition_state->mympd_state;
    struct t_config *config = mympd_state->config;

    //create response struct
    struct t_work_response *response = create_response(request);

    switch(request->cmd_id) {
        case MYMPD_API_LOGLEVEL:
            if (json_get_int(request->data, "$.params.loglevel", 0, 7, &int_buf1, &error) == true) {
                set_loglevel(int_buf1);
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_GENERAL);
            }
            break;
        case MYMPD_API_CACHES_CREATE:
        case MYMPD_API_PLAYLIST_CONTENT_DEDUP:
        case MYMPD_API_PLAYLIST_CONTENT_DEDUP_ALL:
        case MYMPD_API_PLAYLIST_CONTENT_SHUFFLE:
        case MYMPD_API_PLAYLIST_CONTENT_SORT:
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE:
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE_ALL:
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP:
        case MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP_ALL:
        case MYMPD_API_SMARTPLS_UPDATE:
        case MYMPD_API_SMARTPLS_UPDATE_ALL:
            if (worker_threads > 5) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Too many worker threads are already running");
                MYMPD_LOG_ERROR(partition_state->name, "Too many worker threads are already running");
                break;
            }
            if (request->cmd_id == MYMPD_API_CACHES_CREATE ||
                request->cmd_id == MYMPD_API_SMARTPLS_UPDATE_ALL)
            {
                if (json_get_bool(request->data, "$.params.force", &bool_buf1, NULL) == false) {
                    //enforce parameter
                    break;
                }
            }
            if (request->cmd_id == MYMPD_API_CACHES_CREATE) {
                if (mympd_state->mpd_state->album_cache.building == true ||
                    mympd_state->mpd_state->sticker_cache.building == true)
                {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_WARN, "Cache update is already running");
                    MYMPD_LOG_WARN(partition_state->name, "Cache update is already running");
                    break;
                }
                mympd_state->mpd_state->album_cache.building = mympd_state->mpd_state->feat_tags;
                mympd_state->mpd_state->sticker_cache.building = mympd_state->mpd_state->feat_stickers;
            }
            async = mpd_worker_start(mympd_state, request);
            if (async == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Error starting worker thread");
            }
            break;
        case INTERNAL_API_ALBUMCACHE_SKIPPED:
            mympd_state->mpd_state->album_cache.building = false;
            response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_DATABASE);
            break;
        case INTERNAL_API_STICKERCACHE_SKIPPED:
            mympd_state->mpd_state->sticker_cache.building = false;
            response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_STICKER);
            break;
        case INTERNAL_API_ALBUMCACHE_ERROR:
            mympd_state->mpd_state->album_cache.building = false;
            response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error creating album cache");
            break;
        case INTERNAL_API_STICKERCACHE_ERROR:
            mympd_state->mpd_state->sticker_cache.building = false;
            response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "Error creating sticker cache");
            break;
        case INTERNAL_API_STICKERCACHE_CREATED:
            if (request->extra != NULL) {
                sticker_cache_free(&mympd_state->mpd_state->sticker_cache);
                mympd_state->mpd_state->sticker_cache.cache = (rax *) request->extra;
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_STICKER);
                MYMPD_LOG_INFO(partition_state->name, "Sticker cache was replaced");
            }
            else {
                MYMPD_LOG_ERROR(partition_state->name, "Sticker cache is NULL");
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "Sticker cache is NULL");
            }
            mympd_state->mpd_state->sticker_cache.building = false;
            break;
        case INTERNAL_API_ALBUMCACHE_CREATED:
            if (request->extra != NULL) {
                //first clear the jukebox queues - it has references to the album cache
                MYMPD_LOG_INFO(partition_state->name, "Clearing jukebox queues");
                jukebox_clear_all(mympd_state);
                //free the old album cache and replace it with the freshly generated one
                album_cache_free(&mympd_state->mpd_state->album_cache);
                mympd_state->mpd_state->album_cache.cache = (rax *) request->extra;
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_DATABASE);
                MYMPD_LOG_INFO(partition_state->name, "Album cache was replaced");
            }
            else {
                MYMPD_LOG_ERROR(partition_state->name, "Album cache is NULL");
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Album cache is NULL");
            }
            mympd_state->mpd_state->album_cache.building = false;
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
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_HOME, "Can not save home icon");
                if (rc == true) {
                    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_HOME, MPD_PARTITION_ALL);
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
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_HOME, "Can not move home icon");
                if (rc == true) {
                    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_HOME, MPD_PARTITION_ALL);
                }
            }
            break;
        case MYMPD_API_HOME_ICON_RM:
            if (json_get_long(request->data, "$.params.pos", 0, LIST_HOME_ICONS_MAX, &long_buf1, &error) == true) {
                rc = mympd_api_home_icon_delete(&mympd_state->home_list, long_buf1);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_HOME, "Can not delete home icon");
                if (rc == true) {
                    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_HOME, MPD_PARTITION_ALL);
                }
            }
            break;
        case MYMPD_API_HOME_ICON_GET:
            if (json_get_long(request->data, "$.params.pos", 0, LIST_HOME_ICONS_MAX, &long_buf1, &error) == true) {
                response->data = mympd_api_home_icon_get(&mympd_state->home_list, response->data, request->id, long_buf1);
            }
            break;
        case MYMPD_API_HOME_ICON_LIST:
            response->data = mympd_api_home_icon_list(&mympd_state->home_list, response->data, request->id);
            break;
        #ifdef MYMPD_ENABLE_LUA
        case MYMPD_API_SCRIPT_VALIDATE:
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.content", 0, CONTENT_LEN_MAX, &sds_buf2, vcb_istext, &error) == true)
            {
                rc = mympd_api_script_validate(sds_buf1, sds_buf2, config->lualibs, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, error);
            }
        break;
        case MYMPD_API_SCRIPT_SAVE: {
            struct t_list arguments;
            list_init(&arguments);
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.oldscript", 0, FILENAME_LEN_MAX, &sds_buf2, vcb_isfilename, &error) == true &&
                json_get_int(request->data, "$.params.order", 0, LIST_TIMER_MAX, &int_buf1, &error) == true &&
                json_get_string(request->data, "$.params.content", 0, CONTENT_LEN_MAX, &sds_buf3, vcb_istext, &error) == true &&
                json_get_array_string(request->data, "$.params.arguments", &arguments, vcb_isalnum, SCRIPT_ARGUMENTS_MAX, &error) == true)
            {
                rc = mympd_api_script_validate(sds_buf1, sds_buf3, config->lualibs, &error) &&
                    mympd_api_script_save(config->workdir, sds_buf1, sds_buf2, int_buf1, sds_buf3, &arguments, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, error);
            }
            list_clear(&arguments);
            break;
        }
        case MYMPD_API_SCRIPT_RM:
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mympd_api_script_delete(config->workdir, sds_buf1);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, "Could not delete script");
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
                rc = mympd_api_script_start(config->workdir, sds_buf1, config->lualibs, arguments, partition_state->name, true);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, "Can't create mympd_script thread");
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Invalid script name");
                list_free(arguments);
                FREE_PTR(arguments);
            }
            break;
        }
        case INTERNAL_API_SCRIPT_POST_EXECUTE: {
            //malloc list - it is used in another thread
            struct t_list *arguments = list_new();
            if (json_get_string(request->data, "$.params.script", 1, CONTENT_LEN_MAX, &sds_buf1, vcb_istext, &error) == true &&
                json_get_object_string(request->data, "$.params.arguments", arguments, vcb_isname, 10, &error) == true)
            {
                rc = mympd_api_script_start(config->workdir, sds_buf1, config->lualibs, arguments, partition_state->name, false);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, "Can't create mympd_script thread");
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
                sds cols = sdsempty();
                cols = json_get_cols_as_string(request->data, cols, &rc);
                if (rc == true) {
                    rc = mympd_api_settings_cols_save(mympd_state, sds_buf1, cols);
                    response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                            JSONRPC_FACILITY_SCRIPT, "Could not save columns");
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
                if (partition_state->conn_state == MPD_CONNECTED) {
                    //feature detection
                    mpd_client_mpd_features(partition_state);
                }
                else {
                    settings_to_webserver(partition_state->mympd_state);
                }
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_GENERAL);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Can't save settings");
            }
            break;
        }
        case MYMPD_API_PLAYER_OPTIONS_SET: {
            if (partition_state->conn_state != MPD_CONNECTED) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Can't set playback options: MPD not connected");
                break;
            }
            if (json_iterate_object(request->data, "$.params", mympd_api_settings_mpd_options_set, partition_state, NULL, 100, &error) == true) {
                if (partition_state->jukebox_mode != JUKEBOX_OFF) {
                    // start jukebox
                    jukebox_run(partition_state);
                }
                // save options as preset if name is found and not empty
                if (json_find_key(request->data, "$.params.name") == true && // prevent warning message
                    json_get_string(request->data, "$.params.name", 0, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                    sdslen(sds_buf1) > 0)
                {
                    sds params = json_get_key_as_sds(request->data, "$.params");
                    if (params != NULL) {
                        int idx = list_get_node_idx(&partition_state->presets, sds_buf1);
                        if (idx > -1) {
                            list_replace(&partition_state->presets, idx, sds_buf1, 0, params, NULL);
                        }
                        else {
                            list_push(&partition_state->presets, sds_buf1, 0, params, NULL);
                        }
                        FREE_SDS(params);
                    }
                    else {
                        response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                                JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Can't save preset");
                    }
                }
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_MPD);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Can't set playback options");
            }
            break;
        }
        case MYMPD_API_PRESET_RM:
            if (json_get_string(request->data, "$.params.name", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true) {
                rc = presets_delete(&partition_state->presets, sds_buf1);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_MPD, "Could not delete preset");
            }
            break;
        case MYMPD_API_PRESET_LOAD:
            if (json_get_string(request->data, "$.params.name", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true) {
                struct t_list_node *preset = list_get_node(&partition_state->presets, sds_buf1);
                if (preset != NULL) {
                    if (json_iterate_object(preset->value_p, "$", mympd_api_settings_mpd_options_set, partition_state, NULL, 100, &error) == true) {
                        if (partition_state->jukebox_mode != JUKEBOX_OFF) {
                            //start jukebox
                            jukebox_run(partition_state);
                        }
                        //respond with ok
                        response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_MPD);
                    }
                    else {
                        response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                                JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Can't set playback options");
                    }
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Could not load preset");
                }
            }
            break;
        case MYMPD_API_SETTINGS_GET:
            response->data = mympd_api_settings_get(partition_state, response->data, request->id);
            break;
        case MYMPD_API_CONNECTION_SAVE: {
            sds old_mpd_settings = sdscatfmt(sdsempty(), "%S%i%S", mympd_state->mpd_state->mpd_host, mympd_state->mpd_state->mpd_port, mympd_state->mpd_state->mpd_pass);

            if (json_iterate_object(request->data, "$.params", mympd_api_settings_connection_save, mympd_state, NULL, 100, &error) == true) {
                sds new_mpd_settings = sdscatfmt(sdsempty(), "%S%i%S", mympd_state->mpd_state->mpd_host, mympd_state->mpd_state->mpd_port, mympd_state->mpd_state->mpd_pass);
                if (strcmp(old_mpd_settings, new_mpd_settings) != 0) {
                    //disconnect all partitions
                    MYMPD_LOG_DEBUG(partition_state->name, "MPD host has changed, disconnecting");
                    mpd_client_disconnect_all(mympd_state, MPD_DISCONNECTED);
                    //remove all but default partition
                    partitions_list_clear(mympd_state);
                    //remove caches
                    album_cache_remove(config->workdir);
                    sticker_cache_remove(config->workdir);
                }
                else if (partition_state->conn_state == MPD_CONNECTED) {
                    //feature detection
                    mpd_client_mpd_features(partition_state);
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
            int_buf1 = covercache_clear(config->cachedir, mympd_state->config->covercache_keep_days);
            rc = int_buf1 >= 0
                ? true
                : false;
            response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, 0,
                    JSONRPC_FACILITY_GENERAL, "Successfully croped covercache", "Error cropping the covercache");
            break;
        case MYMPD_API_COVERCACHE_CLEAR:
            int_buf1 = covercache_clear(config->cachedir, 0);
            rc = int_buf1 >= 0
                ? true
                : false;
            response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, 0,
                    JSONRPC_FACILITY_GENERAL, "Successfully cleared covercache", "Error clearing the covercache");
            break;
        case MYMPD_API_TIMER_SAVE: {
            rc = mympd_api_timer_save(partition_state, request->data, &error);
            response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                    JSONRPC_FACILITY_TIMER, "Timer saved successfully", error);
            break;
        }
        case MYMPD_API_TIMER_LIST:
            response->data = mympd_api_timer_list(&mympd_state->timer_list, response->data, request->id, partition_state->name);
            break;
        case MYMPD_API_TIMER_GET:
            if (json_get_int(request->data, "$.params.timerid", USER_TIMER_ID_MIN, USER_TIMER_ID_MAX, &int_buf1, &error) == true) {
                response->data = mympd_api_timer_get(&mympd_state->timer_list, response->data, request->id, int_buf1);
            }
            break;
        case MYMPD_API_TIMER_RM:
            if (json_get_int(request->data, "$.params.timerid", USER_TIMER_ID_MIN, USER_TIMER_ID_MAX, &int_buf1, &error) == true) {
                rc = mympd_api_timer_remove(&mympd_state->timer_list, int_buf1);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_TIMER, "Timer with given id not found");
            }
            break;
        case MYMPD_API_TIMER_TOGGLE:
            if (json_get_int(request->data, "$.params.timerid", USER_TIMER_ID_MIN, USER_TIMER_ID_MAX, &int_buf1, &error) == true) {
                rc = mympd_api_timer_toggle(&mympd_state->timer_list, int_buf1);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_TIMER, "Timer with given id not found");
            }
            break;
        case MYMPD_API_LYRICS_GET:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mympd_api_lyrics_get(&mympd_state->lyrics, mympd_state->mpd_state->music_directory_value, response->data, request->id, sds_buf1);
            }
            break;
        case INTERNAL_API_STATE_SAVE:
            mympd_state_save(mympd_state, false);
            response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_GENERAL);
            break;
        case MYMPD_API_JUKEBOX_RM: {
            struct t_list positions;
            list_init(&positions);
            if (json_get_array_llong(request->data, "$.params.positions", &positions, MPD_COMMANDS_MAX, &error) == true)
            {
                rc = mympd_api_jukebox_rm_entries(&partition_state->jukebox_queue, &positions, partition_state->name, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_JUKEBOX, "Could not remove song from jukebox queue");
            }
            list_clear(&positions);
            break;
        }
        case MYMPD_API_JUKEBOX_CLEAR:
            mympd_api_jukebox_clear(&partition_state->jukebox_queue, partition_state->name);
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
                response->data = mympd_api_jukebox_list(partition_state, response->data, request->cmd_id, request->id,
                        long_buf1, long_buf2, sds_buf1, &tagcols);
            }
            break;
        }
        case MYMPD_API_TRIGGER_LIST:
            response->data = mympd_api_trigger_list(&mympd_state->trigger_list, response->data, request->id, partition_state->name);
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
            //malloc trigger_data - it is used in trigger list
            struct t_trigger_data *trigger_data = trigger_data_new();

            if (json_get_string(request->data, "$.params.name", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.script", 0, FILENAME_LEN_MAX, &trigger_data->script, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.partition", 1, NAME_LEN_MAX, &sds_buf2, vcb_isname, &error) == true &&
                json_get_int(request->data, "$.params.id", -1, LIST_TRIGGER_MAX, &int_buf1, &error) == true &&
                json_get_int_max(request->data, "$.params.event", &int_buf2, &error) == true &&
                json_get_object_string(request->data, "$.params.arguments", &trigger_data->arguments, vcb_isname, SCRIPT_ARGUMENTS_MAX, &error) == true)
            {
                rc = mympd_api_trigger_save(&mympd_state->trigger_list, sds_buf1, int_buf1, int_buf2, sds_buf2, trigger_data, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_TRIGGER, "Could not save trigger");
                if (rc == true) {
                    //trigger_data is onw referenced by the trigger list
                    break;
                }
            }
            mympd_api_trigger_data_free(trigger_data);
            break;
        }
        case MYMPD_API_TRIGGER_RM:
            if (json_get_long(request->data, "$.params.id", 0, LIST_TRIGGER_MAX, &long_buf1, &error) == true) {
                rc = mympd_api_trigger_delete(&mympd_state->trigger_list, long_buf1);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_TRIGGER, "Could not delete trigger");
            }
            break;
        case INTERNAL_API_SCRIPT_INIT: {
            struct t_list *lua_mympd_state = list_new();
            rc = mympd_api_status_lua_mympd_state_set(lua_mympd_state, partition_state);
            if (rc == true) {
                response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_SCRIPT);
                response->extra = lua_mympd_state;
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Error getting mympd state for script execution");
                FREE_PTR(lua_mympd_state);
            }
            break;
        }
        case MYMPD_API_PLAYER_OUTPUT_ATTRIBUTES_SET: {
            struct t_list attributes;
            list_init(&attributes);
            if (json_get_uint(request->data, "$.params.outputId", 0, MPD_OUTPUT_ID_MAX, &uint_buf1, &error) == true &&
                json_get_object_string(request->data, "$.params.attributes", &attributes, vcb_isalnum, 10, &error) == true)
            {
                rc = mympd_api_output_attributes_set(partition_state, uint_buf1, &attributes, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc, JSONRPC_FACILITY_MPD, error);
            }
            list_clear(&attributes);
            break;
        }
        case MYMPD_API_MESSAGE_SEND:
            if (json_get_string(request->data, "$.params.channel", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.message", 1, CONTENT_LEN_MAX, &sds_buf2, vcb_isname, &error) == true)
            {
                mpd_run_send_message(partition_state->conn, sds_buf1, sds_buf2);
                response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_send_message", &rc);
            }
            break;
        case MYMPD_API_LIKE:
            if (mympd_state->mpd_state->feat_stickers == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_STICKER, JSONRPC_SEVERITY_ERROR, "MPD stickers are disabled");
                MYMPD_LOG_ERROR(partition_state->name, "MPD stickers are disabled");
                break;
            }
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true &&
                json_get_int(request->data, "$.params.like", 0, 2, &int_buf1, &error) == true)
            {
                rc = sticker_set_like(&mympd_state->mpd_state->sticker_queue, sds_buf1, int_buf1);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_STICKER, "Failed to set like");
                //mympd_feedback trigger
                mympd_api_trigger_execute_feedback(&mympd_state->trigger_list, sds_buf1, int_buf1, partition_state->name);
            }
            break;
        case MYMPD_API_PLAYER_STATE:
            response->data = mympd_api_status_get(partition_state, response->data, request->id, RESPONSE_TYPE_JSONRPC_RESPONSE);
            break;
        case MYMPD_API_PLAYER_CLEARERROR:
            mpd_run_clearerror(partition_state->conn);
            response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_clearerror", &rc);
            break;
        case MYMPD_API_DATABASE_UPDATE:
        case MYMPD_API_DATABASE_RESCAN:
            if (json_get_string(request->data, "$.params.uri", 0, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mympd_api_database_update(partition_state, response->data, request->cmd_id, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_SMARTPLS_STICKER_SAVE:
        case MYMPD_API_SMARTPLS_NEWEST_SAVE:
        case MYMPD_API_SMARTPLS_SEARCH_SAVE:
            if (mympd_state->mpd_state->feat_playlists == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "MPD does not support playlists");
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
            response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc, JSONRPC_FACILITY_PLAYLIST, "Failed saving smart playlist");
            if (rc == true) {
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
            mpd_run_pause(partition_state->conn, true);
            response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_pause", &rc);
            break;
        case MYMPD_API_PLAYER_RESUME:
            if (mympd_api_status_clear_error(partition_state, &response->data, request->cmd_id, request->id) == false) {
                break;
            }
            mpd_run_pause(partition_state->conn, false);
            response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_pause", &rc);
            break;
        case MYMPD_API_PLAYER_PREV:
            mpd_run_previous(partition_state->conn);
            response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_previous", &rc);
            break;
        case MYMPD_API_PLAYER_NEXT:
            mpd_run_next(partition_state->conn);
            response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_next", &rc);
            break;
        case MYMPD_API_PLAYER_PLAY:
            if (mympd_api_status_clear_error(partition_state, &response->data, request->cmd_id, request->id) == false) {
                break;
            }
            mpd_run_play(partition_state->conn);
            response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_play", &rc);
            break;
        case MYMPD_API_PLAYER_STOP:
            mpd_run_stop(partition_state->conn);
            response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_stop", &rc);
            break;
        case MYMPD_API_QUEUE_CLEAR:
            mpd_run_clear(partition_state->conn);
            response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_clear", &rc);
            break;
        case MYMPD_API_QUEUE_CROP:
            response->data = mympd_api_queue_crop(partition_state, response->data, request->cmd_id, request->id, false);
            break;
        case MYMPD_API_QUEUE_CROP_OR_CLEAR:
            response->data = mympd_api_queue_crop(partition_state, response->data, request->cmd_id, request->id, true);
            break;
        case MYMPD_API_QUEUE_RM_IDS: {
            struct t_list song_ids;
            list_init(&song_ids);
            if (json_get_array_llong(request->data, "$.params.songIds", &song_ids, MPD_COMMANDS_MAX, &error) == true) {
                rc = mympd_api_queue_rm_song_ids(partition_state, &song_ids, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, error);
            }
            list_clear(&song_ids);
            break;
        }
        case MYMPD_API_QUEUE_RM_RANGE:
            if (json_get_uint(request->data, "$.params.start", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_int(request->data, "$.params.end", -1, MPD_PLAYLIST_LENGTH_MAX, &int_buf1, &error) == true)
            {
                //map -1 to UINT_MAX for open ended range
                uint_buf2 = int_buf1 < 0
                    ? UINT_MAX
                    : (unsigned)int_buf1;
                mpd_run_delete_range(partition_state->conn, uint_buf1, uint_buf2);
                response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_delete_range", &rc);
            }
            break;
        case MYMPD_API_QUEUE_MOVE_POSITION:
            if (json_get_uint(request->data, "$.params.from", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf2, &error) == true)
            {
                if (uint_buf1 < uint_buf2) {
                    // decrease to position
                    uint_buf2--;
                }
                mpd_run_move(partition_state->conn, uint_buf1, uint_buf2);
                response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_move", &rc);
            }
            break;
        case MYMPD_API_QUEUE_MOVE_RELATIVE: {
            if (mympd_state->mpd_state->feat_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            struct t_list song_ids;
            list_init(&song_ids);
            if (json_get_array_llong(request->data, "$.params.songIds", &song_ids, MPD_COMMANDS_MAX, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.whence", 0, 2, &uint_buf2, &error) == true)
            {
                rc = mympd_api_queue_move_relative(partition_state, &song_ids, uint_buf1, uint_buf2, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, error);
            }
            list_clear(&song_ids);
            break;
        }
        case MYMPD_API_QUEUE_PRIO_SET: {
            struct t_list song_ids;
            list_init(&song_ids);
            if (json_get_array_llong(request->data, "$.params.songIds", &song_ids, MPD_COMMANDS_MAX, &error) == true &&
                json_get_uint(request->data, "$.params.priority", 0, MPD_QUEUE_PRIO_MAX, &uint_buf1, &error) == true)
            {
                rc = mympd_api_queue_prio_set(partition_state, &song_ids, uint_buf1, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, error);
            }
            list_clear(&song_ids);
            break;
        }
        case MYMPD_API_QUEUE_PRIO_SET_HIGHEST: {
            struct t_list song_ids;
            list_init(&song_ids);
            if (json_get_array_llong(request->data, "$.params.songIds", &song_ids, MPD_COMMANDS_MAX, &error) == true) {
                rc = mympd_api_queue_prio_set_highest(partition_state, &song_ids, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, error);
            }
            list_clear(&song_ids);
            break;
        }
        case MYMPD_API_PLAYER_PLAY_SONG:
            if (json_get_uint_max(request->data, "$.params.songId", &uint_buf1, &error) == true) {
                mpd_run_play_id(partition_state->conn, uint_buf1);
                response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_play_id", &rc);
            }
            break;
        case MYMPD_API_PLAYER_OUTPUT_LIST:
            response->data = mympd_api_output_list(partition_state, response->data, request->id);
            break;
        case MYMPD_API_PLAYER_OUTPUT_TOGGLE:
            if (json_get_uint(request->data, "$.params.outputId", 0, MPD_OUTPUT_ID_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.state", 0, 1, &uint_buf2, &error) == true)
            {
                if (uint_buf2 == 1) {
                    mpd_run_enable_output(partition_state->conn, uint_buf1);
                    response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_enable_output", &rc);
                }
                else {
                    mpd_run_disable_output(partition_state->conn, uint_buf1);
                    response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_disable_output", &rc);
                }
            }
            break;
        case MYMPD_API_PLAYER_VOLUME_SET:
            if (json_get_uint(request->data, "$.params.volume", 0, 100, &uint_buf1, &error) == true) {
                response->data = mympd_api_volume_set(partition_state, response->data, MYMPD_API_PLAYER_VOLUME_SET, request->id, uint_buf1);
            }
            break;
        case MYMPD_API_PLAYER_VOLUME_CHANGE:
            if (json_get_int(request->data, "$.params.volume", -99, 99, &int_buf1, &error) == true) {
                response->data = mympd_api_volume_change(partition_state, response->data, request->id, int_buf1);
            }
            break;
        case MYMPD_API_PLAYER_VOLUME_GET:
            response->data = mympd_api_status_volume_get(partition_state, response->data, request->id, RESPONSE_TYPE_JSONRPC_RESPONSE);
            break;
        case MYMPD_API_PLAYER_SEEK_CURRENT:
            if (json_get_int_max(request->data, "$.params.seek", &int_buf1, &error) == true &&
                json_get_bool(request->data, "$.params.relative", &bool_buf1, &error) == true)
            {
                mpd_run_seek_current(partition_state->conn, (float)int_buf1, bool_buf1);
                response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_seek_current", &rc);
            }
            break;
        case MYMPD_API_QUEUE_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                response->data = mympd_api_queue_list(partition_state, response->data, request->id, long_buf1, long_buf2, &tagcols);
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
                response->data = mympd_api_last_played_list(partition_state, response->data, request->id, long_buf1, long_buf2, sds_buf1, &tagcols);
            }
            break;
        }
        case MYMPD_API_PLAYER_CURRENT_SONG: {
            response->data = mympd_api_status_current_song(partition_state, response->data, request->id);
            break;
        }
        case MYMPD_API_SONG_DETAILS:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mympd_api_song_details(partition_state, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_SONG_COMMENTS:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mympd_api_song_comments(partition_state, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_SONG_FINGERPRINT:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mympd_api_song_fingerprint(partition_state, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_PLAYLIST_RENAME:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.newName", 1, FILENAME_LEN_MAX, &sds_buf2, vcb_isfilename, &error) == true)
            {
                response->data = mympd_api_playlist_rename(partition_state, response->data, request->id, sds_buf1, sds_buf2);
            }
            break;
        case MYMPD_API_PLAYLIST_RM: {
            struct t_list plists;
            list_init(&plists);
            if (json_get_array_string(request->data, "$.params.plists", &plists, vcb_isfilename, MPD_COMMANDS_MAX, &error) == true) {
                rc = mympd_api_playlist_delete(partition_state, &plists, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, error);
            }
            list_clear(&plists);
            break;
        }
        case MYMPD_API_PLAYLIST_RM_ALL:
            if (json_get_string(request->data, "$.params.type", 1, NAME_LEN_MAX, &sds_buf1, vcb_isalnum, &error) == true) {
                enum plist_delete_criterias criteria = parse_plist_delete_criteria(sds_buf1);
                response->data = mympd_api_playlist_delete_all(partition_state, response->data, request->id, criteria);
            }
            break;
        case MYMPD_API_PLAYLIST_LIST:
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_uint(request->data, "$.params.type", 0, 2, &uint_buf1, &error) == true)
            {
                response->data = mympd_api_playlist_list(partition_state, response->data, request->cmd_id, long_buf1, long_buf2, sds_buf1, uint_buf1);
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
                response->data = mympd_api_playlist_content_list(partition_state, response->data, request->id, sds_buf1, long_buf1, long_buf2, sds_buf2, &tagcols);
            }
            break;
        }
        case MYMPD_API_PLAYLIST_CONTENT_APPEND_URIS: {
            struct t_list uris;
            list_init(&uris);
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_array_string(request->data, "$.params.uris", &uris, vcb_isuri, MPD_COMMANDS_MAX, &error) == true)
            {
                rc = mympd_api_playlist_content_append(partition_state, sds_buf1, &uris, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, "Playlist updated", error);
            }
            list_clear(&uris);
            break;
        }
        case MYMPD_API_PLAYLIST_CONTENT_INSERT_URIS: {
            if (mympd_state->mpd_state->feat_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            struct t_list uris;
            list_init(&uris);
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_array_string(request->data, "$.params.uris", &uris, vcb_isuri, MPD_COMMANDS_MAX, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true)
            {
                rc = mympd_api_playlist_content_insert(partition_state, sds_buf1, &uris, uint_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, "Playlist updated", error);
            }
            list_clear(&uris);
            break;
        }
        case MYMPD_API_PLAYLIST_CONTENT_REPLACE_URIS: {
            struct t_list uris;
            list_init(&uris);
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_array_string(request->data, "$.params.uris", &uris, vcb_isuri, MPD_COMMANDS_MAX, &error) == true)
            {
                rc = mympd_api_playlist_content_replace(partition_state, sds_buf1, &uris, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, "Playlist updated", error);
            }
            list_clear(&uris);
            break;
        }
        case MYMPD_API_PLAYLIST_CONTENT_INSERT_SEARCH:
            if (mympd_state->mpd_state->feat_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf2, vcb_isname, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true)
            {
                rc = mympd_api_playlist_content_insert_search(partition_state, sds_buf2, sds_buf1, uint_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, "Playlist updated", error);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_REPLACE_SEARCH:
        case MYMPD_API_PLAYLIST_CONTENT_APPEND_SEARCH:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf2, vcb_isname, &error) == true)
            {
                rc = request->cmd_id == MYMPD_API_PLAYLIST_CONTENT_APPEND_SEARCH
                    ? mympd_api_playlist_content_append_search(partition_state, sds_buf2, sds_buf1, &error)
                    : mympd_api_playlist_content_replace_search(partition_state, sds_buf2, sds_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, "Playlist updated", error);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUMS:
        case MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUMS: {
            struct t_list albumids;
            list_init(&albumids);
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_array_string(request->data, "$.params.albumids", &albumids, vcb_isalnum, MPD_COMMANDS_MAX, &error) == true)
            {
                rc = request->cmd_id == MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUMS
                    ? mympd_api_playlist_content_append_albums(partition_state, sds_buf1, &albumids, &error)
                    : mympd_api_playlist_content_replace_albums(partition_state, sds_buf1, &albumids, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, "Playlist updated", error);
            }
            list_clear(&albumids);
            break;
        }
        case MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUMS: {
            if (mympd_state->mpd_state->feat_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            struct t_list albumids;
            list_init(&albumids);
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_array_string(request->data, "$.params.albumids", &albumids, vcb_isalnum, MPD_COMMANDS_MAX, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true)
            {
                rc = mympd_api_playlist_content_insert_albums(partition_state, sds_buf1, &albumids, uint_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, "Playlist updated", error);
            }
            list_clear(&albumids);
            break;
        }
        case MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUM_DISC:
        case MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUM_DISC:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.albumid", 1, FILENAME_LEN_MAX, &sds_buf2, vcb_isalnum, &error) == true &&
                json_get_string(request->data, "$.params.disc", 1, FILENAME_LEN_MAX, &sds_buf3, vcb_isdigit, &error) == true)
            {
                rc = request->cmd_id == MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUM_DISC
                    ? mympd_api_playlist_content_append_album_disc(partition_state, sds_buf1, sds_buf2, sds_buf3, &error)
                    : mympd_api_playlist_content_replace_album_disc(partition_state, sds_buf1, sds_buf2, sds_buf3, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, "Playlist updated", error);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUM_DISC:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.albumid", 1, FILENAME_LEN_MAX, &sds_buf2, vcb_isalnum, &error) == true &&
                json_get_string(request->data, "$.params.disc", 1, FILENAME_LEN_MAX, &sds_buf3, vcb_isdigit, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true)
            {
                rc = mympd_api_playlist_content_insert_album_disc(partition_state, sds_buf1, sds_buf2, sds_buf3, uint_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, "Playlist updated", error);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_CLEAR:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true) {
                rc = mpd_client_playlist_clear(partition_state, sds_buf1, &error);
                response->data = rc == true
                    ? jsonrpc_respond_ok(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST)
                    : jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Clearing the playlist %{playlist} failed: %{error}", 4, "playlist", sds_buf1, "error", error);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.from", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf2, &error) == true)
            {
                if (uint_buf1 < uint_buf2) {
                    // decrease to position
                    uint_buf2--;
                }
                mpd_run_playlist_move(partition_state->conn, sds_buf1, uint_buf1, uint_buf2);
                response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_playlist_move", &rc);
            }
            break;
        case MYMPD_API_PLAYLIST_CONTENT_MOVE_TO_PLAYLIST: {
            struct t_list positions;
            list_init(&positions);
            if (json_get_string(request->data, "$.params.srcPlist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.dstPlist", 1, FILENAME_LEN_MAX, &sds_buf2, vcb_isfilename, &error) == true &&
                json_get_array_llong(request->data, "$.params.positions", &positions, MPD_COMMANDS_MAX, &error) == true &&
                json_get_uint(request->data, "$.params.mode", 0, 1, &uint_buf1, &error) == true)
            {
                rc = mympd_api_playlist_content_move_to_playlist(partition_state, sds_buf1, sds_buf2, &positions, uint_buf1, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, error);
            }
            list_clear(&positions);
            break;
        }
        case MYMPD_API_PLAYLIST_CONTENT_RM_POSITIONS: {
            struct t_list positions;
            list_init(&positions);
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_array_llong(request->data, "$.params.positions", &positions, MPD_COMMANDS_MAX, &error) == true)
            {
                rc = mympd_api_playlist_content_rm_positions(partition_state, sds_buf1, &positions, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_PLAYLIST, error);
            }
            list_clear(&positions);
            break;
        }
        case MYMPD_API_PLAYLIST_CONTENT_RM_RANGE:
            if (mympd_state->mpd_state->feat_playlist_rm_range == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.start", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_int(request->data, "$.params.end", -1, MPD_PLAYLIST_LENGTH_MAX, &int_buf1, &error) == true)
            {
                //map -1 to UINT_MAX for open ended range
                uint_buf2 = int_buf1 < 0
                    ? UINT_MAX
                    : (unsigned)int_buf1;
                mpd_run_playlist_delete_range(partition_state->conn, sds_buf1, uint_buf1, uint_buf2);
                response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_playlist_delete_range", &rc);
            }
            break;
        case MYMPD_API_PLAYLIST_COPY: {
            struct t_list src_plists;
            list_init(&src_plists);
            if (json_get_array_string(request->data, "$.params.srcPlists", &src_plists, vcb_isfilename, JSONRPC_ARRAY_MAX, &error) == true &&
                json_get_string(request->data, "$.params.dstPlist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.mode", 0, 4, &uint_buf1, &error))
            {
                rc = mympd_api_playlist_copy(partition_state, &src_plists, sds_buf1, uint_buf1, &error);
                if (rc == false) {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, error);
                }
                else {
                    const char *err_msg = uint_buf1 == PLAYLIST_COPY_APPEND || uint_buf1 == PLAYLIST_COPY_INSERT
                        ? "Playlist successfully copied"
                        : uint_buf1 == PLAYLIST_COPY_REPLACE
                            ? "Playlist successfully replaced"
                            : "Playlist successfully moved";
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, err_msg);
                }
            }
            list_clear(&src_plists);
            break;
        }
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
                response->data = strcmp(sds_buf3, "plist") == 0
                    ? mympd_api_playlist_content_list(partition_state, response->data, request->id, sds_buf2, long_buf1, long_buf2, sds_buf1, &tagcols)
                    : mympd_api_browse_filesystem(partition_state, response->data, request->id, sds_buf2, long_buf1, long_buf2, sds_buf1, &tagcols);
            }
            break;
        }
        case MYMPD_API_QUEUE_APPEND_URIS:
        case MYMPD_API_QUEUE_REPLACE_URIS: {
            struct t_list uris;
            list_init(&uris);
            if (json_get_array_string(request->data, "$.params.uris", &uris, vcb_isuri, MPD_PLAYLIST_LENGTH_MAX, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = (request->cmd_id == MYMPD_API_QUEUE_APPEND_URIS
                        ? mympd_api_queue_append(partition_state, &uris, &error)
                        : mympd_api_queue_replace(partition_state, &uris, &error)) &&
                    mpd_client_queue_check_start_play(partition_state, bool_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                            JSONRPC_FACILITY_QUEUE, "Queue updated", error);
            }
            list_clear(&uris);
            break;
        }
        case MYMPD_API_QUEUE_INSERT_URIS: {
            if (mympd_state->mpd_state->feat_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            struct t_list uris;
            list_init(&uris);
            if (json_get_array_string(request->data, "$.params.uris", &uris, vcb_isuri, MPD_PLAYLIST_LENGTH_MAX, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.whence", 0, 2, &uint_buf2, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = mympd_api_queue_insert(partition_state, &uris, uint_buf1, uint_buf2, &error) &&
                    mpd_client_queue_check_start_play(partition_state, bool_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                            JSONRPC_FACILITY_QUEUE, "Queue updated", error);
            }
            list_clear(&uris);
            break;
        }
        case MYMPD_API_QUEUE_APPEND_PLAYLISTS:
        case MYMPD_API_QUEUE_REPLACE_PLAYLISTS: {
            struct t_list plists;
            list_init(&plists);
            if (json_get_array_string(request->data, "$.params.plists", &plists, vcb_isuri, MPD_COMMANDS_MAX, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = (request->cmd_id == MYMPD_API_QUEUE_APPEND_PLAYLISTS
                        ? mympd_api_queue_append_plist(partition_state, &plists, &error)
                        : mympd_api_queue_replace_plist(partition_state, &plists, &error)) &&
                    mpd_client_queue_check_start_play(partition_state, bool_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, "Queue updated", error);
            }
            list_clear(&plists);
            break;
        }
        case MYMPD_API_QUEUE_INSERT_PLAYLISTS: {
            if (mympd_state->mpd_state->feat_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            struct t_list plists;
            list_init(&plists);
            if (json_get_array_string(request->data, "$.params.plists", &plists, vcb_isuri, MPD_COMMANDS_MAX, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.whence", 0, 2, &uint_buf2, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = mympd_api_queue_insert_plist(partition_state, &plists, uint_buf1, uint_buf2, &error) &&
                    mpd_client_queue_check_start_play(partition_state, bool_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, "Queue updated", error);
            }
            list_clear(&plists);
            break;
        }
        case MYMPD_API_QUEUE_INSERT_SEARCH:
            if (mympd_state->mpd_state->feat_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            if (json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.whence", 0, 2, &uint_buf2, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = mympd_api_queue_insert_search(partition_state, sds_buf1, uint_buf1, uint_buf2, &error) &&
                        mpd_client_queue_check_start_play(partition_state, bool_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, "Queue updated", error);
            }
            break;
        case MYMPD_API_QUEUE_APPEND_SEARCH:
        case MYMPD_API_QUEUE_REPLACE_SEARCH:
            if (json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = (request->cmd_id == MYMPD_API_QUEUE_APPEND_SEARCH
                        ? mympd_api_queue_append_search(partition_state, sds_buf1, &error)
                        : mympd_api_queue_replace_search(partition_state, sds_buf1, &error)) &&
                    mpd_client_queue_check_start_play(partition_state, bool_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, "Queue updated", error);
            }
            break;
        case MYMPD_API_QUEUE_APPEND_ALBUMS:
        case MYMPD_API_QUEUE_REPLACE_ALBUMS: {
            struct t_list albumids;
            list_init(&albumids);
            if (json_get_array_string(request->data, "$.params.albumids", &albumids, vcb_isalnum, MPD_COMMANDS_MAX, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = (request->cmd_id == MYMPD_API_QUEUE_APPEND_ALBUMS
                        ? mympd_api_queue_append_albums(partition_state, &albumids, &error)
                        : mympd_api_queue_replace_albums(partition_state, &albumids, &error)) &&
                    mpd_client_queue_check_start_play(partition_state, bool_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, "Queue updated", error);
            }
            list_clear(&albumids);
            break;
        }
        case MYMPD_API_QUEUE_INSERT_ALBUMS: {
            if (mympd_state->mpd_state->feat_whence == false) {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Method not supported");
                break;
            }
            struct t_list albumids;
            list_init(&albumids);
            if (json_get_array_string(request->data, "$.params.albumids", &albumids, vcb_isalnum, MPD_COMMANDS_MAX, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.whence", 0, 2, &uint_buf2, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = mympd_api_queue_insert_albums(partition_state, &albumids, uint_buf1, uint_buf2, &error) &&
                    mpd_client_queue_check_start_play(partition_state, bool_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, "Queue updated", error);
            }
            list_clear(&albumids);
            break;
        }
        case MYMPD_API_QUEUE_APPEND_ALBUM_DISC:
        case MYMPD_API_QUEUE_REPLACE_ALBUM_DISC: {
            if (json_get_string(request->data, "$.params.albumid", 1, NAME_LEN_MAX, &sds_buf1, vcb_isalnum, &error) == true &&
                json_get_string(request->data, "$.params.disc", 1, NAME_LEN_MAX, &sds_buf2, vcb_isdigit, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = (request->cmd_id == MYMPD_API_QUEUE_APPEND_ALBUM_DISC
                        ? mympd_api_queue_append_album_disc(partition_state, sds_buf1, sds_buf2, &error)
                        : mympd_api_queue_replace_album_disc(partition_state, sds_buf1, sds_buf2, &error)) &&
                    mpd_client_queue_check_start_play(partition_state, bool_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, "Queue updated", error);
            }
            break;
        }
        case MYMPD_API_QUEUE_INSERT_ALBUM_DISC: {
            if (json_get_string(request->data, "$.params.albumid", 1, NAME_LEN_MAX, &sds_buf1, vcb_isalnum, &error) == true &&
                json_get_string(request->data, "$.params.disc", 1, NAME_LEN_MAX, &sds_buf2, vcb_isdigit, &error) == true &&
                json_get_uint(request->data, "$.params.to", 0, MPD_PLAYLIST_LENGTH_MAX, &uint_buf1, &error) == true &&
                json_get_uint(request->data, "$.params.whence", 0, 2, &uint_buf2, &error) == true &&
                json_get_bool(request->data, "$.params.play", &bool_buf1, &error) == true)
            {
                rc = mympd_api_queue_insert_album_disc(partition_state, sds_buf1, sds_buf2, uint_buf1, uint_buf2, &error) &&
                    mpd_client_queue_check_start_play(partition_state, bool_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, "Queue updated", error);
            }
            break;
        }
        case MYMPD_API_QUEUE_ADD_RANDOM:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_uint(request->data, "$.params.mode", 0, 2, &uint_buf1, &error) == true &&
                json_get_long(request->data, "$.params.quantity", 0, 1000, &long_buf1, &error) == true)
            {
                rc = jukebox_add_to_queue(partition_state, long_buf1, uint_buf1, sds_buf1, true);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, "Successfully added random songs to queue", "Adding random songs to queue failed");
            }
            break;
        case MYMPD_API_QUEUE_SAVE:
            if (json_get_string(request->data, "$.params.plist", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string(request->data, "$.params.mode", 1, NAME_LEN_MAX, &sds_buf2, vcb_isalnum, &error) == true)
            {
                rc = mympd_api_queue_save(partition_state, sds_buf1, sds_buf2, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_QUEUE, error);
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
                response->data = mympd_api_queue_search(partition_state, response->data, request->id,
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
                response->data = mympd_api_queue_search_adv(partition_state, response->data, request->id,
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
                response->data = mympd_api_search_songs(partition_state, response->data, request->id,
                        sds_buf1, sds_buf2, bool_buf1, uint_buf1, uint_buf2, &tagcols, &rc);
            }
            break;
        }
        case MYMPD_API_QUEUE_SHUFFLE:
            mpd_run_shuffle(partition_state->conn);
            response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_shuffle", &rc);
            break;
        case MYMPD_API_STATS:
            response->data = mympd_api_stats_get(partition_state, response->data, request->id);
            break;
        case INTERNAL_API_ALBUMART:
            if (json_get_string(request->data, "$.params.uri", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                response->data = mympd_api_albumart_getcover(partition_state, response->data, request->id, sds_buf1, &response->binary);
            }
            break;
        case MYMPD_API_DATABASE_ALBUM_LIST: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.expression", 0, EXPRESSION_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.sort", 1, NAME_LEN_MAX, &sds_buf2, vcb_ismpdsort, &error) == true &&
                json_get_bool(request->data, "$.params.sortdesc", &bool_buf1, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                response->data = mympd_api_browse_album_list(partition_state, response->data, request->id,
                        sds_buf1, sds_buf2, bool_buf1, long_buf1, long_buf2, &tagcols);
            }
            break;
        }
        case MYMPD_API_DATABASE_TAG_LIST:
            if (json_get_long(request->data, "$.params.offset", 0, MPD_PLAYLIST_LENGTH_MAX, &long_buf1, &error) == true &&
                json_get_long(request->data, "$.params.limit", MPD_RESULTS_MIN, MPD_RESULTS_MAX, &long_buf2, &error) == true &&
                json_get_string(request->data, "$.params.searchstr", 0, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true &&
                json_get_string(request->data, "$.params.tag", 1, NAME_LEN_MAX, &sds_buf2, vcb_ismpdtag_or_any, &error) == true &&
                json_get_bool(request->data, "$.params.sortdesc", &bool_buf1, &error) == true)
            {
                response->data = mympd_api_browse_tag_list(partition_state, response->data, request->id,
                        sds_buf1, sds_buf2, long_buf1, long_buf2, bool_buf1);
            }
            break;
        case MYMPD_API_DATABASE_ALBUM_DETAIL: {
            struct t_tags tagcols;
            reset_t_tags(&tagcols);
            if (json_get_string(request->data, "$.params.albumid", 1, NAME_LEN_MAX, &sds_buf1, vcb_isalnum, &error) == true &&
                json_get_tags(request->data, "$.params.cols", &tagcols, COLS_MAX, &error) == true)
            {
                response->data = mympd_api_browse_album_detail(partition_state, response->data, request->id, sds_buf1, &tagcols);
            }
            break;
        }
        case INTERNAL_API_TIMER_STARTPLAY:
            if (json_get_uint(request->data, "$.params.volume", 0, 100, &uint_buf1, &error) == true &&
                json_get_string(request->data, "$.params.plist", 0, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &error) == true &&
                json_get_string_max(request->data, "$.params.preset", &sds_buf2, vcb_isname, &error) == true)
            {
                rc = mympd_api_timer_startplay(partition_state, uint_buf1, sds_buf1, sds_buf2);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_TIMER, "Error executing timer start play");
            }
            break;
        case MYMPD_API_MOUNT_URLHANDLER_LIST:
            response->data = mympd_api_mounts_urlhandler_list(partition_state, response->data, request->id);
            break;
        case MYMPD_API_PARTITION_LIST:
            response->data = mympd_api_partition_list(mympd_state, response->data, request->id);
            break;
        case MYMPD_API_PARTITION_NEW:
            if (json_get_string(request->data, "$.params.name", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true) {
                rc = mympd_api_partition_new(partition_state, sds_buf1, &error);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_MPD, "Partition created successfully", error);
            }
            break;
        case MYMPD_API_PARTITION_SAVE:
            rc = json_iterate_object(request->data, "$.params", mympd_api_settings_partition_set, partition_state, NULL, 1000, &error);
            if (rc == true) {
                settings_to_webserver(partition_state->mympd_state);
            }
            response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                    JSONRPC_FACILITY_MPD, error);
            break;
        case MYMPD_API_PARTITION_RM:
            if (json_get_string(request->data, "$.params.name", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, &error) == true) {
                response->data = mympd_api_partition_rm(partition_state, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_PARTITION_OUTPUT_MOVE: {
            struct t_list outputs;
            list_init(&outputs); 
            if (json_get_array_string(request->data, "$.params.outputs", &outputs, vcb_isname, 10, &error) == true) {
                rc = mympd_api_partition_outputs_move(partition_state, &outputs, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc, JSONRPC_FACILITY_MPD, error);
            }
            list_clear(&outputs);
            break;
        }
        case MYMPD_API_MOUNT_LIST:
            response->data = mympd_api_mounts_list(partition_state, response->data, request->id);
            break;
        case MYMPD_API_MOUNT_NEIGHBOR_LIST:
            response->data = mympd_api_mounts_neighbor_list(partition_state, response->data, request->id);
            break;
        case MYMPD_API_MOUNT_MOUNT:
            if (json_get_string(request->data, "$.params.mountUrl", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isuri, &error) == true &&
                json_get_string(request->data, "$.params.mountPoint", 1, FILEPATH_LEN_MAX, &sds_buf2, vcb_isfilepath, &error) == true)
            {
                mpd_run_mount(partition_state->conn, sds_buf2, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_mount", &rc);
            }
            break;
        case MYMPD_API_MOUNT_UNMOUNT:
            if (json_get_string(request->data, "$.params.mountPoint", 1, FILEPATH_LEN_MAX, &sds_buf1, vcb_isfilepath, &error) == true) {
                mpd_run_unmount(partition_state->conn, sds_buf1);
                response->data = mympd_respond_with_error_or_ok(partition_state, response->data, request->cmd_id, request->id, "mpd_run_unmount", &rc);
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
                json_get_string(request->data, "$.params.codec", 0, FILEPATH_LEN_MAX, &sds_buf9, vcb_isprint, &error) == true &&
                json_get_int(request->data, "$.params.bitrate", 0, 2048, &int_buf1, &error) == true &&
                json_get_string(request->data, "$.params.description", 0, CONTENT_LEN_MAX, &sds_buf0, vcb_isname, &error) == true
            ) {
                rc = mympd_api_webradio_save(config->workdir, sds_buf1, sds_buf2, sds_buf3, sds_buf4, sds_buf5, sds_buf6, sds_buf7,
                    sds_buf8, sds_buf9, int_buf1, sds_buf0);
                response->data = jsonrpc_respond_with_message_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_DATABASE, "Webradio favorite successfully saved", "Could not save webradio favorite");
            }
            break;
        case MYMPD_API_WEBRADIO_FAVORITE_RM: {
            struct t_list filenames;
            list_init(&filenames);
            if (json_get_array_string(request->data, "$.params.filenames", &filenames, vcb_isfilename, MPD_COMMANDS_MAX, &error) == true) {
                if (filenames.length == 0) {
                    response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "No webradios provided");
                }
                rc = mympd_api_webradio_delete(config->workdir, &filenames);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_DATABASE, "Could not delete webradio favorite");
            }
            list_clear(&filenames);
            break;
        }
        default:
            response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Unknown request");
            MYMPD_LOG_ERROR(partition_state->name, "Unknown API request: %.*s", (int)sdslen(request->data), request->data);
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

    #ifdef MYMPD_DEBUG
        MEASURE_END
        MEASURE_PRINT(partition_state->name, method)
    #endif

    //async request handling
    //request was forwarded to worker thread - do not free it
    if (async == true) {
        free_response(response);
        FREE_SDS(error);
        return;
    }

    //sync request handling
    if (sdslen(response->data) == 0) {
        //errorhandling
        if (sdslen(error) > 0) {
            response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, error);
            MYMPD_LOG_ERROR(partition_state->name, "Error processing method \"%s\"", method);
        }
        else {
            response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "No response for method %{method}", 2, "method", method);
            MYMPD_LOG_ERROR(partition_state->name, "No response for method \"%s\"", method);
        }
    }
    push_response(response, request->id, request->conn_id);
    free_request(request);
    FREE_SDS(error);
}
