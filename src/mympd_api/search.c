/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD search API
 */

#include "compile_time.h"
#include "src/mympd_api/search.h"

#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/search.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/sticker.h"

/**
 * Searches the mpd database for songs by expression and returns an jsonrpc result
 * @param partition_state pointer to partition specific states
 * @param stickerdb pointer to stickerdb state
 * @param buffer already allocated sds string to append the result
 * @param request_id jsonrpc request id
 * @param expression mpd search expression
 * @param sort tag to sort
 * @param sortdesc false = ascending, true = descending
 * @param offset result offset
 * @param limit max number of results to return
 * @param tagcols tags to return
 * @param result pointer to bool to set returncode
 * @return pointer to buffer
 */
sds mympd_api_search_songs(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb, 
        sds buffer, unsigned request_id, const char *expression, const char *sort, bool sortdesc,
        unsigned offset, unsigned limit, const struct t_fields *tagcols, bool *result)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_DATABASE_SEARCH;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");

    unsigned real_limit = limit == 0 ? offset + MPD_PLAYLIST_LENGTH_MAX : offset + limit;
    if (mpd_search_db_songs(partition_state->conn, false) == false ||
        mpd_search_add_expression(partition_state->conn, expression) == false ||
        mpd_client_add_search_sort_param(partition_state, sort, sortdesc, false) == false ||
        mpd_search_add_window(partition_state->conn, offset, real_limit) == false)
    {
        mpd_search_cancel(partition_state->conn);
        *result = false;
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
                JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
    }

    unsigned entities_returned = 0;
    if (partition_state->mpd_state->feat.stickers == true &&
        tagcols->stickers.len > 0)
    {
        stickerdb_exit_idle(stickerdb);
    }
    if (mpd_search_commit(partition_state->conn) == true) {
        struct mpd_song *song;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscat(buffer, "{\"Type\": \"song\",");
            buffer = print_song_tags(buffer, partition_state->mpd_state, &tagcols->mpd_tags, song);
            if (partition_state->mpd_state->feat.stickers == true &&
                tagcols->stickers.len > 0)
            {
                buffer = mympd_api_sticker_get_print_batch(buffer, stickerdb, MPD_STICKER_TYPE_SONG, mpd_song_get_uri(song), &tagcols->stickers);
            }
            buffer = sdscatlen(buffer, "}", 1);
            mpd_song_free(song);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (partition_state->mpd_state->feat.stickers == true &&
        tagcols->stickers.len > 0)
    {
        stickerdb_enter_idle(stickerdb);
    }
    *result = mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_search_db_songs");
    if (*result == false) {
        return buffer;
    }

    buffer = sdscatlen(buffer, "],", 2);

    buffer = offset == 0 && entities_returned < limit
        ? tojson_uint(buffer, "totalEntities", entities_returned, true)
        : tojson_int(buffer, "totalEntities", -1, true);

    buffer = tojson_uint(buffer, "offset", offset, true);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_char(buffer, "expression", expression, true);
    buffer = tojson_char(buffer, "sort", sort, true);
    buffer = tojson_bool(buffer, "sortdesc", sortdesc, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}
