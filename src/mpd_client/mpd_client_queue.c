/* myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de> This project's
   homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../utility.h"
#include "../log.h"
#include "../config_defs.h"
#include "mpd_client_queue.h"
#include "../dist/src/frozen/frozen.h"

int mpd_client_get_queue_state(t_mpd_state *mpd_state, char *buffer) {
    int len = 0;
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        LOG_ERROR_AND_RECOVER("mpd_run_status");
        return -1;
    }

    mpd_state->queue_version = mpd_status_get_queue_version(status);
    mpd_state->queue_length = mpd_status_get_queue_length(status);
    mpd_state->crossfade = mpd_status_get_crossfade(status);
    mpd_state->state = mpd_status_get_state(status);

    if (buffer != NULL) {
        len = mpd_client_put_queue_state(status, buffer);
    }
    mpd_status_free(status);
    return len;
}

int mpd_client_put_queue_state(struct mpd_status *status, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    len = json_printf(&out, "{type: update_queue, data: {state: %d, queueLength: %d, queueVersion: %d, songPos: %d, nextSongPos: %d}}", 
        mpd_status_get_state(status),
        mpd_status_get_queue_length(status),
        mpd_status_get_queue_version(status),
        mpd_status_get_song_pos(status),
        mpd_status_get_next_song_pos(status)
    );
   
    CHECK_RETURN_LEN();
}

int mpd_client_put_queue(t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const t_tags *tagcols) {
    struct mpd_entity *entity;
    int totalTime = 0;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    size_t len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }
        
    if (!mpd_send_list_queue_range_meta(mpd_state->conn, offset, offset + mpd_state->max_elements_per_page)) {
        RETURN_ERROR_AND_RECOVER("mpd_send_list_queue_meta");
    }
        
    len = json_printf(&out, "{type: queue, data: [");

    while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
        const struct mpd_song *song;
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            song = mpd_entity_get_song(entity);
            totalTime += mpd_song_get_duration(song);
            entity_count++;
            if (entities_returned++) 
                len += json_printf(&out, ",");
            len += json_printf(&out, "{id: %d, Pos: %d, ",
                mpd_song_get_id(song),
                mpd_song_get_pos(song)
            );
            PUT_SONG_TAG_COLS(tagcols);
            len += json_printf(&out, "}");
        }
        mpd_entity_free(entity);
    }

    len += json_printf(&out, "], totalTime: %d, totalEntities: %d, offset: %d, returnedEntities: %d, queue_version: %d}",
        totalTime,
        mpd_status_get_queue_length(status),
        offset,
        entities_returned,
        mpd_status_get_queue_version(status)
    );

    mpd_state->queue_version = mpd_status_get_queue_version(status);
    mpd_state->queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);
    
    CHECK_RETURN_LEN();
}

int mpd_client_queue_crop(t_mpd_state *mpd_state, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    const int length = mpd_status_get_queue_length(status) - 1;
    int playing_song_pos = mpd_status_get_song_pos(status);

    if (length < 1) {
        len = json_printf(&out, "{type: error, data: %Q}", "A queue longer than 1 song in length is required to crop");
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
        len = json_printf(&out, "{type: result, data: ok}");
    } else {
        len = json_printf(&out, "{type: error, data: %Q}", "You need to be playing to crop the playlist");
        LOG_ERROR("You need to be playing to crop the playlist");
    }
    
    mpd_status_free(status);
    
    CHECK_RETURN_LEN();
}

int mpd_client_search_queue(t_mpd_state *mpd_state, char *buffer, const char *mpdtagtype, const unsigned int offset, const char *searchstr, const t_tags *tagcols) {
    struct mpd_song *song;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_queue_songs(mpd_state->conn, false) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_search_queue_songs");
    }
    
    if (mpd_tag_name_parse(mpdtagtype) != MPD_TAG_UNKNOWN) {
       if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdtagtype), searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
    }
    else {
        if (mpd_search_add_any_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_any_tag_constraint");        
    }

    if (mpd_search_commit(mpd_state->conn) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    }
    else {
        len = json_printf(&out, "{type: queuesearch, data: [");

        while ((song = mpd_recv_song(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
                if (entities_returned++)
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{type: song, id: %d, Pos: %d, ",
                    mpd_song_get_id(song),
                    mpd_song_get_pos(song)
                );
                PUT_SONG_TAG_COLS(tagcols);
                len += json_printf(&out, "}");
                mpd_song_free(song);
            }
        }
        
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, mpdtagtype: %Q}",
            entity_count,
            offset,
            entities_returned,
            mpdtagtype
        );
    }

    CHECK_RETURN_LEN();
}
