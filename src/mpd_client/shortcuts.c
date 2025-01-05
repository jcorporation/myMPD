/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Some MPD shortcuts
 */

#include "compile_time.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/shortcuts.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/cache_rax_album.h"
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
 * Insert uris into the queue
 * @param partition_state pointer to partition state
 * @param uris uris to insert
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mpd_client_add_uris_to_queue(struct t_partition_state *partition_state, struct t_list *uris,
        unsigned to, unsigned whence, sds *error)
{
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat.whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    if (uris->length == 0) {
        *error = sdscat(*error, "No uris provided");
        return false;
    }
    if (mpd_command_list_begin(partition_state->conn, false)) {
        struct t_list_node *current;
        while ((current = list_shift_first(uris)) != NULL) {
            bool rc = to == UINT_MAX
                ? mpd_send_add(partition_state->conn, current->key)
                : mpd_send_add_whence(partition_state->conn, current->key, to, whence);
            list_node_free(current);
            if (rc == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_add");
                break;
            }
            if (to != UINT_MAX) {
                to++;
            }
        }
        mpd_client_command_list_end_check(partition_state);
    }
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_add_whence");
}

/**
 * Inserts an album into the queue
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
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
    sds expression = get_search_expression_album(sdsempty(), partition_state->mpd_state->tag_albumartist,
        mpd_album, &partition_state->config->albums);
    const char *sort = NULL;
    bool sortdesc = false;
    bool rc = mpd_client_search_add_to_queue(partition_state, expression, to, whence, sort, sortdesc, error);
    FREE_SDS(expression);
    return rc;
}

/**
 * Inserts albums into the queue
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
 * @param albumids album ids to insert
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mpd_client_add_albums_to_queue(struct t_partition_state *partition_state, struct t_cache *album_cache,
    struct t_list *albumids, unsigned to, unsigned whence, sds *error)
{
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat.whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    if (albumids->length == 0) {
        *error = sdscat(*error, "No album ids provided");
        return false;
    }
    struct t_list_node *current = albumids->head;
    bool rc = true;
    while (current != NULL) {
        rc = mpd_client_add_album_to_queue(partition_state, album_cache, current->key, to, whence, error);
        if (rc == false) {
            break;
        }
        current = current->next;
    }
    return rc;
}
