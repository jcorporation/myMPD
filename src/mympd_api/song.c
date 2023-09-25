/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/song.h"

#include "src/lib/jsonrpc.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/extra_media.h"
#include "src/mympd_api/sticker.h"

/**
 * Gets the song details, tags and stickers
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param uri song uri
 * @return pointer to buffer
 */
sds mympd_api_song_details(struct t_partition_state *partition_state, sds buffer, long request_id, const char *uri) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SONG_DETAILS;
    if (mpd_send_list_meta(partition_state->conn, uri)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        struct mpd_song *song;
        if ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            const struct mpd_audio_format *audioformat = mpd_song_get_audio_format(song);
            buffer = printAudioFormat(buffer, audioformat);
            buffer = sdscatlen(buffer, ",", 1);
            buffer = print_song_tags(buffer, partition_state->mpd_state->feat_tags, &partition_state->mpd_state->tags_mympd,
                song, partition_state->mympd_state->config->albums);
            mpd_song_free(song);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_list_meta") == false) {
        return buffer;
    }

    if (partition_state->mpd_state->feat_stickers) {
        struct t_tags tagcols;
        reset_t_tags(&tagcols);
        tags_enable_all_stickers(&tagcols);
        buffer = mympd_api_sticker_get_print(buffer, partition_state->mympd_state->stickerdb, uri, &tagcols);
    }

    buffer = sdscatlen(buffer, ",", 1);
    buffer = mympd_api_get_extra_media(partition_state->mpd_state, buffer, uri, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Gets the comments from a song
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param uri song uri
 * @return pointer to buffer
 */
sds mympd_api_song_comments(struct t_partition_state *partition_state, sds buffer, long request_id, const char *uri) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SONG_COMMENTS;
    int entities_returned = 0;
    if (mpd_send_read_comments(partition_state->conn, uri)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":{");
        struct mpd_pair *pair;
        while ((pair = mpd_recv_pair(partition_state->conn)) != NULL) {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = tojson_char(buffer, pair->name, pair->value,false);
            mpd_return_pair(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_read_comments") == false) {
        return buffer;
    }
    buffer = sdscatlen(buffer, "},", 2);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "totalEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}
