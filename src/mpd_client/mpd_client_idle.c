/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_idle.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../lib/sticker_cache.h"
#include "../lib/utility.h"
#include "../mpd_worker/mpd_worker.h"
#include "../mympd_api/mympd_api_handler.h"
#include "../mympd_api/mympd_api_last_played.h"
#include "../mympd_api/mympd_api_queue.h"
#include "../mympd_api/mympd_api_status.h"
#include "../mympd_api/mympd_api_timer.h"
#include "../mympd_api/mympd_api_timer_handlers.h"
#include "../mympd_api/mympd_api_trigger.h"
#include "mpd_client_connection.h"
#include "mpd_client_errorhandler.h"
#include "mpd_client_features.h"
#include "mpd_client_jukebox.h"
#include "mpd_client_tags.h"

#include <poll.h>
#include <string.h>

/**
 * Private definitions
 */

static bool update_mympd_caches(struct t_mpd_shared_state *mpd_shared_state, struct t_timer_list *timer_list, time_t timeout);
static void mpd_client_parse_idle(struct t_partition_state *partition_state, unsigned idle_bitmask,
    struct t_timer_list *timer_list, struct t_list *trigger_list);

/**
 * Public functions
 */

/**
 * This is the central function to handle api requests and mpd events.
 * It is called from the mympd_api thread.
 * @param mympd_state pointer to the mympd state struct
 */
