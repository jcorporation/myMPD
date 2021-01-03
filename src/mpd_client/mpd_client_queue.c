/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"
#include "mpd_client_queue.h"

bool mpd_client_queue_prio_set_highest(t_mpd_client_state *mpd_client_state, const unsigned trackid) {
    //default prio is 10
    int priority = 10;
    
    //try to get prio of next song
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    if (status == NULL) {
        check_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0);
        return false;
    }
    int next_song_id = mpd_status_get_next_song_id(status);
    mpd_status_free(status);
    if (next_song_id > -1 ) {
        bool rc = mpd_send_get_queue_song_id(mpd_client_state->mpd_state->conn, next_song_id);
        if (rc == true) {
            struct mpd_song *song = mpd_recv_song(mpd_client_state->mpd_state->conn);
            if (song != NULL) {
                priority = mpd_song_get_prio(song);
                priority++;
                mpd_song_free(song);
            }
        }
        mpd_response_finish(mpd_client_state->mpd_state->conn);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_get_queue_song_id") == false) {
            return false;
        }
    }
    
    //set priority, priority have only an effect in random mode
    bool rc = mpd_run_prio_id(mpd_client_state->mpd_state->conn, priority, trackid);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_prio_id") == false) {
        return false;
    }
    return true;
}

bool mpd_client_queue_replace_with_song(t_mpd_client_state *mpd_client_state, const char *uri) {
    if (mpd_command_list_begin(mpd_client_state->mpd_state->conn, false)) {
        bool rc = mpd_send_clear(mpd_client_state->mpd_state->conn);
        if (rc == false) {
            LOG_ERROR("Error adding command to command list mpd_send_clear");
        }
        rc = mpd_send_add(mpd_client_state->mpd_state->conn, uri);
        if (rc == false) {
            LOG_ERROR("Error adding command to command list mpd_send_add");
        }
        rc = mpd_send_play(mpd_client_state->mpd_state->conn);
        if (rc == false) {
            LOG_ERROR("Error adding command to command list mpd_send_play");
        }
        if (mpd_command_list_end(mpd_client_state->mpd_state->conn) == true) {
            mpd_response_finish(mpd_client_state->mpd_state->conn);
        }
    }
    if (check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false) == false) {
        return false;
    }
    return true;
}

bool mpd_client_queue_replace_with_playlist(t_mpd_client_state *mpd_client_state, const char *plist) {
    if (mpd_command_list_begin(mpd_client_state->mpd_state->conn, false)) {
        mpd_send_clear(mpd_client_state->mpd_state->conn);
        mpd_send_load(mpd_client_state->mpd_state->conn, plist);
        mpd_send_play(mpd_client_state->mpd_state->conn);
        if (mpd_command_list_end(mpd_client_state->mpd_state->conn) == true) {
            mpd_response_finish(mpd_client_state->mpd_state->conn);
        }
    }
    if (check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false) == false) {
        return false;
    }
    return true;
}

sds mpd_client_get_queue_state(t_mpd_client_state *mpd_client_state, sds buffer) {
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    if (status == NULL) {
        check_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0);
        return buffer;
    }

    mpd_client_state->queue_version = mpd_status_get_queue_version(status);
    mpd_client_state->queue_length = mpd_status_get_queue_length(status);
    mpd_client_state->crossfade = mpd_status_get_crossfade(status);
    mpd_client_state->mpd_state->state = mpd_status_get_state(status);

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

