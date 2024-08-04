/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD client idle event handling
 */

#include "compile_time.h"
#include "src/mpd_client/idle.h"

#include "src/lib/datetime.h"
#include "src/lib/event.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/connection.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/jukebox.h"
#include "src/mpd_client/partitions.h"
#include "src/mpd_client/queue.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mympd_api/last_played.h"
#include "src/mympd_api/mympd_api_handler.h"
#include "src/mympd_api/status.h"
#include "src/mympd_api/timer.h"
#include "src/mympd_api/timer_handlers.h"
#include "src/mympd_api/trigger.h"

#include <string.h>

/**
 * Private definitions
 */

static void mpd_client_idle_partition(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        struct t_work_request *request);
static void mpd_client_parse_idle(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state, unsigned idle_bitmask);

/**
 * Public functions
 */

/**
 * This is the central function to handle api requests and mpd events.
 * It is called from the mympd_api thread.
 * @param mympd_state pointer to the mympd state struct
 * @param request work request from the mympd_api queue
 */
void mpd_client_idle(struct t_mympd_state *mympd_state, struct t_work_request *request) {
    // iterate through all partitions
    struct t_partition_state *partition_state = mympd_state->partition_state;
    do {
        if (partition_state->waiting_events > 0 ||
            partition_state->set_conn_options == true)
        {
            if (partition_state->waiting_events & PFD_TYPE_QUEUE) {
                mpd_client_idle_partition(mympd_state, partition_state, request);
                request = NULL;
            }
            else {
                mpd_client_idle_partition(mympd_state, partition_state, NULL);
            }
            partition_state->waiting_events = 0;
        }
    } while ((partition_state = partition_state->next) != NULL);
    // cleanup
    if (request != NULL) {
        // request was for unknown partition, discard it
        MYMPD_LOG_WARN(NULL, "Discarding request for unknown partition \"%s\"", request->partition);
        if (request->type == REQUEST_TYPE_DEFAULT) {
            struct t_work_response *response = create_response(request);
            response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Unknown partition");
            MYMPD_LOG_DEBUG(NULL, "Send http response to connection %lu: %s", request->conn_id, response->data);
            mympd_queue_push(web_server_queue, response, 0);
        }
        free_request(request);
    }
}

/**
 * Scrobble event
 * @param mympd_state pointer to partition state
 * @param partition_state pointer to partition specific state
 */
void mpd_client_scrobble(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state) {
    #ifdef MYMPD_DEBUG
        char fmt_time[32];
        readable_time(fmt_time, time(NULL));
        MYMPD_LOG_DEBUG(partition_state->name, "Song scrobble time reached: %s", fmt_time);
    #endif
    //add song to the last_played list
    mympd_api_last_played_add_song(partition_state, mympd_state->last_played_count);
    // set stickers
    if (partition_state->mpd_state->feat.stickers == true) {
        stickerdb_inc_play_count(mympd_state->stickerdb, MPD_STICKER_TYPE_SONG,
            partition_state->song_uri, partition_state->song_start_time);
    }
    // scrobble event
    mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_SCROBBLE, partition_state->name, NULL);
}

/**
 * Private functions
 */

/**
 * This function checks the mpd connection state, handles api requests and mpd events per partition.
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to the partition state
 * @param request api request
 */
