/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_song.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../mpd_client/mpd_client_errorhandler.h"
#include "../mpd_client/mpd_client_tags.h"
#include "mympd_api_extra_media.h"
#include "mympd_api_sticker.h"

//public functions
sds mympd_api_fingerprint(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                                 const char *uri)
{
    char fp_buffer[8192];
    const char *fingerprint = mpd_run_getfingerprint_chromaprint(mympd_state->mpd_state->conn, uri, fp_buffer, sizeof(fp_buffer));
    if (fingerprint == NULL) {
        check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false);
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, method, request_id);
    buffer = tojson_char(buffer, "fingerprint", fingerprint, false);
    buffer = jsonrpc_respond_end(buffer);

    mpd_response_finish(mympd_state->mpd_state->conn);
    check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false);

    return buffer;
}

sds mympd_api_songdetails(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                               const char *uri)
{
    bool rc = mpd_send_list_meta(mympd_state->mpd_state->conn, uri);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_meta") == false) {
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, method, request_id);

    struct mpd_song *song;
    if ((song = mpd_recv_song(mympd_state->mpd_state->conn)) != NULL) {
        const struct mpd_audio_format *audioformat = mpd_song_get_audio_format(song);
        buffer = printAudioFormat(buffer, audioformat);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = get_song_tags(buffer, mympd_state->mpd_state, &mympd_state->mpd_state->tag_types_mympd, song);
        mpd_song_free(song);
    }

    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    if (mympd_state->mpd_state->feat_mpd_stickers) {
        buffer = sdscatlen(buffer, ",", 1);
        buffer = mympd_api_sticker_list(buffer, &mympd_state->sticker_cache, uri);
    }

    buffer = sdscatlen(buffer, ",", 1);
    buffer = get_extra_media(mympd_state, buffer, uri, false);
    buffer = jsonrpc_respond_end(buffer);
    return buffer;
}

sds mympd_api_read_comments(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                             const char *uri)
{
    bool rc = mpd_send_read_comments(mympd_state->mpd_state->conn, uri);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_meta") == false) {
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":{");
    struct mpd_pair *pair;
    int entities_returned = 0;
    while ((pair = mpd_recv_pair(mympd_state->mpd_state->conn)) != NULL) {
        if (entities_returned++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_char(buffer, pair->name, pair->value,false);
        mpd_return_pair(mympd_state->mpd_state->conn, pair);
    }
    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    buffer = sdscatlen(buffer, "},", 2);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "totalEntities", entities_returned, false);
    buffer = jsonrpc_respond_end(buffer);
    return buffer;
}
