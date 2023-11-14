/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/shortcuts.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/album_cache.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/search.h"

/**
 * Sends mpd_command_list_end if MPD is connected.
 * Usage: Set MPD_FAILURE if a send command returns false in a command list.
 * Workaround for not cancelable command lists
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mpd_client_command_list_end_check(struct t_partition_state *partition_state) {
    if (partition_state->conn_state == MPD_CONNECTED) {
        return mpd_command_list_end(partition_state->conn);
    }
    MYMPD_LOG_ERROR(partition_state->name, "Skipping mpd_command_list_end");
    return false;
}

/**
 * Adds an album to the queue
 * @param partition_state pointer to partition state
 * @param album_id album id to add
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mpd_client_add_album_to_queue(struct t_partition_state *partition_state, struct t_cache *album_cache,
    sds album_id, unsigned to, unsigned whence, sds *error)
{
    struct mpd_song *mpd_album = album_cache_get_album(album_cache, album_id);
    if (mpd_album == NULL) {
        *error = sdscat(*error, "Album not found");
        return false;
    }
    sds expression = get_search_expression_album(partition_state->mpd_state->tag_albumartist,
        mpd_album, &partition_state->config->albums);
    const char *sort = NULL;
    bool sortdesc = false;
    bool rc = mpd_client_search_add_to_queue(partition_state, expression, to, whence, sort, sortdesc, error);
    FREE_SDS(expression);
    return rc;
}
