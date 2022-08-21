/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "idle.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mympd_state.h"
#include "../lib/sds_extras.h"
#include "../lib/sticker_cache.h"
#include "../lib/utility.h"
#include "../mpd_worker/mpd_worker.h"
#include "../mympd_api/mympd_api_handler.h"
#include "../mympd_api/last_played.h"
#include "../mympd_api/queue.h"
#include "../mympd_api/settings.h"
#include "../mympd_api/status.h"
#include "../mympd_api/timer.h"
#include "../mympd_api/timer_handlers.h"
#include "../mympd_api/trigger.h"
#include "connection.h"
#include "errorhandler.h"
#include "features.h"
#include "jukebox.h"
#include "tags.h"

#include <mpd/idle.h>
#include <mpd/partition.h>
#include <poll.h>
#include <string.h>

/**
 * Private definitions
 */

static bool update_mympd_caches(struct t_mpd_state *mpd_state,
        struct t_timer_list *timer_list, time_t timeout);
static void mpd_client_idle_partition(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        bool mpd_idle_event_waiting, struct t_work_request *request);
static void mpd_client_parse_idle(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state, unsigned idle_bitmask);

static bool partitions_populate(struct t_mympd_state *mympd_state);
static bool partitions_check(struct t_mympd_state *mympd_state, const char *name);
static void partitions_add(struct t_mympd_state *mympd_state, const char *name);
static void partitions_get_fds(struct t_mympd_state *mympd_state);

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
            MYMPD_LOG_ERROR("Error polling mpd connection");
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
            mpd_client_idle_partition(mympd_state, partition_state, mpd_idle_event_waiting, request);
            request = NULL;
        }
        else {
            mpd_client_idle_partition(mympd_state, partition_state, mpd_idle_event_waiting, NULL);
        }
    } while ((partition_state = partition_state->next) != NULL);
    //cleanup
    if (request != NULL) {
        //request was for unknown partition, discard it
        MYMPD_LOG_WARN("Discarding request for unknown partition \"%s\"", request->partition);
        free_request(request);
    }
}

/**
 * Private functions
 */

/**
 * This function handles api requests and mpd events per partition.
 * @param mympd_state pointer to the mympd state struct
 * @param partition_state pointer to the partition state
 * @param mpd_idle_event_waiting true if mpd idle event is waiting, else false
 * @param request api request
 */
