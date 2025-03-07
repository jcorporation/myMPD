/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Functions to populate the jukebox queue
 */

#include "compile_time.h"
#include "src/mympd_worker/jukebox.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"
#include "src/mympd_client/jukebox.h"
#include "src/mympd_client/random_select.h"

#include <string.h>

/**
 * Pushes the created jukebox queue to the mympd api thread
 * @param mympd_worker_state pointer to mpd worker state
 * @return true on success, else false
 */
bool mympd_worker_jukebox_push(struct t_mympd_worker_state *mympd_worker_state) {
    // save and detach the creates jukebox list
    struct t_list *jukebox_queue = mympd_worker_state->partition_state->jukebox.queue;
    mympd_worker_state->partition_state->jukebox.queue = NULL;
    // push it to the mympd api thread
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_JUKEBOX_CREATED, NULL, mympd_worker_state->partition_state->name);
    request->data = jsonrpc_end(request->data);
    request->extra = (void *)jukebox_queue;
    return mympd_queue_push(mympd_api_queue, request, 0);
}

/**
 * Pushes an jukebox creation error to the mympd api thread
 * @param mympd_worker_state pointer to mpd worker state
 * @param error Error message to send
 * @return true on success, else false
 */
bool mympd_worker_jukebox_error(struct t_mympd_worker_state *mympd_worker_state, sds error) {
    // push error to the mympd api thread
    struct t_work_request *request = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_JUKEBOX_ERROR, NULL, mympd_worker_state->partition_state->name);
    request->data = tojson_sds(request->data, "error", error, false);
    request->data = jsonrpc_end(request->data);
    return mympd_queue_push(mympd_api_queue, request, 0);
}

/**
 * Fills the jukebox queue
 * @param mympd_worker_state pointer to mpd worker state
 * @param queue_list list to check uniq constraint against
 * @param add_songs number of songs to additionally add
 * @param error pointer to allocates sds for error message
 * @return bool true on success, else false
 */
bool mympd_worker_jukebox_queue_fill(struct t_mympd_worker_state *mympd_worker_state, struct t_list *queue_list,
        unsigned add_songs, sds *error)
{
    send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_INFO, mympd_worker_state->partition_state->name, "Filling jukebox queue");
    struct t_random_add_constraints constraints = {
        .filter_include = mympd_worker_state->partition_state->jukebox.filter_include,
        .filter_exclude = mympd_worker_state->partition_state->jukebox.filter_exclude,
        .uniq_tag = mympd_worker_state->partition_state->jukebox.uniq_tag.tags[0],
        .last_played = mympd_worker_state->partition_state->jukebox.last_played,
        .ignore_hated = mympd_worker_state->partition_state->jukebox.ignore_hated,
        .min_song_duration = mympd_worker_state->partition_state->jukebox.min_song_duration,
        .max_song_duration = mympd_worker_state->partition_state->jukebox.max_song_duration
    };

    unsigned expected_length;
    unsigned new_length = 0;
    if (mympd_worker_state->partition_state->jukebox.mode == JUKEBOX_ADD_ALBUM) {
        expected_length = JUKEBOX_INTERNAL_ALBUM_QUEUE_LENGTH + add_songs;
        if (cache_get_read_lock(mympd_worker_state->album_cache) == true) {
            new_length = random_select_albums(mympd_worker_state->partition_state, mympd_worker_state->stickerdb, mympd_worker_state->album_cache,
                expected_length, queue_list, mympd_worker_state->partition_state->jukebox.queue, &constraints);
            cache_release_lock(mympd_worker_state->album_cache);
        }
        else {
            *error = sdscat(*error, "Can not get lock for album cache");
            return false;
        }
    }
    else if (mympd_worker_state->partition_state->jukebox.mode == JUKEBOX_ADD_SONG) {
        expected_length = JUKEBOX_INTERNAL_SONG_QUEUE_LENGTH + add_songs;
        new_length = random_select_songs(mympd_worker_state->partition_state, mympd_worker_state->stickerdb, expected_length,
            mympd_worker_state->partition_state->jukebox.playlist, queue_list, mympd_worker_state->partition_state->jukebox.queue, &constraints);
    }
    else {
        *error = sdscat(*error, "Jukebox is disabled");
        return false;
    }

    if (new_length < expected_length) {
        MYMPD_LOG_WARN(mympd_worker_state->partition_state->name, "Jukebox queue didn't contain %u entries", expected_length);
        send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_ERROR, mympd_worker_state->partition_state->name, "Filling jukebox queue failed");
        *error = sdscat(*error, "Filling jukebox queue failed");
        return false;
    }
    return true;
}

/**
 * Fills the jukebox queue and add songs to the mpd queue
 * @param mympd_worker_state pointer to mpd worker state
 * @param queue_list list to check uniq constraint against
 * @param add_songs number of songs to add
 * @param error pointer to allocates sds for error message
 * @return true on success, else false
 */
bool mympd_worker_jukebox_queue_fill_add(struct t_mympd_worker_state *mympd_worker_state, struct t_list *queue_list,
        unsigned add_songs, sds *error)
{
    return mympd_worker_jukebox_queue_fill(mympd_worker_state, queue_list, add_songs, error) &&
        jukebox_add_to_queue(mympd_worker_state->partition_state, mympd_worker_state->album_cache, add_songs, error);
}
