/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../utility.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "mpd_client_utility.h"
#include "mpd_client_queue.h"
#include "../dist/src/frozen/frozen.h"

sds mpd_client_get_queue_state(t_mpd_state *mpd_state, sds buffer) {
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (status == NULL) {
        check_error_and_recover(mpd_state, NULL, NULL, 0);
        return buffer;
    }

    mpd_state->queue_version = mpd_status_get_queue_version(status);
    mpd_state->queue_length = mpd_status_get_queue_length(status);
    mpd_state->crossfade = mpd_status_get_crossfade(status);
    mpd_state->state = mpd_status_get_state(status);

    if (buffer != NULL) {
        buffer = mpd_client_put_queue_state(status, buffer);
    }
    mpd_status_free(status);
    return buffer;
}

sds mpd_client_put_queue_state(struct mpd_status *status, sds buffer) {
    buffer = jsonrpc_start_notify(buffer, "update_queue");
    buffer = tojson_long(buffer, "state", mpd_status_get_state(status), true);
    buffer = tojson_long(buffer, "queueLength", mpd_status_get_queue_length(status), true);
    buffer = tojson_long(buffer, "queueVersion", mpd_status_get_queue_version(status), true);
    buffer = tojson_long(buffer, "songPos", mpd_status_get_song_pos(status), true);
    buffer = tojson_long(buffer, "nextSongPos", mpd_status_get_next_song_pos(status), false);
    buffer = jsonrpc_end_notify(buffer);
    return buffer;
}

sds mpd_client_put_queue(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                         const unsigned int offset, const t_tags *tagcols)
{
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (status == NULL) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
    }
        
    if (!mpd_send_list_queue_range_meta(mpd_state->conn, offset, offset + mpd_state->max_elements_per_page)) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }
        
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    int totalTime = 0;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    struct mpd_entity *entity;
    while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            const struct mpd_song *song = mpd_entity_get_song(entity);
            totalTime += mpd_song_get_duration(song);
            entity_count++;
            if (entities_returned++) {
                buffer = sdscat(buffer, ",");
            }
            buffer = sdscat(buffer, "{");
            buffer = tojson_long(buffer, "id", mpd_song_get_id(song), true);
            buffer = tojson_long(buffer, "Pos", mpd_song_get_pos(song), true);
            buffer = put_song_tags(buffer, mpd_state, tagcols, song);
            buffer = sdscat(buffer, "}");
        }
        mpd_entity_free(entity);
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalTime", totalTime, true);
    buffer = tojson_long(buffer, "totalEntities", mpd_status_get_queue_length(status), true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "queueVersion", mpd_status_get_queue_version(status), false);
    buffer = jsonrpc_end_result(buffer);
    
    mpd_state->queue_version = mpd_status_get_queue_version(status);
    mpd_state->queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);
    
    return buffer;
}

sds mpd_client_crop_queue(t_mpd_state *mpd_state, sds buffer, sds method, int request_id) {
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    const int length = mpd_status_get_queue_length(status) - 1;
    int playing_song_pos = mpd_status_get_song_pos(status);

    if (length < 1) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "A queue longer than 1 song in length is required to crop", true);
        LOG_ERROR("A playlist longer than 1 song in length is required to crop");
    }
    else if (mpd_status_get_state(status) == MPD_STATE_PLAY || mpd_status_get_state(status) == MPD_STATE_PAUSE) {
        playing_song_pos++;
        if (playing_song_pos < length) {
            mpd_run_delete_range(mpd_state->conn, playing_song_pos, -1);
        }
        playing_song_pos--;
        if (playing_song_pos > 0) {
            mpd_run_delete_range(mpd_state->conn, 0, playing_song_pos--);
        }
        buffer = jsonrpc_respond_ok(buffer, method, request_id);
    } else {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "You need to be playing to crop the playlist", true);
        LOG_ERROR("You need to be playing to crop the playlist");
    }
    
    mpd_status_free(status);
    
    return buffer;
}

sds mpd_client_search_queue(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                            const char *mpdtagtype, const unsigned int offset, const char *searchstr, const t_tags *tagcols)
{
    if (mpd_search_queue_songs(mpd_state->conn, false) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }
    
    if (mpd_tag_name_parse(mpdtagtype) != MPD_TAG_UNKNOWN) {
        if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdtagtype), searchstr) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
    }
    else {
        if (mpd_search_add_any_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, searchstr) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
    }

    if (mpd_search_commit(mpd_state->conn) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }

    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    struct mpd_song *song;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            if (entities_returned++) {
                buffer= sdscat(buffer, ",");
            }
            buffer = sdscat(buffer, "{");
            buffer = tojson_long(buffer, "id", mpd_song_get_id(song), true);
            buffer = tojson_long(buffer, "Pos", mpd_song_get_pos(song), true);
            buffer = put_song_tags(buffer, mpd_state, tagcols, song);
            buffer = sdscat(buffer, "}");
        }
        mpd_song_free(song);
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_char(buffer, "mpdtagtype", mpdtagtype, false);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}
