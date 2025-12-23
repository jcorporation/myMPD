/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD tags helper functions
 */

#include "compile_time.h"
#include "src/mympd_client/database.h"

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
