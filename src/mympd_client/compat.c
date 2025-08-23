/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Compatibility functions for older libmpdclient versions
 */

#include "compile_time.h"
#include "src/mympd_client/compat.h"

/**
 * Compatibility function for "stringnormalization all"
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mympd_client_stringnormalization_all(struct t_partition_state *partition_state) {
    #if LIBMPDCLIENT_CHECK_VERSION(2, 24, 0)
        return mpd_send_all_stringnormalization(partition_state->conn);
    #else
        return mpd_send_command(partition_state->conn, "stringnormalization", "all", NULL);
    #endif
}

/**
 * Compatibility function for "stringnormalization all"
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mympd_client_stringnormalization_clear(struct t_partition_state *partition_state) {
    #if LIBMPDCLIENT_CHECK_VERSION(2, 24, 0)
        return mpd_send_clear_stringnormalization(partition_state->conn);
    #else
        return mpd_send_command(partition_state->conn, "stringnormalization", "clear", NULL);
    #endif
}