static void mpd_client_idle_partition(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        bool mpd_idle_event_waiting, struct t_work_request *request)
{
    switch (partition_state->conn_state) {
        case MPD_WAIT: {
            time_t now = time(NULL);
            if (now > partition_state->reconnect_time) {
                //wait time elapsed, try to reconnect
                partition_state->conn_state = MPD_DISCONNECTED;
                break;
            }
            //process mympd_api queue request
            if (request != NULL) {
                MYMPD_LOG_DEBUG("Handle request (mpd disconnected)");
                if (is_mympd_only_api_method(request->cmd_id) == true) {
                    //request that are handled without a mpd connection
                    mympd_api_handler(mympd_state, partition_state, request);
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
            break;
        }
        case MPD_DISCONNECTED:
            //try to connect
            MYMPD_LOG_INFO("Creating mpd connection for partition \"%s\"", partition_state->name);
            if (mpd_client_connect(partition_state) == false) {
                break;
            }
            if (partition_state->is_default == true) {
                //check version
                if (mpd_connection_cmp_server_version(partition_state->conn, 0, 21, 0) < 0) {
                    MYMPD_LOG_EMERG("MPD version too old, myMPD supports only MPD version >= 0.21");
                    s_signal_received = 1;
                    break;
                }
            }
            //we are connected
            if (partition_state->is_default == false) {
                //change partition
                MYMPD_LOG_INFO("Switching to partition \"%s\"", partition_state->name);
                bool rc = mpd_run_switch_partition(partition_state->conn, partition_state->name);
                if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_run_switch_partition") == false) {
                    MYMPD_LOG_ERROR("Could not switch to partition \"%s\"", partition_state->name);
                    mpd_client_disconnect(partition_state);
                    break;
                }
            }
            send_jsonrpc_event(JSONRPC_EVENT_MPD_CONNECTED, partition_state->name);
            if (partition_state->is_default == true) {
                //get mpd features
                mpd_client_mpd_features(mympd_state);
                //initiate cache updates
                update_mympd_caches(mympd_state->mpd_state, &mympd_state->timer_list, 2);
                //set timer for smart playlist update
                mympd_api_timer_replace(&mympd_state->timer_list, 30, (int)mympd_state->smartpls_interval,
                    timer_handler_by_id, TIMER_ID_SMARTPLS_UPDATE, NULL);
                //populate the partition list
                partitions_populate(mympd_state);
            }
            //jukebox
            if (partition_state->jukebox_mode != JUKEBOX_OFF) {
                jukebox_run(partition_state);
            }
            if (mpd_send_idle(partition_state->conn) == false) {
                MYMPD_LOG_ERROR("\"%s\": Entering idle mode failed", partition_state->name);
                partition_state->conn_state = MPD_FAILURE;
            }
            mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_CONNECTED, partition_state->name);
            break;
        case MPD_FAILURE:
            MYMPD_LOG_ERROR("\"%s\": MPD connection failed", partition_state->name);
            // fall through
        case MPD_DISCONNECT:
        case MPD_DISCONNECT_INSTANT:
            send_jsonrpc_event(JSONRPC_EVENT_MPD_DISCONNECTED, partition_state->name);
            mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_DISCONNECTED, partition_state->name);
            mpd_client_disconnect(partition_state);
            //set wait time for next connection attempt
            if (partition_state->conn_state != MPD_DISCONNECT_INSTANT) {
                partition_state->conn_state = MPD_WAIT;
                if (partition_state->reconnect_interval < 20) {
                    partition_state->reconnect_interval += 2;
                }
                partition_state->reconnect_time = time(NULL) + partition_state->reconnect_interval;
                MYMPD_LOG_INFO("\"%s\": Waiting %lld seconds before reconnection", partition_state->name, (long long)partition_state->reconnect_interval);
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
            //handle jukebox and last played only in mpd play state
            if (partition_state->play_state == MPD_STATE_PLAY) {
                time_t now = time(NULL);
                //check if we should set the played state of current song
                if (now > partition_state->set_song_played_time &&
                    partition_state->set_song_played_time > 0 &&
                    partition_state->last_last_played_id != partition_state->song_id)
                {
                    MYMPD_LOG_DEBUG("\"%s\": Song has played half: %lld", partition_state->name, (long long)partition_state->set_song_played_time);
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
                        MYMPD_LOG_DEBUG("\"%s\": Jukebox should add song", partition_state->name);
                        jukebox_add_song = true;
                    }
                }
            }
            //check if we need to exit the idle mode
            if (mpd_idle_event_waiting == true ||                        //idle event waiting
                request != NULL ||                                       //api was called
                jukebox_add_song == true ||                              //jukebox trigger
                set_played == true ||                                    //playstate of song must be set
                mympd_state->mpd_state->sticker_queue.length > 0)        //we must set waiting stickers
            {
                MYMPD_LOG_DEBUG("\"%s\": Leaving mpd idle mode", partition_state->name);
                if (mpd_send_noidle(partition_state->conn) == false) {
                    mympd_check_error_and_recover(partition_state);
                    partition_state->conn_state = MPD_FAILURE;
                    break;
                }
                if (mpd_idle_event_waiting == true) {
                    //Handle idle events
                    MYMPD_LOG_DEBUG("\"%s\": Checking for idle events", partition_state->name);
                    enum mpd_idle idle_bitmask = mpd_recv_idle(partition_state->conn, false);
                    mpd_client_parse_idle(mympd_state, partition_state, idle_bitmask);
                }
                else {
                    mpd_response_finish(partition_state->conn);
                }
                //set song played state
                if (set_played == true) {
                    partition_state->last_last_played_id = partition_state->song_id;

                    if (mympd_state->mpd_state->last_played_count > 0) {
                        mympd_api_last_played_add_song(partition_state, partition_state->song_id);
                    }
                    if (mympd_state->mpd_state->feat_stickers == true) {
                        sticker_inc_play_count(&mympd_state->mpd_state->sticker_queue,
                            partition_state->song_uri);
                        sticker_set_last_played(&mympd_state->mpd_state->sticker_queue,
                            partition_state->song_uri, partition_state->last_song_start_time);
                    }
                    mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_SCROBBLE, partition_state->name);
                }
                //trigger jukebox
                if (jukebox_add_song == true) {
                    jukebox_run(partition_state);
                }
                //an api request is there
                if (request != NULL) {
                    //Handle request
                    MYMPD_LOG_DEBUG("Handle API request");
                    mympd_api_handler(mympd_state, partition_state, request);
                }
                if (partition_state->is_default == true) {
                    //process sticker queue
                    if (mympd_state->mpd_state->feat_stickers == true &&
                        mympd_state->mpd_state->sticker_queue.length > 0)
                    {
                        MYMPD_LOG_DEBUG("Processing sticker queue");
                        sticker_dequeue(&mympd_state->mpd_state->sticker_queue,
                            &mympd_state->mpd_state->sticker_cache, partition_state);
                    }
                }
                //reenter idle mode
                MYMPD_LOG_DEBUG("\"%s\": Entering mpd idle mode", partition_state->name);
                if (mpd_send_idle_mask(partition_state->conn, partition_state->idle_mask) == false) {
                    mympd_check_error_and_recover(partition_state);
                    partition_state->conn_state = MPD_FAILURE;
                }
            }
            break;
        }
        case MPD_REMOVED:
            MYMPD_LOG_DEBUG("\"%s\": removed", partition_state->name);
            break;
        default:
            MYMPD_LOG_ERROR("Invalid mpd connection state");
    }
}