sds mpd_client_put_queue(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id,
                         unsigned int offset, unsigned int limit, const t_tags *tagcols)
{
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    if (status == NULL) {
        buffer = check_error_and_recover(mpd_client_state->mpd_state, buffer, method, request_id);
        return buffer;
    }

    if (offset >= mpd_status_get_queue_length(status)) {
        offset = 0;
    }
    
    if (limit == 0) {
        limit = UINT_MAX - offset;
    }
    bool rc = mpd_send_list_queue_range_meta(mpd_client_state->mpd_state->conn, offset, offset + limit);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_queue_range_meta") == false) {
        return buffer;
    }
        
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    unsigned totalTime = 0;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    struct mpd_song *song;
    while ((song = mpd_recv_song(mpd_client_state->mpd_state->conn)) != NULL) {
        totalTime += mpd_song_get_duration(song);
        entity_count++;
        if (entities_returned++) {
            buffer = sdscat(buffer, ",");
        }
        buffer = sdscat(buffer, "{");
        buffer = tojson_long(buffer, "id", mpd_song_get_id(song), true);
        buffer = tojson_long(buffer, "Pos", mpd_song_get_pos(song), true);
        buffer = put_song_tags(buffer, mpd_client_state->mpd_state, tagcols, song);
        buffer = sdscat(buffer, "}");
        mpd_song_free(song);
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalTime", totalTime, true);
    buffer = tojson_long(buffer, "totalEntities", mpd_status_get_queue_length(status), true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "queueVersion", mpd_status_get_queue_version(status), false);
    buffer = jsonrpc_end_result(buffer);
    
    mpd_client_state->queue_version = mpd_status_get_queue_version(status);
    mpd_client_state->queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);

    mpd_response_finish(mpd_client_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    
    return buffer;
}

sds mpd_client_crop_queue(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, bool or_clear) {
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    if (status == NULL) {
        buffer = check_error_and_recover(mpd_client_state->mpd_state, buffer, method, request_id);
        return buffer;
    }
    const unsigned length = mpd_status_get_queue_length(status) - 1;
    unsigned playing_song_pos = mpd_status_get_song_pos(status);
    enum mpd_state state = mpd_status_get_state(status);

    if ((state == MPD_STATE_PLAY || state == MPD_STATE_PAUSE) && length > 1) {
        playing_song_pos++;
        if (playing_song_pos < length) {
            bool rc = mpd_run_delete_range(mpd_client_state->mpd_state->conn, playing_song_pos, -1);
            if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_run_delete_range") == false) {
                return buffer;
            }
        }
        playing_song_pos--;
        if (playing_song_pos > 0) {
            bool rc = mpd_run_delete_range(mpd_client_state->mpd_state->conn, 0, playing_song_pos--);
            if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_run_delete_range") == false) {
                return buffer;
            }
        }
        buffer = jsonrpc_respond_ok(buffer, method, request_id);
    }
    else if (or_clear == true || state == MPD_STATE_STOP) {
        bool rc = mpd_run_clear(mpd_client_state->mpd_state->conn);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_run_clear") == true) {
            buffer = jsonrpc_respond_ok(buffer, method, request_id);
        }
    }
    else {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Can not crop the queue", true);
        LOG_ERROR("Can not crop the queue");
    }
    
    mpd_status_free(status);
    
    return buffer;
}

sds mpd_client_search_queue(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id,
                            const char *mpdtagtype, const unsigned int offset, const unsigned int limit, const char *searchstr, const t_tags *tagcols)
{
    bool rc = mpd_search_queue_songs(mpd_client_state->mpd_state->conn, false);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_queue_songs") == false) {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        return buffer;
    }
    if (mpd_tag_name_parse(mpdtagtype) != MPD_TAG_UNKNOWN) {
        rc = mpd_search_add_tag_constraint(mpd_client_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdtagtype), searchstr);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_tag_constraint") == false) {
            mpd_search_cancel(mpd_client_state->mpd_state->conn);
            return buffer;
        }        
    }
    else {
        rc = mpd_search_add_any_tag_constraint(mpd_client_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, searchstr);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_any_tag_constraint") == false) {
            mpd_search_cancel(mpd_client_state->mpd_state->conn);
            return buffer;
        }
    }

    rc = mpd_search_commit(mpd_client_state->mpd_state->conn);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_commit") == false) {
        return buffer;
    }
    
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    struct mpd_song *song;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    while ((song = mpd_recv_song(mpd_client_state->mpd_state->conn)) != NULL) {
        entity_count++;
        if (entity_count > offset && (entity_count <= offset + limit || limit == 0)) {
            if (entities_returned++) {
                buffer= sdscat(buffer, ",");
            }
            buffer = sdscat(buffer, "{");
            buffer = tojson_long(buffer, "id", mpd_song_get_id(song), true);
            buffer = tojson_long(buffer, "Pos", mpd_song_get_pos(song), true);
            buffer = put_song_tags(buffer, mpd_client_state->mpd_state, tagcols, song);
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
    
    mpd_response_finish(mpd_client_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    
    return buffer;
}
