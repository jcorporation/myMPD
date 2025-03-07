/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD queue functions.
 */

#include "compile_time.h"
#include "src/mympd_client/queue.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_api/status.h"

/**
 * Clears the queue
 * @param partition_state pointer to partition state
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_client_queue_clear(struct t_partition_state *partition_state, sds *error) {
    mpd_run_clear(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, error, "mpd_run_clear");
}

/**
 * Plays the newest inserted song in the queue
 * @param partition_state pointer to partition state
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_client_queue_play_newly_inserted(struct t_partition_state *partition_state, sds *error) {
    unsigned song_pos = 0;
    unsigned song_id = 0;
    if (mpd_send_queue_changes_brief(partition_state->conn, partition_state->queue_version)) {
        mpd_recv_queue_change_brief(partition_state->conn, &song_pos, &song_id);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, error, "mpd_send_queue_changes_brief") == false) {
        return false;
    }
    mpd_run_play_id(partition_state->conn, song_id);
    return mympd_check_error_and_recover(partition_state, error, "mpd_run_play_id");
}

/**
 * Tries to play the last inserted song and checks for success
 * @param partition_state pointer to partition state
 * @param play really play last inserts song
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_client_queue_check_start_play(struct t_partition_state *partition_state, bool play, sds *error) {
    if (play == true) {
        MYMPD_LOG_DEBUG(partition_state->name, "Start playing newly added songs");
        return mympd_client_queue_play_newly_inserted(partition_state, error);
    }
    return true;
}

/**
 * Prints the queue status and updates internal state
 * @param partition_state pointer to partition state
 */
void mympd_client_queue_status_update(struct t_partition_state *partition_state) {
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status != NULL) {
        partition_state->queue_version = mpd_status_get_queue_version(status);
        partition_state->queue_length = mpd_status_get_queue_length(status);
        partition_state->crossfade = (time_t)mpd_status_get_crossfade(status);
        partition_state->play_state = mpd_status_get_state(status);
        mpd_status_free(status);
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state, NULL, "mpd_run_status");
}

/**
 * Prints the queue status and updates internal state
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
 * @param buffer already allocated sds string to append the response
 * @return pointer to buffer
 */
sds mympd_client_queue_status_print(struct t_partition_state *partition_state, struct t_cache *album_cache, sds buffer) {
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status != NULL) {
        partition_state->queue_version = mpd_status_get_queue_version(status);
        partition_state->queue_length = mpd_status_get_queue_length(status);
        partition_state->crossfade = (time_t)mpd_status_get_crossfade(status);
        partition_state->play_state = mpd_status_get_state(status);

        buffer = jsonrpc_notify_start(buffer, JSONRPC_EVENT_UPDATE_QUEUE);
        buffer = mympd_api_status_print(partition_state, album_cache, buffer, status);
        buffer = jsonrpc_end(buffer);
        mpd_status_free(status);
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state, NULL, "mpd_run_status");
    return buffer;
}
