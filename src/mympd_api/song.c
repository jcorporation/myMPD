/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD song API
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
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param uri song uri
 * @return pointer to buffer
 */
sds mympd_api_song_details(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
    sds buffer, unsigned request_id, const char *uri)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_SONG_DETAILS;
    if (mpd_send_list_meta(partition_state->conn, uri)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        struct mpd_song *song;
        if ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            const struct mpd_audio_format *audioformat = mpd_song_get_audio_format(song);
            buffer = printAudioFormat(buffer, audioformat);
            buffer = sdscatlen(buffer, ",", 1);
            buffer = print_song_tags(buffer, partition_state->mpd_state, &partition_state->mpd_state->tags_mympd, song);
            mpd_song_free(song);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_list_meta") == false) {
        return buffer;
    }

    if (partition_state->mpd_state->feat.stickers == true) {
        struct t_stickers sticker;
        stickers_reset(&sticker);
        stickers_enable_all(&sticker, STICKER_TYPE_SONG);
        buffer = mympd_api_sticker_get_print(buffer, mympd_state->stickerdb, STICKER_TYPE_SONG, uri, &sticker);
    }

    buffer = sdscatlen(buffer, ",", 1);
    buffer = mympd_api_get_extra_media(buffer, partition_state->mpd_state, mympd_state->booklet_name, mympd_state->info_txt_name, uri, false);
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
sds mympd_api_song_comments(struct t_partition_state *partition_state, sds buffer, unsigned request_id, const char *uri) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SONG_COMMENTS;
    unsigned entities_returned = 0;
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
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "totalEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}