static void mpd_client_idle_partition(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        struct t_work_request *request)
{
    if (request != NULL) {
        if (is_mpd_disconnected_api_method(request->cmd_id) == true &&
            partition_state->conn_state != MPD_CONNECTED)
        {
            // Handle request if MPD is not connected
            MYMPD_LOG_DEBUG(partition_state->name, "Handle request \"%s\"", get_cmd_id_method_name(request->cmd_id));
            mympd_api_handler(mympd_state, partition_state, request);
            partition_state->waiting_events &= ~(unsigned)PFD_TYPE_QUEUE;
            request = NULL;
        }
        else if (is_mympd_only_api_method(request->cmd_id) == true) {
            // Request that can be handled without a MPD connection
            MYMPD_LOG_DEBUG(partition_state->name, "Handle request \"%s\"", get_cmd_id_method_name(request->cmd_id));
            mympd_api_handler(mympd_state, partition_state, request);
            partition_state->waiting_events &= ~(unsigned)PFD_TYPE_QUEUE;
            request = NULL;
        }
        else if (partition_state->conn_state != MPD_CONNECTED) {
            // Respond with error if MPD is not connected
            if (request->type != REQUEST_TYPE_DISCARD) {
                struct t_work_response *response = create_response(request);
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "MPD disconnected");
                MYMPD_LOG_DEBUG(partition_state->name, "Send http response to connection %lu: %s", request->conn_id, response->data);
                push_response(response);
            }
            else {
                MYMPD_LOG_WARN(partition_state->name, "Discarding request %s, MPD disconnected.", get_cmd_id_method_name(request->cmd_id));
            }
            free_request(request);
            partition_state->waiting_events &= ~(unsigned)PFD_TYPE_QUEUE;
            request = NULL;
        }
    }

    // Check if we need to exit the idle mode
    if (partition_state->waiting_events == 0) {
        return;
    }

    MYMPD_LOG_DEBUG(partition_state->name, "Leaving mpd idle mode");
    if (mpd_send_noidle(partition_state->conn) == false) {
        mympd_check_error_and_recover(partition_state, NULL, "mpd_send_noidle");
        return;
    }
    if (partition_state->waiting_events & PFD_TYPE_PARTITION) {
        // handle idle events
        MYMPD_LOG_DEBUG(partition_state->name, "Checking for idle events");
        enum mpd_idle idle_bitmask = mpd_recv_idle(partition_state->conn, false);
        mpd_client_parse_idle(mympd_state, partition_state, idle_bitmask);
    }
    else {
        mpd_response_finish(partition_state->conn);
    }
    // set mpd connection options
    if (partition_state->set_conn_options == true &&
        mpd_client_set_connection_options(partition_state) == true)
    {
        partition_state->set_conn_options = false;
    }
    // run jukebox
    if (partition_state->waiting_events & PFD_TYPE_TIMER_JUKEBOX) {
        jukebox_run(mympd_state, partition_state, &mympd_state->album_cache);
    }
    // an api request is there
    if (request != NULL) {
        //Handle request
        MYMPD_LOG_DEBUG(partition_state->name, "Handle API request \"%s\"", get_cmd_id_method_name(request->cmd_id));
        mympd_api_handler(mympd_state, partition_state, request);
    }
    // re-enter idle mode
    if (partition_state->conn_state == MPD_CONNECTED) {
        MYMPD_LOG_DEBUG(partition_state->name, "Entering mpd idle mode");
        if (mpd_send_idle_mask(partition_state->conn, partition_state->idle_mask) == false) {
            mympd_check_error_and_recover(partition_state, NULL, "mpd_send_idle_mask");
        }
    }
}

/**
 * Handles mpd idle events
 * @param mympd_state pointer to partition state
 * @param partition_state pointer to partition specific state
 * @param idle_bitmask triggered mpd idle events as bitmask
 */
