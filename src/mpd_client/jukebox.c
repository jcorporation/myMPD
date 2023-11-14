/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/jukebox.h"

#include "dist/sds/sds.h"
#include "src/lib/album_cache.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/queue.h"
#include "src/mpd_client/random_select.h"
#include "src/mpd_client/shortcuts.h"
#include "src/mpd_client/tags.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

//private definitions
static bool jukebox(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb, struct t_cache *album_cache);
static void jukebox_get_last_played_add(struct t_partition_state *partition_state,
        struct mpd_song *song, struct t_list *queue_list, enum jukebox_modes jukebox_mode);
static struct t_list *jukebox_get_last_played(struct t_partition_state *partition_state,
        enum jukebox_modes jukebox_mode);
static bool jukebox_add_to_queue(struct t_partition_state *partition_state,
        struct t_stickerdb_state *stickerdb, struct t_cache *album_cache, long add_songs,
        enum jukebox_modes jukebox_mode, const char *playlist);
static bool jukebox_run_fill_jukebox_queue(struct t_partition_state *partition_state,
        struct t_stickerdb_state *stickerdb, struct t_cache *album_cache,
        enum jukebox_modes jukebox_mode, const char *playlist);
static bool jukebox_fill_jukebox_queue(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        struct t_cache *album_cache, enum jukebox_modes jukebox_mode, const char *playlist);

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
        list_clear(&partition_state->jukebox_queue);
        //notify clients
        send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_state->name);
        //next entry
        partition_state = partition_state->next;
    }
}

/**
 * Wrapper for the real jukebox function that retries adding songs or albums three times.
 * @param partition_state pointer to myMPD partition state
 * @return true on success, else false
 */
bool jukebox_run(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb, struct t_cache *album_cache) {
    for (int i = 1; i < 3; i++) {
         if (jukebox(partition_state, stickerdb, album_cache) == true) {
             return true;
         }
         if (partition_state->jukebox_mode == JUKEBOX_OFF) {
             return false;
         }
         MYMPD_LOG_ERROR(partition_state->name, "Jukebox: trying again, attempt %d", i);
    }
    return false;
}

/**
 * The real jukebox function.
 * It determines if a song must be added or not and starts playing.
 * @param partition_state pointer to myMPD partition state
 * @return true on success, else false
 */
static bool jukebox(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb, struct t_cache *album_cache) {
    long queue_length = 0;
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status != NULL) {
        queue_length = (long)mpd_status_get_queue_length(status);
        mpd_status_free(status);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_status") == false) {
        return false;
    }

    time_t now = time(NULL);
    time_t add_time = partition_state->song_end_time - (partition_state->crossfade + 10);

    MYMPD_LOG_DEBUG(partition_state->name, "Queue length: %ld", queue_length);
    MYMPD_LOG_DEBUG(partition_state->name, "Min queue length: %ld", partition_state->jukebox_queue_length);

    if (queue_length >= partition_state->jukebox_queue_length && now < add_time) {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: Queue length >= %ld and add_time not reached", partition_state->jukebox_queue_length);
        return true;
    }

    //add song if add_time is reached or queue is empty
    long add_songs = partition_state->jukebox_queue_length > queue_length
        ? partition_state->jukebox_queue_length - queue_length
        : 0;

    if (now > add_time &&
        add_time > 0 &&
        queue_length <= partition_state->jukebox_queue_length)
    {
        if (add_songs == 0) {
            add_songs++;
        }
        MYMPD_LOG_DEBUG(partition_state->name, "Time now %lld greater than add_time %lld, adding %ld song(s)", (long long)now, (long long)add_time, add_songs);
    }

    if (add_songs < 1) {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: nothing to do");
        return true;
    }

    if (add_songs > JUKEBOX_ADD_SONG_MAX) {
        MYMPD_LOG_WARN(partition_state->name, "Jukebox: max songs to add set to %ld, adding max. 99 songs", add_songs);
        add_songs = JUKEBOX_ADD_SONG_MAX;
    }

    if (partition_state->mpd_state->feat.playlists == false && strcmp(partition_state->jukebox_playlist, "Database") != 0) {
        MYMPD_LOG_WARN(partition_state->name, "Jukebox: Playlists are disabled");
        return true;
    }

    bool rc = jukebox_add_to_queue(partition_state, stickerdb, album_cache, add_songs, partition_state->jukebox_mode, partition_state->jukebox_playlist);

    //update playback state
    mpd_client_queue_status_update(partition_state);
    if (partition_state->play_state != MPD_STATE_PLAY) {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: start playback");
        mpd_run_play(partition_state->conn);
        mympd_check_error_and_recover(partition_state, NULL, "mpd_run_play");
    }

    if (rc == true) {
        //notify clients
        send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_state->name);
        return true;
    }

    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox mode: %d", partition_state->jukebox_mode);
    MYMPD_LOG_ERROR(partition_state->name, "Jukebox: Error adding song(s)");
    return false;
}

