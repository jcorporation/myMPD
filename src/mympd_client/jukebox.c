/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Jukebox functions
 */

#include "compile_time.h"
#include "src/mympd_client/jukebox.h"

#include "dist/sds/sds.h"
#include "src/lib/cache/cache_rax_album.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/lib/timer.h"
#include "src/mympd_api/trigger.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/queue.h"
#include "src/mympd_client/shortcuts.h"
#include "src/mympd_client/tags.h"

#include <stdbool.h>
#include <string.h>

//private definitions
static void jukebox_get_last_played_add(struct t_partition_state *partition_state,
        struct mpd_song *song, struct t_list *queue_list, enum jukebox_modes jukebox_mode);
static struct t_list *jukebox_get_last_played(struct t_partition_state *partition_state,
        enum jukebox_modes jukebox_mode);

/**
 * Public functions
 */

/**
 * Parses the string to the jukebox mode
 * @param str string to parse
 * @return jukebox mode
 */
enum jukebox_modes jukebox_mode_parse(const char *str) {
    if (strcmp(str, "off") == 0) {
        return JUKEBOX_OFF;
    }
    if (strcmp(str, "song") == 0) {
        return JUKEBOX_ADD_SONG;
    }
    if (strcmp(str, "album") == 0) {
        return JUKEBOX_ADD_ALBUM;
    }
    if (strcmp(str, "script") == 0) {
        return JUKEBOX_SCRIPT;
    }
    return JUKEBOX_UNKNOWN;
}

/**
 * Returns the jukebox mode as string
 * @param mode the jukebox mode
 * @return jukebox mode as string
 */
const char *jukebox_mode_lookup(enum jukebox_modes mode) {
    switch (mode) {
        case JUKEBOX_OFF:
            return "off";
        case JUKEBOX_ADD_SONG:
            return "song";
        case JUKEBOX_ADD_ALBUM:
            return "album";
        case JUKEBOX_SCRIPT:
            return "script";
        case JUKEBOX_UNKNOWN:
            return NULL;
    }
    return NULL;
}

/**
 * Clears the jukebox queue of all partitions.
 * @param mympd_state pointer to central myMPD state.
 */
void jukebox_clear_all(struct t_mympd_state *mympd_state) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        list_clear(partition_state->jukebox.queue);
        //notify clients
        send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_state->name);
        //next entry
        partition_state = partition_state->next;
    }
}

/**
 * Disables the jukebox timer
 * @param partition_state pointer to partition state
 */
void jukebox_disable(struct t_partition_state *partition_state) {
    MYMPD_LOG_DEBUG(partition_state->name, "Disabling jukebox timer");
    mympd_timer_set(partition_state->timer_fd_jukebox, 0, 0);
}

/**
 * The real jukebox function.
 * It determines if a song must be added or not and starts playing.
 * @param mympd_state Pointer to myMPD state
 * @param partition_state pointer to myMPD partition state
 * @param album_cache pointer to album cache
 * @return true on success, else false
 */