static void mpd_client_parse_idle(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state, unsigned idle_bitmask) {
    sds buffer = sdsempty();
    for (unsigned j = 0;; j++) {
        enum mpd_idle idle_event = 1 << j;
        const char *idle_name = mpd_idle_name(idle_event);
        if (idle_name == NULL) {
            //loop end condition
            break;
        }
        if (idle_bitmask & idle_event) {
            MYMPD_LOG_INFO(partition_state->name, "MPD idle event \"%s\"", idle_name);
            switch(idle_event) {
                case MPD_IDLE_DATABASE:
                    //database has changed - global event
                    MYMPD_LOG_INFO(partition_state->name, "MPD database has changed");
                    buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_DATABASE);
                    //add timer for cache updates
                    if (mympd_state->mpd_state->feat.tags == true) {
                        mympd_api_timer_replace(&mympd_state->timer_list, 2, TIMER_ONE_SHOT_REMOVE,
                            timer_handler_by_id, TIMER_ID_CACHES_CREATE, NULL);
                    }
                    break;
                case MPD_IDLE_STORED_PLAYLIST:
                    //a playlist has changed - global event
                    buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_STORED_PLAYLIST);
                    break;
                case MPD_IDLE_UPDATE:
                    //database update has started or is finished - global event
                    buffer = mympd_api_status_updatedb_state(partition_state, buffer);
                    break;
                case MPD_IDLE_PARTITION:
                    //partitions are changed - global event
                    partitions_populate(mympd_state);
                    break;
                case MPD_IDLE_QUEUE: {
                    //MPD_IDLE_PLAYLIST is the same
                    //queue has changed - partition specific event
                    buffer = mpd_client_queue_status_print(partition_state, &mympd_state->album_cache, buffer);
                    //jukebox enabled
                    if (partition_state->jukebox.mode != JUKEBOX_OFF &&
                        partition_state->queue_length < partition_state->jukebox.queue_length)
                    {
                        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox mode: %s", jukebox_mode_lookup(partition_state->jukebox.mode));
                        jukebox_run(mympd_state, partition_state, &mympd_state->album_cache);
                    }
                    //autoPlay enabled
                    if (partition_state->auto_play == true &&
                        partition_state->queue_length > 0)
                    {
                        if (partition_state->play_state != MPD_STATE_PLAY) {
                            MYMPD_LOG_INFO(partition_state->name, "Auto play enabled, start playing");
                            if (mpd_run_play(partition_state->conn) == false) {
                                mympd_check_error_and_recover(partition_state, NULL, "mpd_run_play");
                            }
                        }
                        else {
                            MYMPD_LOG_DEBUG(partition_state->name, "Auto play enabled, already playing");
                        }
                    }
                    break;
                }
                case MPD_IDLE_PLAYER:
                    //player status has changed - partition specific event
                    if (partition_state->mpd_state->feat.stickers == true &&
                        partition_state->song_id > -1)
                    {
                        //set song elapsed sticker
                        time_t now = time(NULL);
                        time_t elapsed = now - partition_state->song_start_time;
                        time_t total_time = partition_state->song_end_time - partition_state->song_start_time - elapsed;
                        if (elapsed < SCROBBLE_TIME_MIN ||
                            total_time < SCROBBLE_TIME_MIN)
                        {
                            //10 seconds inaccuracy
                            elapsed = 0;
                        }
                        stickerdb_set_elapsed(mympd_state->stickerdb, MPD_STICKER_TYPE_SONG, partition_state->song_uri, elapsed);
                    }
                    //get and put mpd state
                    buffer = mympd_api_status_get(partition_state, &mympd_state->album_cache, buffer, 0, RESPONSE_TYPE_JSONRPC_NOTIFY);
                    //check if song has changed
                    if (partition_state->song_id != partition_state->last_song_id &&
                        partition_state->last_skipped_id != partition_state->last_song_id &&
                        sdslen(partition_state->last_song_uri) > 0)
                    {
                        //check if last song was skipped
                        time_t last_song_elapsed = partition_state->song_start_time - partition_state->last_song_start_time;
                        if (partition_state->last_song_start_time + last_song_elapsed < partition_state->last_song_end_time - SCROBBLE_TIME_MIN &&
                            partition_state->last_song_start_time > 0)
                        {
                            MYMPD_LOG_DEBUG(partition_state->name, "Song \"%s\" skipped", partition_state->last_song_uri);
                            if (partition_state->mpd_state->feat.stickers == true) {
                                stickerdb_inc_skip_count(mympd_state->stickerdb, MPD_STICKER_TYPE_SONG, partition_state->last_song_uri);
                            }
                            partition_state->last_skipped_id = partition_state->last_song_id;
                            mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_SKIPPED, partition_state->name, NULL);
                        }
                    }
                    break;
                case MPD_IDLE_MIXER:
                    //volume has changed - partition specific event
                    buffer = mympd_api_status_volume_get(partition_state, buffer, 0, RESPONSE_TYPE_JSONRPC_NOTIFY);
                    break;
                case MPD_IDLE_OUTPUT:
                    //outputs are changed - partition specific event
                    buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_OUTPUTS);
                    break;
                case MPD_IDLE_OPTIONS:
                    //mpd playback options are changed - partition specific event
                    mpd_client_queue_status_update(partition_state);
                    buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_OPTIONS);
                    break;
                default: {
                    //other idle events not used
                }
            }
            //check for attached triggers
            mympd_api_trigger_execute(&mympd_state->trigger_list, (enum trigger_events)idle_event, partition_state->name, NULL);
            //broadcast event to all websockets
            if (sdslen(buffer) > 0) {
                switch(idle_event) {
                    case MPD_IDLE_DATABASE:
                    case MPD_IDLE_PARTITION:
                    case MPD_IDLE_STORED_PLAYLIST:
                    case MPD_IDLE_UPDATE:
                        //broadcast to all partitions
                        ws_notify(buffer, MPD_PARTITION_ALL);
                        break;
                    default:
                        //broadcast to specific partition
                        ws_notify(buffer, partition_state->name);
                }
                sdsclear(buffer);
            }
        }
    }
    FREE_SDS(buffer);
}