/**
 * Handles mpd idle events
 * @param mympd_state pointer to t_mympd_state struct
 * @param partition_state pointer to partition specific states
 * @param idle_bitmask triggered mpd idle events as bitmask
 */
static void mpd_client_parse_idle(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state, unsigned idle_bitmask) {
    sds buffer = sdsempty();
    for (unsigned j = 0;; j++) {
        enum mpd_idle idle_event = 1 << j;
        const char *idle_name = mpd_idle_name(idle_event);
        if (idle_name == NULL) {
            break;
        }
        if (idle_bitmask & idle_event) {
            MYMPD_LOG_INFO("\"%s\": MPD idle event \"%s\"", partition_state->name, idle_name);
            switch(idle_event) {
                case MPD_IDLE_DATABASE:
                    //database has changed - global event
                    MYMPD_LOG_INFO("MPD database has changed");
                    buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_DATABASE);
                    //add timer for cache updates
                    update_mympd_caches(partition_state->mpd_state, &mympd_state->timer_list, 10);
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
                    unsigned old_queue_version = partition_state->queue_version;
                    buffer = mympd_api_queue_status(partition_state, buffer);
                    if (partition_state->queue_version == old_queue_version) {
                        //ignore this idle event, queue version has not changed in this partition
                        sdsclear(buffer);
                        MYMPD_LOG_DEBUG("\"%s\": Queue version has not changed, ignoring idle event MPD_IDLE_QUEUE", partition_state->name);
                        break;
                    }
                    //jukebox enabled
                    if (partition_state->jukebox_mode != JUKEBOX_OFF &&
                        partition_state->queue_length < partition_state->jukebox_queue_length)
                    {
                        MYMPD_LOG_DEBUG("\"%s\": Jukebox mode: %s", partition_state->name, jukebox_mode_lookup(partition_state->jukebox_mode));
                        jukebox_run(partition_state);
                    }
                    //autoPlay enabled
                    if (partition_state->auto_play == true &&
                        partition_state->queue_length > 0)
                    {
                        if (partition_state->play_state != MPD_STATE_PLAY) {
                            MYMPD_LOG_INFO("\"%s\": AutoPlay enabled, start playing", partition_state->name);
                            if (mpd_run_play(partition_state->conn) == false) {
                                mympd_check_error_and_recover(partition_state);
                            }
                        }
                        else {
                            MYMPD_LOG_DEBUG("\"%s\": Autoplay enabled, already playing", partition_state->name);
                        }
                    }
                    break;
                }
                case MPD_IDLE_PLAYER:
                    //player status has changed - partition specific event
                    //get and put mpd state
                    buffer = mympd_api_status_get(partition_state, buffer, REQUEST_ID_NOTIFY);
                    //check if song has changed
                    if (partition_state->song_id != partition_state->last_song_id &&
                        partition_state->last_skipped_id != partition_state->last_song_id &&
                        partition_state->last_song_uri != NULL)
                    {
                        time_t now = time(NULL);
                        if (partition_state->mpd_state->feat_stickers == true &&          //stickers enabled
                            partition_state->last_song_set_song_played_time > now) //time in the future
                        {
                            //last song skipped
                            time_t elapsed = now - partition_state->last_song_start_time;
                            if (elapsed > 10 &&
                                partition_state->last_song_start_time > 0 &&
                                sdslen(partition_state->last_song_uri) > 0)
                            {
                                MYMPD_LOG_DEBUG("\"%s\": Song \"%s\" skipped", partition_state->name, partition_state->last_song_uri);
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
                    buffer = mympd_api_status_volume_get(partition_state, buffer, REQUEST_ID_NOTIFY);
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
            mympd_api_trigger_execute(&mympd_state->trigger_list, (enum trigger_events)idle_event, partition_state->name);
            //broadcast event to all websockets
            if (sdslen(buffer) > 0) {
                switch(idle_event) {
                    case MPD_IDLE_DATABASE:
                    case MPD_IDLE_PARTITION:
                    case MPD_IDLE_STORED_PLAYLIST:
                    case MPD_IDLE_UPDATE:
                        ws_notify(buffer, MPD_PARTITION_ALL);
                        break;
                    default:
                        ws_notify(buffer, partition_state->name);
                }
                sdsclear(buffer);
            }
        }
    }
    FREE_SDS(buffer);
}

/**
 * Checks if we should create the caches and adds a one-shot timer
 * We do not create the caches instantly to debounce MPD_IDLE_DATABASE events
 * @param mpd_state pointer to the mympd_state struct
 * @param timer_list the timer list
 * @param timeout seconds after the timer triggers
 * @return true on success else false
 */
static bool update_mympd_caches(struct t_mpd_state *mpd_state,
        struct t_timer_list *timer_list, time_t timeout)
{
    if (mpd_state->feat_stickers == false &&
        mpd_state->feat_tags == false)
    {
        MYMPD_LOG_DEBUG("Caches are disabled");
        return true;
    }
    MYMPD_LOG_DEBUG("Adding timer to update the caches");
    return mympd_api_timer_replace(timer_list, timeout, TIMER_ONE_SHOT_REMOVE,
            timer_handler_by_id, TIMER_ID_CACHES_CREATE, NULL);
}

/**
 * Populates the partition list: removes and adds partitions according to mpd
 * @param mympd_state pointer to t_mympd_state struct
 * @return true on success, else false
 */
static bool partitions_populate(struct t_mympd_state *mympd_state) {
    //first add all missing partitions to the list
    bool rc = mpd_send_listpartitions(mympd_state->partition_state->conn);
    if (mympd_check_rc_error_and_recover(mympd_state->partition_state, rc, "mpd_send_listpartitions") == false) {
        return false;
    }
    struct mpd_pair *partition;
    struct t_list mpd_partitions;
    list_init(&mpd_partitions);
    while ((partition = mpd_recv_partition_pair(mympd_state->partition_state->conn)) != NULL) {
        const char *name = partition->value;
        if (partitions_check(mympd_state, name) == false) {
            MYMPD_LOG_INFO("Adding partition \"%s\" to the partition list", name);
            partitions_add(mympd_state, name);
        }
        list_push(&mpd_partitions, name, 0, NULL, NULL);
        mpd_return_pair(mympd_state->partition_state->conn, partition);
    }
    mpd_response_finish(mympd_state->partition_state->conn);
    if (mympd_check_error_and_recover(mympd_state->partition_state) == false) {
        list_clear(&mpd_partitions);
        return false;
    }
    //remove obsolet partitions
    //skip default partition (first entry)
    struct t_partition_state *current = mympd_state->partition_state->next;
    struct t_partition_state *previous = mympd_state->partition_state;
    for (; current != NULL; previous = current, current = current->next) {
        if (list_get_node(&mpd_partitions, current->name) == NULL) {
            MYMPD_LOG_INFO("Removing partition \"%s\" from the partition list", current->name);
            struct t_partition_state *next = current->next;
            //free partition state
            partition_state_free(current);
            //partition was removed from mpd
            previous->next = next;
            //go back to previous node
            current = previous;
        }
    }
    list_clear(&mpd_partitions);
    return true;
}

/**
 * Checks if the partition is already in the list
 * @param mympd_state pointer to t_mympd_state struct
 * @param name  partition name
 * @return true if partition is in the list, else false
 */
static bool partitions_check(struct t_mympd_state *mympd_state, const char *name) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        if (strcmp(partition_state->name, name) == 0) {
            return true;
        }
        partition_state = partition_state->next;
    }
    return false;
}

