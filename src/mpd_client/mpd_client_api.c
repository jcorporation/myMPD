/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <time.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../api.h"
#include "../log.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "../mpd_shared/mpd_shared_search.h"
#include "../mpd_shared/mpd_shared_playlists.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_sticker.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../lua_mympd_state.h"
#include "mpd_client_utility.h"
#include "mpd_client_browse.h"
#include "mpd_client_cover.h" 
#include "mpd_client_features.h"
#include "mpd_client_jukebox.h"
#include "mpd_client_playlists.h"
#include "mpd_client_queue.h"
#include "mpd_client_state.h"
#include "mpd_client_stats.h"
#include "mpd_client_settings.h"
#include "mpd_client_sticker.h"
#include "mpd_client_timer.h"
#include "mpd_client_mounts.h"
#include "mpd_client_partitions.h"
#include "mpd_client_trigger.h"
#include "mpd_client_lyrics.h"
#include "mpd_client_api.h"

void mpd_client_api(t_config *config, t_mpd_client_state *mpd_client_state, void *arg_request) {
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
    
    #ifdef DEBUG
    clock_t start = clock();
    #endif

    LOG_VERBOSE("MPD CLIENT API request (%d)(%ld) %s: %s", request->conn_id, request->id, request->method, request->data);
    //create response struct
    t_work_result *response = create_result(request);
    
    switch(request->cmd_id) {
        case MPD_API_LYRICS_GET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                if (p_charbuf1 == NULL || validate_uri(p_charbuf1) == false) {
                    LOG_ERROR("Invalid URI: %s", p_charbuf1);
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Invalid uri", true);
                }
                else {
                    response->data = mpd_client_lyrics_get(config, mpd_client_state, response->data, request->method, request->id, p_charbuf1);
                }
            }
            break;
        case MPD_API_LYRICS_UNSYNCED_GET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                if (p_charbuf1 == NULL || validate_uri(p_charbuf1) == false) {
                    LOG_ERROR("Invalid URI: %s", p_charbuf1);
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Invalid uri", true);
                }
                else {
                    response->data = mpd_client_lyrics_unsynced(config, mpd_client_state, response->data, request->method, request->id, p_charbuf1);
                }
            }
            break;
        case MPD_API_LYRICS_SYNCED_GET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                if (p_charbuf1 == NULL || validate_uri(p_charbuf1) == false) {
                    LOG_ERROR("Invalid URI: %s", p_charbuf1);
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Invalid uri", true);
                }
                else {
                    response->data = mpd_client_lyrics_synced(config, mpd_client_state, response->data, request->method, request->id, p_charbuf1);
                }
            }
            break;
        case MPD_API_STATE_SAVE:
            mpd_client_last_played_list_save(config, mpd_client_state);
            triggerfile_save(config, mpd_client_state);
            response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
            break;
        case MPD_API_JUKEBOX_RM:
            je = json_scanf(request->data, sdslen(request->data), "{params: {pos: %u}}", &uint_buf1);
            if (je == 1) {
                rc = mpd_client_rm_jukebox_entry(mpd_client_state, uint_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Could not remove song from jukebox queue", true);
                }
            }
            break;
        case MPD_API_JUKEBOX_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, limit: %u, cols: %M}}", &uint_buf1, &uint_buf2, json_to_tags, tagcols);
            if (je == 3) {
                response->data = mpd_client_put_jukebox_list(mpd_client_state, response->data, request->method, request->id, uint_buf1, uint_buf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_TRIGGER_LIST:
            response->data = trigger_list(mpd_client_state, response->data, request->method, request->id);
            break;
        case MPD_API_TRIGGER_GET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {id: %d}}", &int_buf1);
            if (je == 1) {
                response->data = trigger_get(mpd_client_state, response->data, request->method, request->id, int_buf1);
            }
            break;
        case MPD_API_TRIGGER_SAVE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {id: %d, name: %Q, event: %d, script: %Q}}", 
                &int_buf1, &p_charbuf1, &int_buf2, &p_charbuf2);
            if (je == 4 && validate_string_not_empty(p_charbuf2) == true) {
                struct list *arguments = (struct list *) malloc(sizeof(struct list));
                assert(arguments);
                list_init(arguments);
                void *h = NULL;
                struct json_token key;
                struct json_token val;
                while ((h = json_next_key(request->data, sdslen(request->data), h, ".params.arguments", &key, &val)) != NULL) {
                    list_push_len(arguments, key.ptr, key.len, 0, val.ptr, val.len, NULL);
                }
                //add new entry
                rc = list_push(&mpd_client_state->triggers, p_charbuf1, int_buf2, p_charbuf2, arguments);
                if (rc == true) {
                    if (int_buf1 >= 0) {
                        //delete old entry
                        rc = delete_trigger(mpd_client_state, int_buf1);
                    }
                    if (rc == true) {
                        response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
                    }
                    else {
                        response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Could not save trigger", true);
                    }
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Could not save trigger", true);
                }
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Invalid trigger name", true);
            }
            break;
        case MPD_API_TRIGGER_DELETE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {id: %u}}", &uint_buf1);
            if (je == 1) {
                rc = delete_trigger(mpd_client_state, uint_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Could not delete trigger", true);
                }
            }
            break;
        #ifdef ENABLE_LUA
        case MPD_API_SCRIPT_INIT:
            if (config->scripting == true) {
                struct list *lua_mympd_state = (struct list *) malloc(sizeof(struct list));
                assert(lua_mympd_state);
                list_init(lua_mympd_state);
                rc = mpd_client_get_lua_mympd_state(config, mpd_client_state, lua_mympd_state);
                if (rc == true) {
                    t_work_request *script_request = create_request(-2, request->id, MYMPD_API_SCRIPT_INIT, "MYMPD_API_SCRIPT_INIT", "");
                    script_request->data = sdscat(script_request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SCRIPT_INIT\",\"params\":{}}");
                    script_request->extra = lua_mympd_state;
                    tiny_queue_push(mympd_api_queue, script_request, request->id);
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
                    request->conn_id = -1; //do not respond
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, "MYMPD_API_SCRIPT_INIT", request->id, "Error getting mpd state for script execution", true);
                    free(lua_mympd_state);
                }
            }
            else {
                response->data = jsonrpc_respond_message(response->data, "MYMPD_API_SCRIPT_INIT", request->id, "Scripting is disabled", true);
            }
            break;
        #endif
        case MPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {outputId: %u}}", &uint_buf1);
            if (je == 1) {
                void *h = NULL;
                struct json_token key;
                struct json_token val;
                while ((h = json_next_key(request->data, sdslen(request->data), h, ".params.attributes", &key, &val)) != NULL) {
                    sds attribute = sdsnewlen(key.ptr, key.len);
                    sds value = sdsnewlen(val.ptr, val.len);
                    rc = mpd_run_output_set(mpd_client_state->mpd_state->conn, uint_buf1, attribute, value);
                    sdsfree(attribute);
                    sdsfree(value);
                    response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_output_set");
                    if (rc == false) {
                        break;
                    }
                }
            }
            break;
        case MPD_API_STICKERCACHE_CREATED:
            sticker_cache_free(&mpd_client_state->sticker_cache);
            if (request->extra != NULL) {
                mpd_client_state->sticker_cache = (rax *) request->extra;
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
                LOG_VERBOSE("Sticker cache was replaced");
            }
            else {
                LOG_ERROR("Sticker cache is NULL");
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Sticker cache is NULL", true);
            }
            mpd_client_state->sticker_cache_building = false;
            break;
        case MPD_API_ALBUMCACHE_CREATED:
            album_cache_free(&mpd_client_state->album_cache);
            if (request->extra != NULL) {
                mpd_client_state->album_cache = (rax *) request->extra;
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
                LOG_VERBOSE("Album cache was replaced");
            }
            else {
                LOG_ERROR("Album cache is NULL");
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Album cache is NULL", true);
            }
            mpd_client_state->album_cache_building = false;
            break;
        case MPD_API_LOVE:
            if (mpd_run_send_message(mpd_client_state->mpd_state->conn, mpd_client_state->love_channel, mpd_client_state->love_message) == true) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Scrobbled love", false);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Failed to send love message to channel", true);
            }
            check_error_and_recover2(mpd_client_state->mpd_state, &response->data, request->method, request->id, false);
            break;
        case MPD_API_MESSAGE_SEND:
            je = json_scanf(request->data, sdslen(request->data), "{params: {channel: %Q, message: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                uint_buf1 = mpd_run_send_message(mpd_client_state->mpd_state->conn, p_charbuf1, p_charbuf2);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, (uint_buf1 == 0 ? false : true), "mpd_run_send_message");
            }
            break;
        case MPD_API_LIKE:
            if (mpd_client_state->feat_sticker == false) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, "MPD stickers are disabled", true);
                LOG_ERROR("MPD stickers are disabled");
                break;
            }
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q, like: %d}}", &p_charbuf1, &int_buf1);
            if (je == 2 && strlen(p_charbuf1) > 0) {
                if (int_buf1 < 0 || int_buf1 > 2) {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Failed to set like, invalid like value", true);
                    break;
                }
                if (is_streamuri(p_charbuf1) == true) {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Failed to set like, invalid song uri", true);
                    break;
                }
                rc = mpd_client_sticker_like(mpd_client_state, p_charbuf1, int_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Failed to set like, unknown error", true);
                }
            }
            break;
        case MPD_API_PLAYER_STATE:
            response->data = mpd_client_put_state(config, mpd_client_state, response->data, request->method, request->id);
            break;
        case MYMPD_API_SETTINGS_SET: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            rc = true;
            bool mpd_host_changed = false;
            bool jukebox_changed = false;
            bool check_mpd_error = false;
            sds notify_buffer = sdsempty();
            while ((h = json_next_key(request->data, sdslen(request->data), h, ".params", &key, &val)) != NULL) {
                rc = mpd_api_settings_set(config, mpd_client_state, &key, &val, &mpd_host_changed, &jukebox_changed, &check_mpd_error);
                if ((check_mpd_error == true && check_error_and_recover2(mpd_client_state->mpd_state, &notify_buffer, request->method, request->id, true) == false)
                    || rc == false)
                {
                    if (sdslen(notify_buffer) > 0) {
                        ws_notify(notify_buffer);
                    }
                    break;
                }
            }
            sdsfree(notify_buffer);
            if (rc == true) {
                if (mpd_host_changed == true) {
                    //reconnect with new settings
                    mpd_client_state->mpd_state->conn_state = MPD_DISCONNECT;
                }
                if (mpd_client_state->mpd_state->conn_state == MPD_CONNECTED) {
                    //feature detection
                    mpd_client_mpd_features(config, mpd_client_state);
                    
                    if (jukebox_changed == true) {
                        LOG_DEBUG("Jukebox options changed, clearing jukebox queue");
                        list_free(&mpd_client_state->jukebox_queue);
                        mpd_client_state->jukebox_enforce_unique = true;
                    }
                    if (mpd_client_state->jukebox_mode != JUKEBOX_OFF) {
                        //enable jukebox
                        mpd_client_jukebox(config, mpd_client_state, 0);
                    }
                }
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
            }
            else {
                response->data = jsonrpc_start_phrase(response->data, request->method, request->id, "Can't save setting %{setting}", true);
                response->data = tojson_char_len(response->data, "setting", val.ptr, val.len, false);
                response->data = jsonrpc_end_phrase(response->data);
            }
            break;
        }
        case MPD_API_DATABASE_UPDATE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                if (strcmp(p_charbuf1, "") == 0) {
                    FREE_PTR(p_charbuf1);
                }
                uint_buf1 = mpd_run_update(mpd_client_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, (uint_buf1 == 0 ? false : true), "mpd_run_update");
            }
            break;
        case MPD_API_DATABASE_RESCAN:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                if (strcmp(p_charbuf1, "") == 0) {
                    FREE_PTR(p_charbuf1);
                }
                uint_buf1 = mpd_run_rescan(mpd_client_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, (uint_buf1 == 0 ? false : true), "mpd_run_rescan");
            }
            break;
        case MPD_API_SMARTPLS_SAVE:
            if (mpd_client_state->feat_smartpls == false) {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Smart playlists are disabled", true);
                break;
            }
            je = json_scanf(request->data, sdslen(request->data), "{params: {type: %Q}}", &p_charbuf1);
            rc = false;
            if (je == 1) {
                if (strcmp(p_charbuf1, "sticker") == 0) {
                    je = json_scanf(request->data, sdslen(request->data), "{params: {playlist: %Q, sticker: %Q, maxentries: %d, minvalue: %d, sort: %Q}}", &p_charbuf2, &p_charbuf3, &int_buf1, &int_buf2, &p_charbuf5);
                    if (je == 5) {
                        rc = mpd_shared_smartpls_save(config, p_charbuf1, p_charbuf2, p_charbuf3, NULL, int_buf1, int_buf2, p_charbuf5);
                    }
                }
                else if (strcmp(p_charbuf1, "newest") == 0) {
                    je = json_scanf(request->data, sdslen(request->data), "{params: {playlist: %Q, timerange: %d, sort: %Q}}", &p_charbuf2, &int_buf1, &p_charbuf5);
                    if (je == 3) {
                        rc = mpd_shared_smartpls_save(config, p_charbuf1, p_charbuf2, NULL, NULL, 0, int_buf1, p_charbuf5);
                    }
                }            
                else if (strcmp(p_charbuf1, "search") == 0) {
                    je = json_scanf(request->data, sdslen(request->data), "{params: {playlist: %Q, tag: %Q, searchstr: %Q, sort: %Q}}", &p_charbuf2, &p_charbuf3, &p_charbuf4, &p_charbuf5);
                    if (je == 4) {
                        rc = mpd_shared_smartpls_save(config, p_charbuf1, p_charbuf2, p_charbuf3, p_charbuf4, 0, 0, p_charbuf5);
                    }
                }
            }
            if (rc == true) {
                response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
                mpd_client_smartpls_update(p_charbuf2);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Failed to save playlist", true);
            }
            break;
        case MPD_API_SMARTPLS_GET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {playlist: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_client_smartpls_put(config, response->data, request->method, request->id, p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PAUSE:
            rc = mpd_run_toggle_pause(mpd_client_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_toggle_pause");
            break;
        case MPD_API_PLAYER_PREV:
            rc = mpd_run_previous(mpd_client_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_previous");
            break;
        case MPD_API_PLAYER_NEXT:
            rc = mpd_run_next(mpd_client_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_next");
            break;
        case MPD_API_PLAYER_PLAY:
            rc = mpd_run_play(mpd_client_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_play");
            break;
        case MPD_API_PLAYER_STOP:
            rc = mpd_run_stop(mpd_client_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_stop");
            break;
        case MPD_API_QUEUE_CLEAR:
            rc = mpd_run_clear(mpd_client_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_clear");
            break;
        case MPD_API_QUEUE_CROP:
            response->data = mpd_client_crop_queue(mpd_client_state, response->data, request->method, request->id, false);
            break;
        case MPD_API_QUEUE_CROP_OR_CLEAR:
            response->data = mpd_client_crop_queue(mpd_client_state, response->data, request->method, request->id, true);
            break;
        case MPD_API_QUEUE_RM_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {track:%u}}", &uint_buf1);
            if (je == 1) {
                rc = mpd_run_delete_id(mpd_client_state->mpd_state->conn, uint_buf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_delete_id");
            }
            break;
        case MPD_API_QUEUE_RM_RANGE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {start: %u, end: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                rc = mpd_run_delete_range(mpd_client_state->mpd_state->conn, uint_buf1, uint_buf2);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_delete_range");
            }
            break;
        case MPD_API_QUEUE_MOVE_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {from: %u, to: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2) {
                    uint_buf2--;
                }
                rc = mpd_run_move(mpd_client_state->mpd_state->conn, uint_buf1, uint_buf2);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_move");
            }
            break;
        case MPD_API_QUEUE_PRIO_SET_HIGHEST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {trackid: %u}}", &uint_buf1);
            if (je == 1) {
                rc = mpd_client_queue_prio_set_highest(mpd_client_state, uint_buf1);
                if (rc == true) {
                    response->data = jsonrpc_respond_ok(response->data, request->method, request->id);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Failed to set song priority", true);
                }
            }
            break;
        case MPD_API_PLAYLIST_MOVE_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {plist: %Q, from: %u, to: %u }}", &p_charbuf1, &uint_buf1, &uint_buf2);
            if (je == 3) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2) {
                    uint_buf2--;
                }
                rc = mpd_run_playlist_move(mpd_client_state->mpd_state->conn, p_charbuf1, uint_buf1, uint_buf2);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_move");
            }
            break;
        case MPD_API_PLAYER_PLAY_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: { track:%u}}", &uint_buf1);
            if (je == 1) {
                rc = mpd_run_play_id(mpd_client_state->mpd_state->conn, uint_buf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_play_id");
            }
            break;
        case MPD_API_PLAYER_OUTPUT_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {partition: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_client_put_partition_outputs(mpd_client_state, response->data, request->method, request->id, p_charbuf1);
            }
            else {
                response->data = mpd_client_put_outputs(mpd_client_state, response->data, request->method, request->id);
            }
            break;
        case MPD_API_PLAYER_TOGGLE_OUTPUT:
            je = json_scanf(request->data, sdslen(request->data), "{params: {output: %u, state: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (uint_buf2 == 1) {
                    rc = mpd_run_enable_output(mpd_client_state->mpd_state->conn, uint_buf1);
                    response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_enable_output");
                }
                else {
                    rc = mpd_run_disable_output(mpd_client_state->mpd_state->conn, uint_buf1);
                    response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_disable_output");
                }
            }
            break;
        case MPD_API_PLAYER_VOLUME_SET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {volume:%u}}", &uint_buf1);
            if (je == 1) {
                if (uint_buf1 > config->volume_max || uint_buf1 < config->volume_min) {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Invalid volume level", true);
                }
                else {
                    rc = mpd_run_set_volume(mpd_client_state->mpd_state->conn, uint_buf1);
                    response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_set_volume");
                }
            }
            break;
        case MPD_API_PLAYER_VOLUME_GET:
            response->data = mpd_client_put_volume(mpd_client_state, response->data, request->method, request->id);
            break;            
        case MPD_API_PLAYER_SEEK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {songid: %u, seek: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                rc = mpd_run_seek_id(mpd_client_state->mpd_state->conn, uint_buf1, uint_buf2);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_seek_id");
            }
            break;
        case MPD_API_PLAYER_SEEK_CURRENT:
            je = json_scanf(request->data, sdslen(request->data), "{params: {seek: %f, relative: %B}}", &float_buf, &bool_buf1);
            if (je == 2) {
                rc = mpd_run_seek_current(mpd_client_state->mpd_state->conn, float_buf, bool_buf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_seek_current");
            }
            break;
        case MPD_API_QUEUE_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, limit: %u, cols: %M}}", &uint_buf1, &uint_buf2, json_to_tags, tagcols);
            if (je == 3) {
                response->data = mpd_client_put_queue(mpd_client_state, response->data, request->method, request->id, uint_buf1, uint_buf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_LAST_PLAYED: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, limit: %u, cols: %M}}", &uint_buf1, &uint_buf2, json_to_tags, tagcols);
            if (je == 3) {
                response->data = mpd_client_put_last_played_songs(config, mpd_client_state, response->data, request->method, request->id, uint_buf1, uint_buf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYER_CURRENT_SONG: {
            response->data = mpd_client_put_current_song(mpd_client_state, response->data, request->method, request->id);
            break;
        }
        case MPD_API_DATABASE_SONGDETAILS:
            je = json_scanf(request->data, sdslen(request->data), "{params: { uri: %Q}}", &p_charbuf1);
            if (je == 1 && strlen(p_charbuf1) > 0) {
                response->data = mpd_client_put_songdetails(mpd_client_state, response->data, request->method, request->id, p_charbuf1);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Invalid API request", true);
            }
            break;
        case MPD_API_DATABASE_FINGERPRINT:
            if (mpd_client_state->feat_fingerprint == true) {
                je = json_scanf(request->data, sdslen(request->data), "{params: { uri: %Q}}", &p_charbuf1);
                if (je == 1 && strlen(p_charbuf1) > 0) {
                    response->data = mpd_client_put_fingerprint(mpd_client_state, response->data, request->method, request->id, p_charbuf1);
                }
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Fingerprint command not supported", true);
            }
            break;

        case MPD_API_PLAYLIST_RENAME:
            je = json_scanf(request->data, sdslen(request->data), "{params: {from: %Q, to: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                response->data = mpd_client_playlist_rename(config, mpd_client_state, response->data, request->method, request->id, p_charbuf1, p_charbuf2);
            }
            break;            
        case MPD_API_PLAYLIST_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, limit: %u, searchstr: %Q}}", &uint_buf1, &uint_buf2, &p_charbuf1);
            if (je == 3) {
                response->data = mpd_client_put_playlists(config, mpd_client_state, response->data, request->method, request->id, uint_buf1, uint_buf2, p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_CONTENT_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q, offset: %u, limit: %u searchstr: %Q, cols: %M}}", 
                &p_charbuf1, &uint_buf1, &uint_buf2, &p_charbuf2, json_to_tags, tagcols);
            if (je == 5) {
                response->data = mpd_client_put_playlist_list(config, mpd_client_state, response->data, request->method, request->id, p_charbuf1, uint_buf1, uint_buf2, p_charbuf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYLIST_ADD_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {plist: %Q, uri: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                rc = mpd_run_playlist_add(mpd_client_state->mpd_state->conn, p_charbuf1, p_charbuf2);
                if (check_error_and_recover2(mpd_client_state->mpd_state, &response->data, request->method, request->id, false) == true && rc == true) {
                    response->data = jsonrpc_start_phrase(response->data, request->method, request->id, "Added %{uri} to playlist %{playlist}", false);
                    response->data = tojson_char(response->data, "uri", p_charbuf2, true);
                    response->data = tojson_char(response->data, "playlist", p_charbuf1, false);
                    response->data = jsonrpc_end_phrase(response->data);
                }
                else if (rc == false) {
                    response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_add");
                }
            }
            break;
        case MPD_API_PLAYLIST_CLEAR:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_playlist_clear(mpd_client_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_clear");
            }
            break;
        case MPD_API_PLAYLIST_RM_ALL:
            je = json_scanf(request->data, sdslen(request->data), "{params: {type: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_client_playlist_delete_all(config, mpd_client_state, response->data, request->method, request->id, p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_RM_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q, track:%u}}", &p_charbuf1, &uint_buf1);
            if (je == 2) {
                rc = mpd_run_playlist_delete(mpd_client_state->mpd_state->conn, p_charbuf1, uint_buf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_playlist_delete");
            }
            break;
        case MPD_API_PLAYLIST_SHUFFLE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_shared_playlist_shuffle_sort(mpd_client_state->mpd_state, response->data, request->method, request->id, p_charbuf1, "shuffle");
            }
            break;
        case MPD_API_PLAYLIST_SORT:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q, tag:%Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                response->data = mpd_shared_playlist_shuffle_sort(mpd_client_state->mpd_state, response->data, request->method, request->id, p_charbuf1, p_charbuf2);
            }
            break;
        case MPD_API_DATABASE_FILESYSTEM_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset:%u, limit:%u, searchstr:%Q, path:%Q, cols: %M}}", 
                &uint_buf1, &uint_buf2, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 5) {
                response->data = mpd_client_put_filesystem(config, mpd_client_state, response->data, request->method, request->id, p_charbuf2, uint_buf1, uint_buf2, p_charbuf1, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_ADD_TRACK_AFTER:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q, to:%d}}", &p_charbuf1, &int_buf1);
            if (je == 2) {
                rc = mpd_run_add_id_to(mpd_client_state->mpd_state->conn, p_charbuf1, int_buf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_add_id_to");
            }
            break;
        case MPD_API_QUEUE_REPLACE_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q }}", &p_charbuf1);
            if (je == 1 && strlen(p_charbuf1) > 0) {
                rc = mpd_client_queue_replace_with_song(mpd_client_state, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_client_queue_replace_with_song");
            }
            break;
        case MPD_API_QUEUE_ADD_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q}}", &p_charbuf1);
            if (je == 1 && strlen(p_charbuf1) > 0) {
                rc = mpd_run_add(mpd_client_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_add");
            }
            break;
        case MPD_API_QUEUE_ADD_PLAY_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                int_buf1 = mpd_run_add_id(mpd_client_state->mpd_state->conn, p_charbuf1);
                if (int_buf1 != -1) {
                    rc = mpd_run_play_id(mpd_client_state->mpd_state->conn, int_buf1);
                }
                else {
                    rc = false;
                }
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_add_id");
            }
            break;
        case MPD_API_QUEUE_REPLACE_PLAYLIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_client_queue_replace_with_playlist(mpd_client_state, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_client_queue_replace_with_playlist");
            }
            break;
        case MPD_API_QUEUE_ADD_RANDOM:
            je = json_scanf(request->data, sdslen(request->data), "{params: {mode:%u, playlist:%Q, quantity:%d}}", &uint_buf1, &p_charbuf1, &int_buf1);
            if (je == 3) {
                rc = mpd_client_jukebox_add_to_queue(config, mpd_client_state, int_buf1, uint_buf1, p_charbuf1, true);
                if (rc == true) {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Sucessfully added random songs to queue", false);
                }
                else {
                    response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Adding random songs to queue failed", true);
                }
            }
            break;
        case MPD_API_QUEUE_ADD_PLAYLIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_load(mpd_client_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_load");
            }
            break;
        case MPD_API_QUEUE_SAVE:
            je = json_scanf(request->data, sdslen(request->data), "{ params: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_save(mpd_client_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_save");
            }
            break;
        case MPD_API_QUEUE_SEARCH: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, limit: %u, filter: %Q, searchstr: %Q, cols: %M}}", 
                &uint_buf1, &uint_buf2, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 5) {
                response->data = mpd_client_search_queue(mpd_client_state, response->data, request->method, request->id, p_charbuf1, uint_buf1, uint_buf2, p_charbuf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_DATABASE_SEARCH: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {searchstr:%Q, filter:%Q, plist:%Q, offset:%u, limit:%u, cols: %M, replace:%B}}", 
                &p_charbuf1, &p_charbuf2, &p_charbuf3, &uint_buf1, &uint_buf2, json_to_tags, tagcols, &bool_buf1);
            if (je == 7) {
                if (bool_buf1 == true) {
                    rc = mpd_run_clear(mpd_client_state->mpd_state->conn);
                    if (rc == false) {
                        LOG_ERROR("Clearing queue failed");
                    }
                    check_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0);
                }
                response->data = mpd_shared_search(mpd_client_state->mpd_state, response->data, request->method, request->id, 
                    p_charbuf1, p_charbuf2, p_charbuf3, uint_buf1, uint_buf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_DATABASE_SEARCH_ADV: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {expression:%Q, sort:%Q, sortdesc:%B, plist:%Q, offset:%u, limit:%u, cols: %M, replace:%B}}", 
                &p_charbuf1, &p_charbuf2, &bool_buf1, &p_charbuf3, &uint_buf1, &uint_buf2, json_to_tags, tagcols, &bool_buf2);
            if (je == 8) {
                if (bool_buf2 == true) {
                    rc = mpd_run_clear(mpd_client_state->mpd_state->conn);
                    if (rc == false) {
                        LOG_ERROR("Clearing queue failed");
                    }
                    check_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0);
                }
                response->data = mpd_shared_search_adv(mpd_client_state->mpd_state, response->data, request->method, request->id, 
                    p_charbuf1, p_charbuf2, bool_buf1, NULL, p_charbuf3, uint_buf1, uint_buf2, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_SHUFFLE:
            rc = mpd_run_shuffle(mpd_client_state->mpd_state->conn);
            response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_shuffle");
            break;
        case MPD_API_PLAYLIST_RM:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_client_playlist_delete(config, mpd_client_state, response->data, request->method, request->id, p_charbuf1);
            }
            break;
        case MPD_API_SETTINGS_GET:
            response->data = mpd_client_put_settings(mpd_client_state, response->data, request->method, request->id);
            break;
        case MPD_API_DATABASE_STATS:
            response->data = mpd_client_put_stats(config, mpd_client_state, response->data, request->method, request->id);
            break;
        case MPD_API_ALBUMART:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->data = mpd_client_getcover(config, mpd_client_state, response->data, request->method, request->id, p_charbuf1, &response->binary);
            }
            break;
        case MPD_API_DATABASE_GET_ALBUMS:
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, limit: %u, searchstr: %Q, filter: %Q, sort: %Q, sortdesc: %B}}", 
                &uint_buf1, &uint_buf2, &p_charbuf1, &p_charbuf2, &p_charbuf3, &bool_buf1);
            if (je == 6) {
                response->data = mpd_client_put_firstsong_in_albums(mpd_client_state, response->data, request->method, request->id, 
                    p_charbuf1, p_charbuf2, p_charbuf3, bool_buf1, uint_buf1, uint_buf2);
            }
            break;
        case MPD_API_DATABASE_TAG_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, limit: %u, searchstr: %Q, filter: %Q, sort: %Q, sortdesc: %B, tag: %Q}}", 
                &uint_buf1, &uint_buf2, &p_charbuf1, &p_charbuf2, &p_charbuf3, &bool_buf1, &p_charbuf4);
            if (je == 7) {
                response->data = mpd_client_put_db_tag2(config, mpd_client_state, response->data, request->method, request->id,
                    p_charbuf1, p_charbuf2, p_charbuf3, bool_buf1, uint_buf1, uint_buf2, p_charbuf4);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {album: %Q, searchstr: %Q, tag: %Q, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &p_charbuf3, json_to_tags, tagcols);
            if (je == 4) {
                response->data = mpd_client_put_songs_in_album(mpd_client_state, response->data, request->method, request->id, p_charbuf1, p_charbuf2, p_charbuf3, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_TIMER_STARTPLAY:
            je = json_scanf(request->data, sdslen(request->data), "{params: {volume:%u, playlist:%Q, jukeboxMode:%u}}", &uint_buf1, &p_charbuf1, &uint_buf2);
            if (je == 3) {
                response->data = mpd_client_timer_startplay(mpd_client_state, response->data, request->method, request->id, uint_buf1, p_charbuf1, uint_buf2);
            }
            break;
        case MPD_API_URLHANDLERS:
            response->data = mpd_client_put_urlhandlers(mpd_client_state, response->data, request->method, request->id);
            break;
        case MPD_API_PARTITION_LIST:
            response->data = mpd_client_put_partitions(mpd_client_state, response->data, request->method, request->id);
            break;
        case MPD_API_PARTITION_NEW:
            je = json_scanf(request->data, sdslen(request->data), "{params: {name: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_newpartition(mpd_client_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_newpartition");
            }
            break;
        case MPD_API_PARTITION_SWITCH:
            je = json_scanf(request->data, sdslen(request->data), "{params: {name: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_switch_partition(mpd_client_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_switch_partition");
            }
            break;
        case MPD_API_PARTITION_RM:
            je = json_scanf(request->data, sdslen(request->data), "{params: {name: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_delete_partition(mpd_client_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_delete_partition");
            }
            break;
        case MPD_API_PARTITION_OUTPUT_MOVE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {name: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_move_output(mpd_client_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_move_output");
            }
            break;
        case MPD_API_MOUNT_LIST:
            response->data = mpd_client_put_mounts(mpd_client_state, response->data, request->method, request->id);
            break;
        case MPD_API_MOUNT_NEIGHBOR_LIST:
            response->data = mpd_client_put_neighbors(mpd_client_state, response->data, request->method, request->id);
            break;
        case MPD_API_MOUNT_MOUNT:
            je = json_scanf(request->data, sdslen(request->data), "{params: {mountUrl: %Q, mountPoint: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                rc = mpd_run_mount(mpd_client_state->mpd_state->conn, p_charbuf2, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_mount");
            }
            break;
        case MPD_API_MOUNT_UNMOUNT:
            je = json_scanf(request->data, sdslen(request->data), "{params: {mountPoint: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_run_unmount(mpd_client_state->mpd_state->conn, p_charbuf1);
                response->data = respond_with_mpd_error_or_ok(mpd_client_state->mpd_state, response->data, request->method, request->id, rc, "mpd_run_unmount");
            }
            break;
        default:
            response->data = jsonrpc_respond_message(response->data, request->method, request->id, "Unknown request", true);
            LOG_ERROR("Unknown API request: %.*s", sdslen(request->data), request->data);
    }
    FREE_PTR(p_charbuf1);
    FREE_PTR(p_charbuf2);
    FREE_PTR(p_charbuf3);                    
    FREE_PTR(p_charbuf4);
    FREE_PTR(p_charbuf5);
    
    #ifdef DEBUG
    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    LOG_DEBUG("Execution time for %s: %lf", request->method, cpu_time_used);
    #endif

    if (sdslen(response->data) == 0) {
        response->data = jsonrpc_start_phrase(response->data, request->method, request->id, "No response for method %{method}", true);
        response->data = tojson_char(response->data, "method", request->method, false);
        response->data = jsonrpc_end_phrase(response->data);
        LOG_ERROR("No response for cmd_id %u", request->cmd_id);
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
