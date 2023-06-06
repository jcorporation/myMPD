/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/idle.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/lib/sticker_cache.h"
#include "src/mpd_client/connection.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/jukebox.h"
#include "src/mpd_client/partitions.h"
#include "src/mympd_api/last_played.h"
#include "src/mympd_api/mympd_api_handler.h"
#include "src/mympd_api/queue.h"
#include "src/mympd_api/status.h"
#include "src/mympd_api/timer.h"
#include "src/mympd_api/timer_handlers.h"
#include "src/mympd_api/trigger.h"

#include <poll.h>
#include <string.h>

/**
 * Private definitions
 */

static void mpd_client_idle_partition(struct t_partition_state *partition_state,
        bool mpd_idle_event_waiting, struct t_work_request *request);
static void mpd_client_parse_idle(struct t_partition_state *partition_state, unsigned idle_bitmask);
static bool update_mympd_caches(struct t_mympd_state *mympd_state, time_t timeout);

/**
 * Public functions
 */

/**
 * This is the central function to handle api requests and mpd events.
 * It is called from the mympd_api thread.
 * @param mympd_state pointer to the mympd state struct
 */
void mpd_client_idle(struct t_mympd_state *mympd_state) {
    //poll all mpd connection fds
    partitions_get_fds(mympd_state);
    if (mympd_state->nfds > 0) {
        int pollrc = poll(mympd_state->fds, mympd_state->nfds, 50);
        if (pollrc < 0) {
            MYMPD_LOG_ERROR(NULL, "Error polling mpd connection");
        }
    }
    //check the mympd_api_queue
    struct t_work_request *request = mympd_queue_shift(mympd_api_queue, 50, 0);
    //iterate through all partitions
    struct t_partition_state *partition_state = mympd_state->partition_state;
    int i = 0;
    bool mpd_idle_event_waiting;
    do {
        if (partition_state->conn_state == MPD_CONNECTED) {
            //only connected partitions has a fd
            mpd_idle_event_waiting = mympd_state->fds[i].revents & POLLIN ? true : false;
            i++;
        }
        else {
            mpd_idle_event_waiting = false;
        }
        if (request != NULL &&
            strcmp(request->partition, partition_state->name) == 0)
        {
            //API request is for this partition
            mpd_client_idle_partition(partition_state, mpd_idle_event_waiting, request);
            request = NULL;
        }
        else {
            mpd_client_idle_partition(partition_state, mpd_idle_event_waiting, NULL);
        }
    } while ((partition_state = partition_state->next) != NULL);
    //cleanup
    if (request != NULL) {
        //request was for unknown partition, discard it
        MYMPD_LOG_WARN(NULL, "Discarding request for unknown partition \"%s\"", request->partition);
        if (request->conn_id > -1) {
            struct t_work_response *response = create_response(request);
            response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Unknown partition");
            MYMPD_LOG_DEBUG(NULL, "Send http response to connection %lld: %s", request->conn_id, response->data);
            mympd_queue_push(web_server_queue, response, 0);
        }
        free_request(request);
    }
}

/**
 * Private functions
 */

/**
 * This function handles api requests and mpd events per partition.
 * @param partition_state pointer to the partition state
 * @param mpd_idle_event_waiting true if mpd idle event is waiting, else false
 * @param request api request
 */