/**
 * Private functions
 */

/**
 * This functions checks if the jukebox queue is long enough, refills the queue if necessary
 * and adds songs or albums to the queue.
 * @param partition_state pointer to myMPD partition state
 * @param add_songs number of songs to add
 * @param jukebox_mode jukebox mode
 * @param playlist playlist to add songs from
 * @return true on success, else false
 */
static bool jukebox_add_to_queue(struct t_partition_state *partition_state,
        struct t_stickerdb_state *stickerdb, struct t_cache *album_cache, long add_songs,
        enum jukebox_modes jukebox_mode, const char *playlist)
{
    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox queue length: %ld", partition_state->jukebox_queue.length);
    if (add_songs > partition_state->jukebox_queue.length) {
        bool rc = jukebox_run_fill_jukebox_queue(partition_state, stickerdb, album_cache, jukebox_mode, playlist);
        if (rc == false) {
            return false;
        }
    }
    long added = 0;
    struct t_list_node *current = partition_state->jukebox_queue.head;
    while (current != NULL &&
        added < add_songs)
    {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
            mpd_run_add(partition_state->conn, current->key);
            if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_add") == true) {
                MYMPD_LOG_NOTICE(partition_state->name, "Jukebox adding song: %s", current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR(partition_state->name, "Jukebox adding song %s failed", current->key);
            }
        }
        else {
            bool rc = mpd_client_add_album_to_queue(partition_state, album_cache, current->key, UINT_MAX, MPD_POSITION_ABSOLUTE, NULL);
            if (rc == true) {
                MYMPD_LOG_NOTICE(partition_state->name, "Jukebox adding album: %s - %s", current->value_p, current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR(partition_state->name, "Jukebox adding album %s - %s failed", current->value_p, current->key);
            }
        }
        if (list_remove_node(&partition_state->jukebox_queue, 0) == false) {
            MYMPD_LOG_ERROR(partition_state->name, "Error removing first entry from jukebox queue");
        }
        current = partition_state->jukebox_queue.head;
    }
    if (added == 0) {
        MYMPD_LOG_ERROR(partition_state->name, "Error adding song(s)");
        send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_ERROR, partition_state->name, "Adding songs from jukebox to queue failed");
        return false;
    }

    if ((jukebox_mode == JUKEBOX_ADD_SONG && partition_state->jukebox_queue.length < MYMPD_JUKEBOX_INTERNAL_SONG_QUEUE_LENGTH_MIN) ||
        (jukebox_mode == JUKEBOX_ADD_ALBUM && partition_state->jukebox_queue.length < MYMPD_JUKEBOX_INTERNAL_ALBUM_QUEUE_LENGTH_MIN))
    {
        bool rc = jukebox_run_fill_jukebox_queue(partition_state, stickerdb, album_cache, jukebox_mode, playlist);
        if (rc == false) {
            return false;
        }
    }
    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox queue length: %ld", partition_state->jukebox_queue.length);
    return true;
}

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
    if (partition_state->jukebox_uniq_tag.tags[0] != MPD_TAG_TITLE) {
        tag_value = mpd_client_get_tag_value_string(song, partition_state->jukebox_uniq_tag.tags[0], tag_value);
    }
    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        list_push(queue_list, mpd_song_get_uri(song), 0, tag_value, NULL);
    }
    else {
        // JUKEBOX_ADD_ALBUM
        sds albumid = album_cache_get_key(sdsempty(), song, &partition_state->config->albums);
        list_push(queue_list, albumid, 0, tag_value, NULL);
        FREE_SDS(albumid);
    }
    FREE_SDS(tag_value);
    mpd_song_free(song);
}