void mpd_client_idle(struct t_mympd_state *mympd_state) {
    switch (mympd_state->partition_state->conn_state) {
        case MPD_WAIT: {
            time_t now = time(NULL);
            if (now > mympd_state->partition_state->reconnect_time) {
                //wait time elapsed, try to reconnect
                mympd_state->partition_state->conn_state = MPD_DISCONNECTED;
                break;
            }
            //process mympd_api queue
            struct t_work_request *request = mympd_queue_shift(mympd_api_queue, 50, 0);
            if (request != NULL) {
                MYMPD_LOG_DEBUG("Handle request (mpd disconnected)");
                if (is_mympd_only_api_method(request->cmd_id) == true) {
                    mympd_api_handler(mympd_state, request);
                }
                else {
                    //other requests not allowed
                    if (request->conn_id > -1) {
                        struct t_work_response *response = create_response(request);
                        response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                            JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "MPD disconnected");
                        MYMPD_LOG_DEBUG("Send http response to connection %lld: %s", request->conn_id, response->data);
                        mympd_queue_push(web_server_queue, response, 0);
                    }
                    free_request(request);
                }
                break;
            }
            //small pause to prevent high cpu usage
            my_msleep(100);
            break;
        }
        case MPD_DISCONNECTED:
            //try to connect
            if (mpd_client_connect(mympd_state->partition_state) == false) {
                break;
            }
            //check version
            if (mpd_connection_cmp_server_version(mympd_state->partition_state->conn, 0, 21, 0) < 0) {
                MYMPD_LOG_EMERG("MPD version too old, myMPD supports only MPD version >= 0.21.0");
                s_signal_received = 1;
            }
            //we are connected
            send_jsonrpc_event(JSONRPC_EVENT_MPD_CONNECTED);
            //get mpd features
            mpd_client_mpd_features(mympd_state);
            //initiate cache updates
            update_mympd_caches(mympd_state->mpd_shared_state, &mympd_state->timer_list, 2);
            //set timer for smart playlist update
            mympd_api_timer_replace(&mympd_state->timer_list, 30, (int)mympd_state->smartpls_interval, timer_handler_by_id, TIMER_ID_SMARTPLS_UPDATE, NULL);
            //jukebox
            if (mympd_state->partition_state->jukebox_mode != JUKEBOX_OFF) {
                mpd_client_jukebox(mympd_state->partition_state);
            }
            if (mpd_send_idle(mympd_state->partition_state->conn) == false) {
                MYMPD_LOG_ERROR("Entering idle mode failed");
                mympd_state->partition_state->conn_state = MPD_FAILURE;
            }
            mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_CONNECTED);
            break;
        case MPD_FAILURE:
            MYMPD_LOG_ERROR("MPD connection failed");
            send_jsonrpc_event(JSONRPC_EVENT_MPD_DISCONNECTED);
            mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_DISCONNECTED);
            // fall through
        case MPD_DISCONNECT:
        case MPD_DISCONNECT_INSTANT:
            mpd_client_disconnect(mympd_state->partition_state);
            //set wait time for next connection attempt
            if (mympd_state->partition_state->conn_state != MPD_DISCONNECT_INSTANT) {
                mympd_state->partition_state->conn_state = MPD_WAIT;
                if (mympd_state->partition_state->reconnect_interval < 20) {
                    mympd_state->partition_state->reconnect_interval += 2;
                }
                mympd_state->partition_state->reconnect_time = time(NULL) + mympd_state->partition_state->reconnect_interval;
                MYMPD_LOG_INFO("Waiting %lld seconds before reconnection", (long long)mympd_state->partition_state->reconnect_interval);
            }
            else {
                mympd_state->partition_state->conn_state = MPD_DISCONNECTED;
                mympd_state->partition_state->reconnect_interval = 0;
                mympd_state->partition_state->reconnect_time = 0;
            }
            break;
        case MPD_CONNECTED: {
            //check for waiting mpd idle events
            struct pollfd fds[1];
            fds[0].fd = mpd_connection_get_fd(mympd_state->partition_state->conn);
            fds[0].events = POLLIN;
            int pollrc = poll(fds, 1, 50);
            //initial states
            bool jukebox_add_song = false;
            bool set_played = false;
            //check the queue
            struct t_work_request *request = mympd_queue_shift(mympd_api_queue, 50, 0);
            //handle jukebox and last played only in mpd play state
            if (mympd_state->partition_state->play_state == MPD_STATE_PLAY) {
                time_t now = time(NULL);
                //check if we should set the played state of current song
                if (now > mympd_state->partition_state->set_song_played_time &&
                    mympd_state->partition_state->set_song_played_time > 0 &&
                    mympd_state->partition_state->last_last_played_id != mympd_state->partition_state->song_id)
                {
                    MYMPD_LOG_DEBUG("Song has played half: %lld", (long long)mympd_state->partition_state->set_song_played_time);
                    set_played = true;
                }
                //check if the jukebox should add a song
                if (mympd_state->partition_state->jukebox_mode != JUKEBOX_OFF) {
                    //add time is crossfade + 10s before song end time
                    time_t add_time = mympd_state->partition_state->song_end_time - (mympd_state->partition_state->crossfade + 10);
                    if (now > add_time &&
                        add_time > 0 &&
                        mympd_state->partition_state->queue_length <= mympd_state->partition_state->jukebox_queue_length)
                    {
                        MYMPD_LOG_DEBUG("Jukebox should add song");
                        jukebox_add_song = true;
                    }
                }
            }
            //check if we need to exit the idle mode
            if (pollrc > 0 ||                                            //idle event waiting
                request != NULL ||                                       //api was called
                jukebox_add_song == true ||                              //jukebox trigger
                set_played == true ||                                    //playstate of song must be set
                mympd_state->mpd_shared_state->sticker_queue.length > 0) //we must set waiting stickers
            {
                MYMPD_LOG_DEBUG("Leaving mpd idle mode");
                if (mpd_send_noidle(mympd_state->partition_state->conn) == false) {
                    mympd_check_error_and_recover(mympd_state->partition_state);
                    mympd_state->partition_state->conn_state = MPD_FAILURE;
                    break;
                }
                if (pollrc > 0) {
                    //Handle idle events
                    MYMPD_LOG_DEBUG("Checking for idle events");
                    enum mpd_idle idle_bitmask = mpd_recv_idle(mympd_state->partition_state->conn, false);
                    mpd_client_parse_idle(mympd_state->partition_state, idle_bitmask, &mympd_state->timer_list, &mympd_state->trigger_list);
                }
                else {
                    mpd_response_finish(mympd_state->partition_state->conn);
                }
                //set song played state
                if (set_played == true) {
                    mympd_state->partition_state->last_last_played_id = mympd_state->partition_state->song_id;

                    if (mympd_state->mpd_shared_state->last_played_count > 0) {
                        mympd_api_last_played_add_song(mympd_state->partition_state, mympd_state->partition_state->song_id);
                    }
                    if (mympd_state->mpd_shared_state->feat_mpd_stickers == true) {
                        sticker_inc_play_count(&mympd_state->mpd_shared_state->sticker_queue, mympd_state->partition_state->song_uri);
                        sticker_set_last_played(&mympd_state->mpd_shared_state->sticker_queue, mympd_state->partition_state->song_uri, mympd_state->partition_state->last_song_start_time);
                    }
                    mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_SCROBBLE);
                }
                //trigger jukebox
                if (jukebox_add_song == true) {
                    mpd_client_jukebox(mympd_state->partition_state);
                }
                //an api request is there
                if (request != NULL) {
                    //Handle request
                    MYMPD_LOG_DEBUG("Handle API request");
                    mympd_api_handler(mympd_state, request);
                }
                //process sticker queue
                if (mympd_state->mpd_shared_state->feat_mpd_stickers == true &&
                    mympd_state->mpd_shared_state->sticker_queue.length > 0)
                {
                    MYMPD_LOG_DEBUG("Processing sticker queue");
                    sticker_dequeue(&mympd_state->mpd_shared_state->sticker_queue, &mympd_state->mpd_shared_state->sticker_cache, mympd_state->partition_state);
                }
                //reenter idle mode
                MYMPD_LOG_DEBUG("Entering mpd idle mode");
                if (mpd_send_idle(mympd_state->partition_state->conn) == false) {
                    mympd_check_error_and_recover(mympd_state->partition_state);
                    mympd_state->partition_state->conn_state = MPD_FAILURE;
                }
            }
            break;
        }
        default:
            MYMPD_LOG_ERROR("Invalid mpd connection state");
    }
}

