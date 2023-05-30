/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/shortcuts.h"

#include "dist/libmympdclient/include/mpd/client.h"

/**
 * Sends mpd_command_list_end if MPD is connected.
 * Usage: Set MPD_FAILURE if a command returns false in a command list.
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mpd_client_command_list_end_check(struct t_partition_state *partition_state) {
    if (partition_state->conn_state == MPD_CONNECTED) {
        return mpd_command_list_end(partition_state->conn);
    }
    return false;
}