/**
 * Gets the song list from queue and last played.
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
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_queue_meta") == false) {
        FREE_PTR(queue_list);
        return NULL;
    }

    // append last_played to queue list
    int added = 0;
    if (queue_list->length < JUKEBOX_UNIQ_RANGE) {
        struct t_list_node *current = partition_state->last_played.head;
        while (current != NULL) {
            if (mpd_send_list_meta(partition_state->conn, current->key)) {
                if ((song = mpd_recv_song(partition_state->conn)) != NULL) {
                    jukebox_get_last_played_add(partition_state, song, queue_list, jukebox_mode);
                    added++;
                }
                else {
                    MYMPD_LOG_WARN(partition_state->name, "Failure fetching song information for uri \"%s\"", current->key);
                }
            }
            mpd_response_finish(partition_state->conn);
            mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_meta");
            if (added == JUKEBOX_UNIQ_RANGE) {
                break;
            }
            current = current->next;
        }
    }

    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox last_played list length: %ld", queue_list->length);
    return queue_list;
}

/**
 * Wrapper function for the real jukebox queue filling function.
 * @param partition_state pointer to myMPD partition state
 * @param add_songs number of songs or albums to add
 * @param jukebox_mode the jukebox mode
 * @param playlist playlist to add songs from
 * @return true on success, else false
 */
static bool jukebox_run_fill_jukebox_queue(struct t_partition_state *partition_state,
        struct t_stickerdb_state *stickerdb, struct t_cache *album_cache,
        enum jukebox_modes jukebox_mode, const char *playlist)
{
    send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_INFO, partition_state->name, "Filling jukebox queue");
    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox queue to small, adding entities");
    bool rc = jukebox_fill_jukebox_queue(partition_state, stickerdb, album_cache, jukebox_mode, playlist);
    if (rc == false) {
        MYMPD_LOG_ERROR(partition_state->name, "Filling jukebox queue failed, disabling jukebox");
        send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_ERROR, partition_state->name, "Filling jukebox queue failed, disabling jukebox");
        partition_state->jukebox_mode = JUKEBOX_OFF;
        return false;
    }
    return true;
}

/**
 * The real jukebox queue filling function.
 * @param partition_state pointer to myMPD partition state
 * @param jukebox_mode the jukebox mode
 * @param playlist playlist to add songs from
 * @return true on success, else false
 */
static bool jukebox_fill_jukebox_queue(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        struct t_cache *album_cache, enum jukebox_modes jukebox_mode, const char *playlist)
{
    //get last_played and current queue
    struct t_list *queue_list = jukebox_get_last_played(partition_state, jukebox_mode);
    if (queue_list == NULL) {
        return false;
    }

    struct t_random_add_constraints constraints = {
        .filter_include = partition_state->jukebox_filter_include,
        .filter_exclude = partition_state->jukebox_filter_exclude,
        .uniq_tag = partition_state->jukebox_uniq_tag.tags[0],
        .last_played = partition_state->jukebox_last_played,
        .ignore_hated = partition_state->jukebox_ignore_hated,
        .min_song_duration = partition_state->jukebox_min_song_duration,
        .max_song_duration = partition_state->jukebox_max_song_duration
    };

    long expected_length;
    long new_length;
    if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
        expected_length = MYMPD_JUKEBOX_INTERNAL_ALBUM_QUEUE_LENGTH;
        new_length = random_select_albums(partition_state, stickerdb, album_cache, expected_length, queue_list, &partition_state->jukebox_queue, &constraints);
    }
    else {
        // JUKEBOX_ADD_SONG
        expected_length = MYMPD_JUKEBOX_INTERNAL_SONG_QUEUE_LENGTH;
        new_length = random_select_songs(partition_state, stickerdb, expected_length, playlist, queue_list, &partition_state->jukebox_queue, &constraints);
    }

    list_free(queue_list);

    if (new_length < expected_length) {
        MYMPD_LOG_WARN(partition_state->name, "Jukebox queue didn't contain %ld entries", expected_length);
        return false;
    }
    return true;
}
