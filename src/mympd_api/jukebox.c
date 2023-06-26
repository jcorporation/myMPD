/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/jukebox.h"

#include "dist/utf8/utf8.h"
#include "src/lib/album_cache.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/search_local.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/sticker.h"

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
 * @param positions positions to remove, must be ordered descending
 * @param partition_name name of the partition
 * @param error pointer to sds string to populate an error string
 * @return true on success, else false
 */
bool mympd_api_jukebox_rm_entries(struct t_list *list, struct t_list *positions, sds partition_name, sds *error) {
    if (positions->length == 0) {
        *error = sdscat(*error, "No song positions provided");
        return false;
    }
    struct t_list_node *current;
    bool rc = true;
    while ((current = list_shift_first(positions)) != NULL) {
        rc = list_remove_node(list, (long)current->value_i);
        list_node_free(current);
        if (rc == false) {
            break;
        }
    }
    //notify clients
    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_name);
    return rc;
}

/**
 * Prints the jukebox queue as an jsonrpc response
 * @param partition_state pointer to myMPD partition state
 * @param buffer already allocated sds string to append the result
 * @param cmd_id jsonrpc method
 * @param request_id jsonrpc request id
 * @param offset offset for printing
 * @param limit max entries to print
 * @param searchstr string to search
 * @param tagcols columns to print
 * @return pointer to buffer
 */
sds mympd_api_jukebox_list(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id, long request_id,
        long offset, long limit, sds searchstr, const struct t_tags *tagcols)
{
    long entity_count = 0;
    long entities_returned = 0;
    long real_limit = offset + limit;

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    if (partition_state->jukebox_mode == JUKEBOX_ADD_SONG) {
        struct t_list_node *current = partition_state->jukebox_queue.head;
        while (current != NULL) {
            if (mpd_send_list_meta(partition_state->conn, current->key)) {
                struct mpd_song *song;
                if ((song = mpd_recv_song(partition_state->conn)) != NULL) {
                    if (search_mpd_song(song, searchstr, tagcols) == true) {
                        if (entity_count >= offset &&
                            entity_count < real_limit)
                        {
                            if (entities_returned++) {
                                buffer = sdscatlen(buffer, ",", 1);
                            }
                            buffer = sdscatlen(buffer, "{", 1);
                            buffer = tojson_long(buffer, "Pos", entity_count, true);
                            buffer = get_song_tags(buffer, partition_state->mpd_state->feat_tags, tagcols, song);
                            if (partition_state->mpd_state->feat_stickers == true &&
                                partition_state->mpd_state->sticker_cache.cache != NULL)
                            {
                                buffer = sdscatlen(buffer, ",", 1);
                                buffer = mympd_api_sticker_get_print(buffer, &partition_state->mpd_state->sticker_cache, mpd_song_get_uri(song));
                            }
                            buffer = sdscatlen(buffer, "}", 1);
                        }
                        entity_count++;
                    }
                    mpd_song_free(song);
                }
            }
            mpd_response_finish(partition_state->conn);
            mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_meta");
            current = current->next;
        }
    }
    else if (partition_state->jukebox_mode == JUKEBOX_ADD_ALBUM) {
        struct t_list_node *current = partition_state->jukebox_queue.head;
        while (current != NULL) {
            if (utf8casestr(current->key, searchstr) != NULL ||
                utf8casestr(current->value_p, searchstr) != NULL)
            {
                if (entity_count >= offset &&
                    entity_count < real_limit)
                {
                    if (entities_returned++) {
                        buffer = sdscatlen(buffer, ",", 1);
                    }
                    struct mpd_song *album = (struct mpd_song *)current->user_data;
                    buffer = sdscatlen(buffer, "{", 1);
                    buffer = tojson_long(buffer, "Pos", entity_count, true);
                    buffer = tojson_char(buffer, "uri", "Album", true);
                    buffer = tojson_char(buffer, "Title", "", true);
                    buffer = tojson_char(buffer, "Album", current->key, true);
                    sds albumkey = album_cache_get_key(album);
                    buffer = tojson_char(buffer, "AlbumId", albumkey, true);
                    FREE_SDS(albumkey);
                    buffer = sdscat(buffer, "\"AlbumArtist\":");
                    buffer = mpd_client_get_tag_values(album, MPD_TAG_ALBUM_ARTIST, buffer);
                    buffer = sdscat(buffer, ",\"Artist\":");
                    buffer = mpd_client_get_tag_values(album, MPD_TAG_ARTIST, buffer);
                    buffer = sdscatlen(buffer, "}", 1);
                }
                entity_count++;
            }
            current = current->next;
        }
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "jukeboxMode", partition_state->jukebox_mode, true);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);

    return buffer;
}