/**
 * Private functions
 */

/**
 * Handles mpd idle events
 * @param partition_state pointer to partition specific states
 * @param idle_bitmask triggered mpd idle events as bitmask
 * @param timer_list pointer to the timer_list
 * @param trigger_list pointer to the trigger_list
 */
static void mpd_client_parse_idle(struct t_partition_state *partition_state, unsigned idle_bitmask,
    struct t_timer_list *timer_list, struct t_list *trigger_list) {
    sds buffer = sdsempty();
    for (unsigned j = 0;; j++) {
        enum mpd_idle idle_event = 1 << j;
        const char *idle_name = mpd_idle_name(idle_event);
        if (idle_name == NULL) {
            break;
        }
        if (idle_bitmask & idle_event) {
            MYMPD_LOG_INFO("MPD idle event: %s", idle_name);
            switch(idle_event) {
                case MPD_IDLE_DATABASE:
                    //database has changed
                    MYMPD_LOG_INFO("MPD database has changed");
                    buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_DATABASE);
                    //add timer for cache updates
                    update_mympd_caches(partition_state->mpd_shared_state, timer_list, 10);
                    break;
                case MPD_IDLE_STORED_PLAYLIST:
                    //a playlist has changed
                    buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_STORED_PLAYLIST);
                    break;
                case MPD_IDLE_QUEUE: {
                    //queue has changed
                    unsigned old_queue_version = partition_state->queue_version;
                    buffer = mympd_api_queue_status(partition_state, buffer);
                    if (partition_state->queue_version == old_queue_version) {
                        //ignore this idle event, queue version has not changed in this partition
                        sdsclear(buffer);
                        MYMPD_LOG_DEBUG("Queue version has not changed, ignoring idle event MPD_IDLE_QUEUE");
                        break;
                    }
                    //jukebox enabled
                    if (partition_state->jukebox_mode != JUKEBOX_OFF &&
                        partition_state->queue_length < partition_state->jukebox_queue_length)
                    {
                        MYMPD_LOG_DEBUG("Jukebox mode: %u", partition_state->jukebox_mode);
                        mpd_client_jukebox(partition_state);
                    }
                    //autoPlay enabled
                    if (partition_state->auto_play == true &&
                        partition_state->queue_length > 0)
                    {
                        if (partition_state->play_state != MPD_STATE_PLAY) {
                            MYMPD_LOG_INFO("AutoPlay enabled, start playing");
                            if (mpd_run_play(partition_state->conn) == false) {
                                mympd_check_error_and_recover(partition_state);
                            }
                        }
                        else {
                            MYMPD_LOG_DEBUG("Autoplay enabled, already playing");
                        }
                    }
                    break;
                }
                case MPD_IDLE_PLAYER:
                    //player status has changed
                    //get and put mpd state
                    buffer = mympd_api_status_get(partition_state, buffer, REQUEST_ID_NOTIFY);
                    //check if song has changed
                    if (partition_state->song_id != partition_state->last_song_id &&
                        partition_state->last_skipped_id != partition_state->last_song_id &&
                        partition_state->last_song_uri != NULL)
                    {
                        time_t now = time(NULL);
                        if (partition_state->mpd_shared_state->feat_mpd_stickers == true &&          //stickers enabled
                            partition_state->last_song_set_song_played_time > now) //time in the future
                        {
                            //last song skipped
                            time_t elapsed = now - partition_state->last_song_start_time;
                            if (elapsed > 10 &&
                                partition_state->last_song_start_time > 0 &&
                                sdslen(partition_state->last_song_uri) > 0)
                            {
                                MYMPD_LOG_DEBUG("Song \"%s\" skipped", partition_state->last_song_uri);
                                if (partition_state->mpd_shared_state->feat_mpd_stickers == true) {
                                    sticker_inc_skip_count(&partition_state->mpd_shared_state->sticker_queue, partition_state->last_song_uri);
                                    sticker_set_last_skipped(&partition_state->mpd_shared_state->sticker_queue, partition_state->last_song_uri);
                                }
                                partition_state->last_skipped_id = partition_state->last_song_id;
                            }
                        }
                    }
                    break;
                case MPD_IDLE_MIXER:
                    //volume has changed
                    buffer = mympd_api_status_volume_get(partition_state, buffer, REQUEST_ID_NOTIFY);
                    break;
                case MPD_IDLE_OUTPUT:
                    //outputs are changed
                    buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_OUTPUTS);
                    break;
                case MPD_IDLE_OPTIONS:
                    //mpd playback options are changed
                    mympd_api_queue_status(partition_state, NULL);
                    buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_OPTIONS);
                    break;
                case MPD_IDLE_UPDATE:
                    //database update has started or is finished
                    buffer = mympd_api_status_updatedb_state(partition_state, buffer);
                    break;
                default: {
                    //other idle events not used
                }
            }
            //check for attached triggers
            mympd_api_trigger_execute(trigger_list, (enum trigger_events)idle_event);
            //broadcast event to all websockets
            if (sdslen(buffer) > 0) {
                ws_notify(buffer);
                sdsclear(buffer);
            }
        }
    }
    FREE_SDS(buffer);
}

/**
 * Checks if we should create the caches and adds a one-shot timer
 * We do not create the caches instantly to debounce MPD_IDLE_DATABASE events
 * @param mympd_state pointer to the mympd_state struct
 * @param timeout seconds after the timer triggers
 * @return true on success else false
 */
static bool update_mympd_caches(struct t_mpd_shared_state *mpd_shared_state, struct t_timer_list *timer_list, time_t timeout) {
    if (mpd_shared_state->feat_mpd_stickers == false &&
        mpd_shared_state->feat_mpd_tags == false)
    {
        MYMPD_LOG_DEBUG("Caches are disabled");
        return true;
    }
    MYMPD_LOG_DEBUG("Adding timer to update the caches");
    return mympd_api_timer_replace(timer_list, timeout, TIMER_ONE_SHOT_REMOVE, timer_handler_by_id, TIMER_ID_CACHES_CREATE, NULL);
}