bool jukebox_run(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state, struct t_cache *album_cache) {
    if (partition_state->jukebox.filling == true) {
        MYMPD_LOG_DEBUG(partition_state->name, "Filling the jukebox queue is already in progress");
        return true;
    }
    if (partition_state->jukebox.mode == JUKEBOX_OFF) {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox is disabled");
        return true;
    }

    mympd_client_queue_status_update(partition_state);
    sdsclear(partition_state->jukebox.last_error);

    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: MPD queue length: %u", partition_state->queue_length);
    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: min queue length: %u", partition_state->jukebox.queue_length);

    if (partition_state->queue_length > partition_state->jukebox.queue_length) {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: MPD queue length > %u", partition_state->jukebox.queue_length);
        return true;
    }

    unsigned add_songs = partition_state->jukebox.queue_length > partition_state->queue_length
        ? partition_state->jukebox.queue_length - partition_state->queue_length
        : 1;

    if (add_songs > JUKEBOX_ADD_SONG_MAX) {
        MYMPD_LOG_WARN(partition_state->name, "Jukebox: max songs to add set to %u, adding max. %d songs", add_songs, JUKEBOX_ADD_SONG_MAX);
        add_songs = JUKEBOX_ADD_SONG_MAX;
    }

    // check if jukebox queue is long enough
    if (add_songs > partition_state->jukebox.queue->length) {
        if (partition_state->jukebox.mode == JUKEBOX_SCRIPT) {
            MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: Trigger");
            struct t_list arguments;
            list_init(&arguments);
            list_push(&arguments, "addToQueue", 0, "1", NULL);
            int n = mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_JUKEBOX,
                    partition_state->name, &arguments);
            list_clear(&arguments);
            if (n > 0) {
                if (n > 1) {
                    MYMPD_LOG_WARN(partition_state->name, "More than one script triggered for jukebox.");
                }
                partition_state->jukebox.filling = true;
            }
            return n == 1;
        }
        // start mpd worker thread
        partition_state->jukebox.filling = true;
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: Starting worker thread to fill the jukebox queue");
        struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_JUKEBOX_REFILL_ADD, NULL, partition_state->name);
        request->data = tojson_uint(request->data, "addSongs", add_songs, false);
        request->data = jsonrpc_end(request->data);

        struct t_list *queue_list = jukebox_get_last_played(partition_state, partition_state->jukebox.mode);
        request->extra = queue_list;
        return mympd_queue_push(mympd_api_queue, request, 0);
    }
    
    // add from jukebox queue to mpd queue
    sds error = sdsempty();
    bool rc = jukebox_add_to_queue(partition_state, album_cache, add_songs, &error);
    if (rc == false) {
        partition_state->jukebox.last_error = sds_replace(partition_state->jukebox.last_error, error);
        send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_ERROR, partition_state->name, error);
        FREE_SDS(error);
        return false;
    }
    FREE_SDS(error);

    if ((partition_state->jukebox.mode == JUKEBOX_ADD_SONG && partition_state->jukebox.queue->length < JUKEBOX_INTERNAL_SONG_QUEUE_LENGTH_MIN) ||
        (partition_state->jukebox.mode == JUKEBOX_ADD_ALBUM && partition_state->jukebox.queue->length < JUKEBOX_INTERNAL_ALBUM_QUEUE_LENGTH_MIN))
    {
        // start mpd worker thread
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: Starting worker thread to fill the jukebox queue");
        partition_state->jukebox.filling = true;
        struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_JUKEBOX_REFILL, NULL, partition_state->name);
        request->data = sdscatlen(request->data, "}}", 2);
        struct t_list *queue_list = jukebox_get_last_played(partition_state, partition_state->jukebox.mode);
        request->extra = queue_list;
        return mympd_queue_push(mympd_api_queue, request, 0);
    }
    if (partition_state->jukebox.mode == JUKEBOX_SCRIPT && partition_state->jukebox.queue->length < JUKEBOX_INTERNAL_SONG_QUEUE_LENGTH_MIN) {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: Trigger");
        struct t_list arguments;
        list_init(&arguments);
        list_push(&arguments, "addToQueue", 0, "0", NULL);
        int n = mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_JUKEBOX,
                partition_state->name, &arguments);
        list_clear(&arguments);
        if (n > 0) {
            if (n > 1) {
                MYMPD_LOG_WARN(partition_state->name, "More than one script triggered for jukebox.");
            }
            partition_state->jukebox.filling = true;
        }
        return n == 1;
    }
    return rc;
}

/**
 * Adds songs or albums from the jukebox queue to the MPD queue and starts playing.
 * @param partition_state pointer to myMPD partition state
 * @param album_cache pointer to album cache
 * @param add_songs number of songs to add
 * @param error pointer to allocated sds for error message
 * @return true on success, else false
 */