/**
 * Adds a partition to the partition list
 * @param mympd_state pointer to t_mympd_state struct
 * @param name partition name
 */
static void partitions_add(struct t_mympd_state *mympd_state, const char *name) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    //goto end
    while (partition_state->next != NULL) {
        partition_state = partition_state->next;
    }
    //append new partition struct and set defaults
    //connection will be established in next idle loop run
    partition_state->next = malloc_assert(sizeof(struct t_partition_state));
    //set default partition state
    partition_state_default(partition_state->next, name, mympd_state);
    //read partition specific state from disc
    mympd_api_settings_statefiles_partition_read(partition_state->next);
}

/**
 * Populates the mpd connection fds
 * @param mympd_state pointer to t_mympd_state struct
 */
static void partitions_get_fds(struct t_mympd_state *mympd_state) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    mympd_state->nfds = 0;
    while (partition_state != NULL) {
        if (mympd_state->nfds == MPD_CONNECTION_MAX) {
            MYMPD_LOG_ERROR("Too many partitions");
            break;
        }
        if (partition_state->conn != NULL &&
            partition_state->conn_state == MPD_CONNECTED)
        {
            mympd_state->fds[mympd_state->nfds].fd = mpd_connection_get_fd(partition_state->conn);
            mympd_state->fds[mympd_state->nfds].events = POLLIN;
            mympd_state->nfds++;
        }
        partition_state = partition_state->next;
    }
}
