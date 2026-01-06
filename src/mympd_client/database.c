/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD database helper functions
 */

#include "compile_time.h"
#include "src/mympd_client/database.h"

#include "src/lib/utility.h"
#include "src/mympd_client/errorhandler.h"

/**
 * Returns the mpd database last modification time
 * @param partition_state pointer to partition specific states
 * @return last modification time
 */
time_t mympd_client_get_db_mtime(struct t_partition_state *partition_state) {
    time_t mtime = 0;
    struct mpd_stats *stats = mpd_run_stats(partition_state->conn);
    if (stats != NULL) {
        mtime = (time_t)mpd_stats_get_db_update_time(stats);
        mpd_stats_free(stats);
    }
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_stats") == false) {
        mtime = 0;
    }
    return mtime;
}

/**
 * Checks for a song in the database
 * @param partition_state Pointer to partition state
 * @param uri Song uri to check
 * @return true on success or uri is a stream, else false
 */
bool mympd_client_song_exists(struct t_partition_state *partition_state, const char *uri) {
    if (is_streamuri(uri) == true) {
        return true;
    }
    if (mpd_send_list_all(partition_state->conn, uri) == true &&
        mpd_response_finish(partition_state->conn) == true)
    {
        return true;
    }
    // Song does not exist
    mympd_clear_finish(partition_state);
    return false;
}