bool jukebox_add_to_queue(struct t_partition_state *partition_state,
        struct t_cache *album_cache, unsigned add_songs, sds *error)
{
    unsigned added = 0;
    struct t_list_node *current;
    while (added < add_songs &&
           (current = list_shift_first(partition_state->jukebox.queue)) != NULL)
    {
        if (partition_state->jukebox.mode == JUKEBOX_ADD_SONG ||
            partition_state->jukebox.mode == JUKEBOX_SCRIPT)
        {
            mpd_run_add(partition_state->conn, current->key);
            if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_add") == true) {
                MYMPD_LOG_NOTICE(partition_state->name, "Jukebox adding song: %s", current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR(partition_state->name, "Jukebox adding song %s failed", current->key);
            }
        }
        else if (partition_state->jukebox.mode == JUKEBOX_ADD_ALBUM) {
            bool rc = mympd_client_add_album_to_queue(partition_state, album_cache, current->key, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
            if (rc == true) {
                MYMPD_LOG_NOTICE(partition_state->name, "Jukebox adding album: %s - %s", current->value_p, current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR(partition_state->name, "Jukebox adding album %s - %s failed", current->value_p, current->key);
            }
        }
        else {
            // This should not appear
            MYMPD_LOG_WARN(partition_state->name, "Jukebox is disabled");
        }
        list_node_free(current);
    }

    //notify clients
    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_state->name);

    if (added == 0) {
        MYMPD_LOG_ERROR(partition_state->name, "Error adding song(s)");
        send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_ERROR, partition_state->name, "Adding songs from jukebox to queue failed");
        if (sdslen(*error) == 0) {
            *error = sdscat(*error, "Adding songs from jukebox to queue failed");
        }
        return false;
    }

    // start playback
    mympd_client_queue_status_update(partition_state);
    if (partition_state->play_state != MPD_STATE_PLAY) {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: start playback");
        mpd_run_play(partition_state->conn);
        if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_play") == false) {
            send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_ERROR, partition_state->name, "Start playing failed");
            *error = sdscat(*error, "Start playing failed");
            return false;
        }
    }
    return true;
}

/**
 * Private functions
 */

/**
 * Adds an album or song to the jukebox last played list
 * @param partition_state pointer to myMPD partition state
 * @param song mpd_song struct to append
 * @param queue_list list to append the entry
 * @param jukebox_mode the jukebox mode
 */
static void jukebox_get_last_played_add(struct t_partition_state *partition_state,
    struct mpd_song *song, struct t_list *queue_list, enum jukebox_modes jukebox_mode)
{
    if (song == NULL) {
        return;
    }
    sds tag_value = sdsempty();
    if (partition_state->jukebox.uniq_tag.tags[0] != MPD_TAG_TITLE) {
        tag_value = mympd_client_get_tag_value_string(song, partition_state->jukebox.uniq_tag.tags[0], tag_value);
    }
    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        list_push(queue_list, mpd_song_get_uri(song), 0, tag_value, NULL);
    }
    else {
        // JUKEBOX_ADD_ALBUM
        sds albumid = album_cache_get_key_from_song(sdsempty(), song, &partition_state->config->albums);
        list_push(queue_list, albumid, 0, tag_value, NULL);
        FREE_SDS(albumid);
    }
    FREE_SDS(tag_value);
    mpd_song_free(song);
}

/**
 * Gets the songs or albums from queue and last played.
 * This list is used to enforce the uniq tag constraint
 * @param partition_state pointer to myMPD partition state
 * @param jukebox_mode the jukebox mode
 * @return a newly allocated list with songs or albums
 */
static struct t_list *jukebox_get_last_played(struct t_partition_state *partition_state, enum jukebox_modes jukebox_mode) {
    struct mpd_song *song;
    struct t_list *queue_list = list_new();

    // get current queue
    if (mpd_send_list_queue_meta(partition_state->conn)) {
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            jukebox_get_last_played_add(partition_state, song, queue_list, jukebox_mode);
        }
    }
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_queue_meta") == false) {
        FREE_PTR(queue_list);
        return NULL;
    }

    // append last_played to queue list
    int added = 0;
    if (queue_list->length < JUKEBOX_UNIQ_RANGE) {
        struct t_list_node *current = partition_state->last_played.head;
        while (added < JUKEBOX_UNIQ_RANGE &&
               current != NULL)
        {
            if (mpd_send_list_meta(partition_state->conn, current->key)) {
                if ((song = mpd_recv_song(partition_state->conn)) != NULL) {
                    jukebox_get_last_played_add(partition_state, song, queue_list, jukebox_mode);
                    added++;
                }
                else {
                    MYMPD_LOG_WARN(partition_state->name, "Failure fetching song information for uri \"%s\"", current->key);
                }
            }
            mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_meta");
            current = current->next;
        }
    }

    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox last_played list length: %u", queue_list->length);
    return queue_list;
}