static void mpd_client_idle_partition(struct t_partition_state *partition_state,
        bool mpd_idle_event_waiting, struct t_work_request *request)
{
    //Handle api requests if mpd is not connected
    if (partition_state->conn_state != MPD_CONNECTED &&
        request != NULL)
    {
        if (is_mympd_only_api_method(request->cmd_id) == true) {
            //request that are handled without a mpd connection
            MYMPD_LOG_DEBUG(partition_state->name, "Handle request \"%s\" (mpd disconnected)", get_cmd_id_method_name(request->cmd_id));
            mympd_api_handler(partition_state, request);
        }
        else {
            //other requests not allowed
            if (request->conn_id > -1) {
                struct t_work_response *response = create_response(request);
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "MPD disconnected");
                MYMPD_LOG_DEBUG(partition_state->name, "Send http response to connection %lld: %s", request->conn_id, response->data);
                mympd_queue_push(web_server_queue, response, 0);
            }
            free_request(request);
        }
        request = NULL;
    }

    switch (partition_state->conn_state) {
        case MPD_WAIT: {
            time_t now = time(NULL);
            if (now > partition_state->reconnect_time) {
                //wait time elapsed, try to reconnect
                partition_state->conn_state = MPD_DISCONNECTED;
            }
            break;
        }
        case MPD_DISCONNECTED:
            //try to connect
            MYMPD_LOG_INFO(partition_state->name, "Creating mpd connection for partition \"%s\"", partition_state->name);
            if (mpd_client_connect(partition_state, partition_state->is_default) == false) {
                break;
            }
            if (partition_state->is_default == true) {
                //check version
                if (mpd_connection_cmp_server_version(partition_state->conn, 0, 21, 0) < 0) {
                    MYMPD_LOG_EMERG(partition_state->name, "MPD version too old, myMPD supports only MPD version >= 0.21");
                    s_signal_received = 1;
                    break;
                }
            }
            //we are connected
            if (partition_state->is_default == false) {
                //change partition
                MYMPD_LOG_INFO(partition_state->name, "Switching to partition \"%s\"", partition_state->name);
                mpd_run_switch_partition(partition_state->conn, partition_state->name);
                if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_switch_partition") == false) {
                    MYMPD_LOG_ERROR(partition_state->name, "Could not switch to partition \"%s\"", partition_state->name);
                    mpd_client_disconnect(partition_state, MPD_FAILURE);
                    break;
                }
            }
            if (partition_state->is_default == true) {
                //initiate cache updates
                update_mympd_caches(partition_state->mympd_state, 2);
                //set timer for smart playlist update
                mympd_api_timer_replace(&partition_state->mympd_state->timer_list, 30, (int)partition_state->mympd_state->smartpls_interval,
                    timer_handler_by_id, TIMER_ID_SMARTPLS_UPDATE, NULL);
                //populate the partition list
                if (partition_state->mpd_state->feat_partitions == true) {
                    partitions_populate(partition_state->mympd_state);
                }
            }
            //jukebox
            if (partition_state->jukebox_mode != JUKEBOX_OFF) {
                jukebox_run(partition_state);
            }
            if (mpd_send_idle(partition_state->conn) == false) {
                MYMPD_LOG_ERROR(partition_state->name, "Entering idle mode failed");
                partition_state->conn_state = MPD_FAILURE;
            }
            send_jsonrpc_event(JSONRPC_EVENT_MPD_CONNECTED, partition_state->name);
            mympd_api_trigger_execute(&partition_state->mympd_state->trigger_list, TRIGGER_MYMPD_CONNECTED, partition_state->name);
            break;
        case MPD_FAILURE:
            MYMPD_LOG_ERROR(partition_state->name, "MPD connection failed");
            // fall through
        case MPD_DISCONNECT:
        case MPD_DISCONNECT_INSTANT:
            mpd_client_disconnect(partition_state, partition_state->conn_state);
            //set wait time for next connection attempt
            if (partition_state->conn_state != MPD_DISCONNECT_INSTANT) {
                partition_state->conn_state = MPD_WAIT;
                if (partition_state->reconnect_interval < 20) {
                    partition_state->reconnect_interval += 2;
                }
                partition_state->reconnect_time = time(NULL) + partition_state->reconnect_interval;
                MYMPD_LOG_INFO(partition_state->name, "Waiting %lld seconds before reconnection", (long long)partition_state->reconnect_interval);
            }
            else {
                partition_state->conn_state = MPD_DISCONNECTED;
                partition_state->reconnect_interval = 0;
                partition_state->reconnect_time = 0;
            }
            break;
        case MPD_CONNECTED: {
            //initial states
            bool jukebox_add_song = false;
            bool set_played = false;
            bool set_stickers = partition_state->is_default &&
                partition_state->mpd_state->sticker_queue.length > 0 &&
                partition_state->mpd_state->sticker_cache.building == false;
            //handle jukebox and last played only in mpd play state
            if (partition_state->play_state == MPD_STATE_PLAY) {
                time_t now = time(NULL);
                //check if we should set the played state of current song
                if (now > partition_state->song_scrobble_time &&
                    partition_state->song_scrobble_time > 0 &&
                    partition_state->last_song_scrobble_time != partition_state->song_scrobble_time)
                {
                    MYMPD_LOG_DEBUG(partition_state->name, "Song scrobble time reached: %lld", (long long)partition_state->song_scrobble_time);
                    set_played = true;
                }
                //check if the jukebox should add a song
                if (partition_state->jukebox_mode != JUKEBOX_OFF) {
                    //add time is crossfade + 10s before song end time
                    time_t add_time = partition_state->song_end_time - (partition_state->crossfade + 10);
                    if (now > add_time &&
                        add_time > 0 &&
                        partition_state->queue_length <= partition_state->jukebox_queue_length)
                    {
                        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox should add song");
                        jukebox_add_song = true;
                    }
                }
            }
            //check if we need to exit the idle mode
            if (mpd_idle_event_waiting == true ||             //idle event waiting
                request != NULL ||                            //api was called
                jukebox_add_song == true ||                   //jukebox trigger
                set_played == true ||                         //playstate of song must be set
                partition_state->set_conn_options == true ||  //connection options must be set
                set_stickers == true)                         //we must set waiting stickers
            {
                MYMPD_LOG_DEBUG(partition_state->name, "Leaving mpd idle mode");
                if (mpd_send_noidle(partition_state->conn) == false) {
                    mympd_check_error_and_recover(partition_state, NULL, "mpd_send_noidle");
                    partition_state->conn_state = MPD_FAILURE;
                    break;
                }
                if (mpd_idle_event_waiting == true) {
                    //Handle idle events
                    MYMPD_LOG_DEBUG(partition_state->name, "Checking for idle events");
                    enum mpd_idle idle_bitmask = mpd_recv_idle(partition_state->conn, false);
                    mpd_client_parse_idle(partition_state, idle_bitmask);
                }
                else {
                    mpd_response_finish(partition_state->conn);
                }
                //set mpd connection options
                if (partition_state->set_conn_options == true &&
                    mpd_client_set_connection_options(partition_state) == true)
                {
                    partition_state->set_conn_options = false;
                }
                //set song played state
                if (set_played == true) {
                    partition_state->last_scrobbled_id = partition_state->song_id;
                    partition_state->last_song_scrobble_time = partition_state->song_scrobble_time;

                    if (partition_state->mpd_state->last_played_count > 0) {
                        //add song to the last_played list
                        mympd_api_last_played_add_song(partition_state, partition_state->song_id);
                    }
                    if (partition_state->mpd_state->feat_stickers == true) {
                        //set stickers
                        sticker_inc_play_count(&partition_state->mpd_state->sticker_queue,
                            partition_state->song_uri);
                        sticker_set_last_played(&partition_state->mpd_state->sticker_queue,
                            partition_state->song_uri, partition_state->last_song_start_time);
                    }
                    //scrobble event
                    mympd_api_trigger_execute(&partition_state->mympd_state->trigger_list, TRIGGER_MYMPD_SCROBBLE, partition_state->name);
                }
                //trigger jukebox
                if (jukebox_add_song == true) {
                    jukebox_run(partition_state);
                }
                //an api request is there
                if (request != NULL) {
                    //Handle request
                    MYMPD_LOG_DEBUG(partition_state->name, "Handle API request \"%s\"", get_cmd_id_method_name(request->cmd_id));
                    mympd_api_handler(partition_state, request);
                }
                //process sticker queue
                if (partition_state->is_default == true) {
                    if (partition_state->mpd_state->feat_stickers == true &&
                        partition_state->mpd_state->sticker_queue.length > 0)
                    {
                        MYMPD_LOG_DEBUG(partition_state->name, "Processing sticker queue");
                        sticker_dequeue(&partition_state->mpd_state->sticker_queue,
                            &partition_state->mpd_state->sticker_cache, partition_state);
                    }
                }
                //reenter idle mode
                if (partition_state->conn_state == MPD_CONNECTED) {
                    MYMPD_LOG_DEBUG(partition_state->name, "Entering mpd idle mode");
                    if (mpd_send_idle_mask(partition_state->conn, partition_state->idle_mask) == false) {
                        mympd_check_error_and_recover(partition_state, NULL, "mpd_send_idle_mask");
                        partition_state->conn_state = MPD_FAILURE;
                    }
                }
            }
            break;
        }
        case MPD_REMOVED:
            MYMPD_LOG_DEBUG(partition_state->name, "removed");
            break;
        default:
            MYMPD_LOG_ERROR(partition_state->name, "Invalid mpd connection state");
    }
}

