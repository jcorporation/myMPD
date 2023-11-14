/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/last_played.h"

#include "dist/sds/sds.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/search_local.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/sticker.h"

#include <errno.h>
#include <string.h>

/**
 * Private definitions
 */

static sds get_last_played_obj(struct t_partition_state *partition_state, struct t_partition_state *stickerdb,
        sds buffer, long entity_count, long long last_played, const char *uri, struct t_list *expr_list, const struct t_tags *tagcols);

/**
 * Public functions
 */

/**
 * Adds a song from queue with song_id to the last played list in memory
 * @param partition_state pointer to partition state
 * @param song_id the song id to add
 * @return true on success, else false
 */
bool mympd_api_last_played_add_song(struct t_partition_state *partition_state, long last_played_count, int song_id) {
    if (song_id == -1 ||                                    // no current song
        last_played_count == 0) // last played is disabled
    {
        return true;
    }

    // get current playing song and add it
    struct mpd_song *song = mpd_run_get_queue_song_id(partition_state->conn, (unsigned)song_id);
    if (song != NULL) {
        const char *uri = mpd_song_get_uri(song);
        if (is_streamuri(uri) == true) {
            //Don't add streams to last played list
            mpd_song_free(song);
            return true;
        }
        list_insert(&partition_state->last_played, uri, (long long)time(NULL), NULL, NULL);
        mpd_song_free(song);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_get_queue_song_id") == false) {
        return false;
    }

    list_crop(&partition_state->last_played, last_played_count, NULL);

    //notify clients
    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_LAST_PLAYED, partition_state->name);
    return true;
}

/**
 * Prints a jsonrpc response with the last played songs (memory and disc)
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param offset offset
 * @param limit max number of entries to return
 * @param expression mpd search expression
 * @param tagcols columns to print
 * @return pointer to buffer
 */
sds mympd_api_last_played_list(struct t_partition_state *partition_state, struct t_partition_state *stickerdb,
        sds buffer, long request_id, long offset, long limit, sds expression, const struct t_tags *tagcols)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_LAST_PLAYED_LIST;
    long entity_count = 0;
    long entities_returned = 0;
    long entities_found = 0;

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    sds obj = sdsempty();

    long real_limit = offset + limit;
    struct t_list *expr_list = parse_search_expression_to_list(expression);
    if (partition_state->mpd_state->feat.stickers == true &&
        tagcols->stickers_len > 0)
    {
        stickerdb_exit_idle(stickerdb);
    }

    struct t_list_node *current = partition_state->last_played.head;
    while (current != NULL) {
        obj = get_last_played_obj(partition_state, stickerdb, obj, entity_count, current->value_i,
            current->key, expr_list, tagcols);
        if (sdslen(obj) > 0) {
            if (entities_found >= offset) {
                if (entities_returned++) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sdscatsds(buffer, obj);
            }
            sdsclear(obj);
            entities_found++;
            if (entities_returned == real_limit) {
                break;
            }
        }
        entity_count++;
        current = current->next;
    }
    FREE_SDS(obj);
    if (partition_state->mpd_state->feat.stickers == true &&
        tagcols->stickers_len > 0)
    {
        stickerdb_enter_idle(stickerdb);
    }
    free_search_expression_list(expr_list);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "totalEntities", -1, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);

    return buffer;
}

/**
 * Private functions
 */

/**
 * Gets the song and searches for searchstr and prints it as json object
 * @param partition_state pointer to partition state
 * @param buffer already allocated buffer to append the result
 * @param entity_count position in the list
 * @param last_played songs last played time as unix timestamp
 * @param uri uri of the song
 * @param expr_list list of search expressions
 * @param tagcols columns to print
 * @return pointer to buffer
 */
static sds get_last_played_obj(struct t_partition_state *partition_state, struct t_partition_state *stickerdb,
        sds buffer, long entity_count, long long last_played, const char *uri, struct t_list *expr_list, const struct t_tags *tagcols)
{
    if (mpd_send_list_meta(partition_state->conn, uri)) {
        struct mpd_song *song;
        if ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            if (search_song_expression(song, expr_list, tagcols) == true) {
                buffer = sdscat(buffer, "{\"Type\": \"song\",");
                buffer = tojson_long(buffer, "Pos", entity_count, true);
                buffer = tojson_llong(buffer, "LastPlayed", last_played, true);
                buffer = print_song_tags(buffer, partition_state->mpd_state, tagcols, song);
                if (partition_state->mpd_state->feat.stickers == true &&
                    tagcols->stickers_len > 0)
                {
                    buffer = mympd_api_sticker_get_print_batch(buffer, stickerdb, mpd_song_get_uri(song), tagcols);
                }
                buffer = sdscatlen(buffer, "}", 1);
            }
            mpd_song_free(song);
        }
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_meta");
    return buffer;
}
