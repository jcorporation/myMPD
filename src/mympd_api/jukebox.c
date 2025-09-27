/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD jukebox API
 */

#include "compile_time.h"
#include "src/mympd_api/jukebox.h"

#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/search.h"
#include "src/mympd_api/sticker.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/jukebox.h"
#include "src/mympd_client/search.h"
#include "src/mympd_client/stickerdb.h"
#include "src/mympd_client/tags.h"

/**
 * Clears the jukebox queue.
 * This is a simple wrapper around list_clear.
 * @param partition_name name of the partition
 * @param list the jukebox queue
 */
void mympd_api_jukebox_clear(struct t_list *list, sds partition_name) {
    list_clear(list);
    //notify clients
    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_name);
}

/**
 * Removes entries from the jukebox queue.
 * @param list the jukebox queue
 * @param positions positions to remove
 * @param partition_name name of the partition
 * @param error pointer to sds string to populate an error string
 * @return true on success, else false
 */
bool mympd_api_jukebox_rm_entries(struct t_list *list, struct t_list *positions, sds partition_name, sds *error) {
    if (positions->length == 0) {
        *error = sdscat(*error, "No song positions provided");
        return false;
    }
    list_sort_by_value_i(positions, LIST_SORT_DESC);
    struct t_list_node *current;
    bool rc = true;
    while ((current = list_shift_first(positions)) != NULL) {
        rc = list_remove_node(list, (unsigned)current->value_i);
        list_node_free(current);
        if (rc == false) {
            *error = sdscat(*error, "Could not remove song from jukebox queue");
            break;
        }
    }
    //notify clients
    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_name);
    return rc;
}

/**
 * Returns the length of the jukebox queue
 * @param partition_state pointer to myMPD partition state
 * @param buffer already allocated sds string to append the result
 * @param cmd_id jsonrpc method
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds mympd_api_jukebox_length(struct t_partition_state *partition_state,
        sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id)
{
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = tojson_uint(buffer, "length", partition_state->jukebox.queue->length, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Appends songs the the jukebox queue.
 * This is only allowed in jukebox script mode.
 * @param partition_state pointer to myMPD partition state
 * @param uris list of uris to add
 * @return true on success, else false
 */
bool mympd_api_jukebox_append_uris(struct t_partition_state *partition_state,
        struct t_list *uris)
{
    if (partition_state->jukebox.mode != JUKEBOX_SCRIPT) {
        MYMPD_LOG_ERROR(partition_state->name, "Inserting jukebox songs is only allowed in script mode.");
        return false;
    }
    struct t_list_node *current = uris->head;
    while (current != NULL) {
        list_push(partition_state->jukebox.queue, current->key, 0, NULL, NULL);
        current = current->next;
    }
    return true;
}

/**
 * Prints the jukebox queue as an jsonrpc response
 * @param partition_state pointer to myMPD partition state
 * @param stickerdb pointer to stickerdb state
 * @param buffer already allocated sds string to append the result
 * @param cmd_id jsonrpc method
 * @param request_id jsonrpc request id
 * @param offset offset for printing
 * @param limit max entries to print
 * @param expression mpd search expression
 * @param tagcols columns to print
 * @return pointer to buffer
 */
sds mympd_api_jukebox_list(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id, unsigned offset, unsigned limit, sds expression, const struct t_fields *tagcols)
{
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    unsigned entities_found = 0;
    unsigned real_limit = offset + limit;
    struct t_list *expr_list = parse_search_expression_to_list(expression, SEARCH_TYPE_SONG);
    if (expr_list == NULL) {
        sdsclear(buffer);
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Invalid search expression");
        return buffer;
    }
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    bool print_stickers = check_get_sticker(partition_state->mpd_state->feat.stickers, &tagcols->stickers);
    if (print_stickers == true) {
        stickerdb_exit_idle(stickerdb);
    }
    if (partition_state->jukebox.mode == JUKEBOX_ADD_SONG ||
        partition_state->jukebox.mode == JUKEBOX_SCRIPT)
    {
        struct t_list_node *current = partition_state->jukebox.queue->head;
        while (current != NULL) {
            if (mpd_send_list_meta(partition_state->conn, current->key)) {
                struct mpd_song *song;
                if ((song = mpd_recv_song(partition_state->conn)) != NULL) {
                    if (search_expression_song(song, expr_list, &tagcols->mpd_tags) == true) {
                        if (entities_found >= offset &&
                            entities_found < real_limit)
                        {
                            if (entities_returned++) {
                                buffer = sdscatlen(buffer, ",", 1);
                            }
                            buffer = sdscat(buffer, "{\"Type\": \"song\",");
                            buffer = tojson_uint(buffer, "Pos", entity_count, true);
                            buffer = print_song_tags(buffer, partition_state->mpd_state, &tagcols->mpd_tags, song);
                            if (print_stickers == true) {
                                buffer = mympd_api_sticker_get_print_batch(buffer, stickerdb, STICKER_TYPE_SONG, mpd_song_get_uri(song), &tagcols->stickers);
                            }
                            buffer = sdscatlen(buffer, "}", 1);
                        }
                        entities_found++;
                    }
                    entity_count++;
                    mpd_song_free(song);
                }
            }
            mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_meta");
            current = current->next;
        }
    }
    else if (partition_state->jukebox.mode == JUKEBOX_ADD_ALBUM) {
        struct t_list_node *current = partition_state->jukebox.queue->head;
        sds album_exp = sdsempty();
        while (current != NULL) {
            struct t_album *album = (struct t_album *)current->user_data;
            if (search_expression_album(album, expr_list, &tagcols->mpd_tags) == true) {
                if (entities_found >= offset &&
                    entities_found < real_limit)
                {
                    if (entities_returned++) {
                        buffer = sdscatlen(buffer, ",", 1);
                    }
                    buffer = sdscat(buffer, "{\"Type\": \"album\",");
                    buffer = tojson_uint(buffer, "Pos", entity_count, true);
                    buffer = print_album_tags(buffer, &partition_state->mpd_state->config->albums, &partition_state->mpd_state->tags_album, album);
                    if (print_stickers == true) {
                        buffer = sdscatlen(buffer, ",", 1);
                        album_exp = get_search_expression_album(album_exp, partition_state->mpd_state->tag_albumartist, album, &partition_state->config->albums);
                        buffer = mympd_api_sticker_get_print_batch(buffer, stickerdb, STICKER_TYPE_FILTER, album_exp, &tagcols->stickers);
                    }
                    buffer = sdscatlen(buffer, "}", 1);
                }
                entities_found++;
            }
            entity_count++;
            current = current->next;
        }
        FREE_SDS(album_exp);
    }
    if (print_stickers == true) {
        stickerdb_enter_idle(stickerdb);
    }
    free_search_expression_list(expr_list);
    buffer = sdscatlen(buffer, "],", 2);
    const char *jukebox_mode_str = jukebox_mode_lookup(partition_state->jukebox.mode);
    buffer = tojson_char(buffer, "jukeboxMode", jukebox_mode_str, true);
    buffer = tojson_uint(buffer, "totalEntities", entities_found, true);
    buffer = tojson_uint(buffer, "offset", offset, true);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);

    return buffer;
}