/**
 * Handles mpd idle events
 * @param partition_state pointer to partition specific state
 * @param idle_bitmask triggered mpd idle events as bitmask
 */
static void mpd_client_parse_idle(struct t_partition_state *partition_state, unsigned idle_bitmask) {
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
                    update_mympd_caches(partition_state->mympd_state, 10);
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
                    partitions_populate(partition_state->mympd_state);
                    break;
                case MPD_IDLE_QUEUE: {
                    //MPD_IDLE_PLAYLIST is the same
                    //queue has changed - partition specific event
                    buffer = mympd_api_queue_status(partition_state, buffer);
                    //jukebox enabled
                    if (partition_state->jukebox_mode != JUKEBOX_OFF &&
                        partition_state->queue_length < partition_state->jukebox_queue_length)
                    {
                        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox mode: %s", jukebox_mode_lookup(partition_state->jukebox_mode));
                        jukebox_run(partition_state);
                    }
                    //autoPlay enabled
                    if (partition_state->auto_play == true &&
                        partition_state->queue_length > 0)
                    {
                        if (partition_state->play_state != MPD_STATE_PLAY) {
                            MYMPD_LOG_INFO(partition_state->name, "AutoPlay enabled, start playing");
                            if (mpd_run_play(partition_state->conn) == false) {
                                mympd_check_error_and_recover(partition_state, NULL, "mpd_run_play");
                            }
                        }
                        else {
                            MYMPD_LOG_DEBUG(partition_state->name, "Autoplay enabled, already playing");
                        }
                    }
                    break;
                }
                case MPD_IDLE_PLAYER:
                    //player status has changed - partition specific event
                    if (partition_state->mpd_state->feat_stickers == true &&
                        partition_state->song_id > -1)
                    {
                        //set song elapsed sticker
                        time_t now = time(NULL);
                        time_t elapsed = now - partition_state->song_start_time;
                        time_t total_time = partition_state->song_end_time - partition_state->song_start_time - elapsed;
                        if (elapsed < 10 ||
                            total_time < 10)
                        {
                            //10 seconds inaccuracy
                            elapsed = 0;
                        }
                        sticker_set_elapsed(&partition_state->mpd_state->sticker_queue, partition_state->song_uri, elapsed);
                    }
                    //get and put mpd state
                    buffer = mympd_api_status_get(partition_state, buffer, 0, RESPONSE_TYPE_NOTIFY);
                    //check if song has changed
                    if (partition_state->song_id != partition_state->last_song_id &&
                        partition_state->last_skipped_id != partition_state->last_song_id &&
                        partition_state->last_song_uri != NULL)
                    {
                        time_t now = time(NULL);
                        if (partition_state->mpd_state->feat_stickers == true &&  //stickers enabled
                            partition_state->last_song_scrobble_time > now)       //time is in the future
                        {
                            //last song skipped
                            time_t elapsed = now - partition_state->last_song_start_time;
                            if (elapsed > 10 &&
                                partition_state->last_song_start_time > 0 &&
                                sdslen(partition_state->last_song_uri) > 0)
                            {
                                MYMPD_LOG_DEBUG(partition_state->name, "Song \"%s\" skipped", partition_state->last_song_uri);
                                if (partition_state->mpd_state->feat_stickers == true) {
                                    sticker_inc_skip_count(&partition_state->mpd_state->sticker_queue, partition_state->last_song_uri);
                                    sticker_set_last_skipped(&partition_state->mpd_state->sticker_queue, partition_state->last_song_uri);
                                }
                                partition_state->last_skipped_id = partition_state->last_song_id;
                            }
                        }
                    }
                    break;
                case MPD_IDLE_MIXER:
                    //volume has changed - partition specific event
                    buffer = mympd_api_status_volume_get(partition_state, buffer, 0, RESPONSE_TYPE_NOTIFY);
                    break;
                case MPD_IDLE_OUTPUT:
                    //outputs are changed - partition specific event
                    buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_OUTPUTS);
                    break;
                case MPD_IDLE_OPTIONS:
                    //mpd playback options are changed - partition specific event
                    mympd_api_queue_status(partition_state, NULL);
                    buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_OPTIONS);
                    break;
                default: {
                    //other idle events not used
                }
            }
            //check for attached triggers
            mympd_api_trigger_execute(&partition_state->mympd_state->trigger_list, (enum trigger_events)idle_event, partition_state->name);
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

/**
 * Checks if we should create the caches and adds an one-shot timer
 * We do not create the caches instantly to debounce MPD_IDLE_DATABASE events
 * @param mympd_state pointer to the central mympd_state struct
 * @param timeout seconds after the timer triggers
 * @return true on success else false
 */
static bool update_mympd_caches(struct t_mympd_state *mympd_state, time_t timeout) {
    if (mympd_state->mpd_state->feat_stickers == false &&
        mympd_state->mpd_state->feat_tags == false)
    {
        MYMPD_LOG_DEBUG(NULL, "Caches are disabled");
        return true;
    }
    MYMPD_LOG_DEBUG(NULL, "Adding timer to update the caches");
    return mympd_api_timer_replace(&mympd_state->timer_list, timeout, TIMER_ONE_SHOT_REMOVE,
            timer_handler_by_id, TIMER_ID_CACHES_CREATE, NULL);
}
