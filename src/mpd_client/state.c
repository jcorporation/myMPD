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
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <poll.h>
#include <dirent.h>
#include <pthread.h>
#include <mpd/client.h>
#include <inttypes.h>

#include "../dist/src/sds/sds.h"
#include "utility.h"
#include "api.h"
#include "log.h"
#include "list.h"
#include "config_defs.h"
#include "tiny_queue.h"
#include "mpd_client_api.h"
#include "../dist/src/frozen/frozen.h"
#include "../dist/src/sds/sds.h"

int mpd_client_get_updatedb_state(t_mpd_state *mpd_state, char *buffer) {
    struct mpd_status *status;
    int len, update_id;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    status = mpd_run_status(mpd_state->conn);
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }
    update_id = mpd_status_get_update_id(status);
    LOG_INFO("Update database ID: %d", update_id);
    if ( update_id > 0) {
        len = json_printf(&out, "{type: update_started, data: {jobid: %d}}", update_id);
    }
    else {
        len = json_printf(&out, "{type: update_finished}");
    }
    mpd_status_free(status);    
    
    CHECK_RETURN_LEN();
}

int mpd_client_get_state(t_mpd_state *mpd_state, char *buffer) {
    int len = 0;
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        LOG_ERROR_AND_RECOVER("mpd_run_status");
        return -1;
    }

    int song_id = mpd_status_get_song_id(status);
    if (mpd_state->song_id != song_id) {
        mpd_state->last_song_id = mpd_state->song_id;
        mpd_state->last_song_end_time = mpd_state->song_end_time;
        mpd_state->last_song_start_time = mpd_state->song_start_time;
        struct mpd_song *song = mpd_run_current_song(mpd_state->conn);
        if (song != NULL) {
            FREE_PTR(mpd_state->last_song_uri);
            if (mpd_state->song_uri != NULL) {
                mpd_state->last_song_uri = mpd_state->song_uri;
            }
            mpd_state->song_uri = strdup(mpd_song_get_uri(song));
            mpd_song_free(song);
        }
        else {
            FREE_PTR(mpd_state->song_uri);
        }
        mpd_response_finish(mpd_state->conn);
    }

    mpd_state->state = mpd_status_get_state(status);
    mpd_state->song_id = song_id;
    mpd_state->next_song_id = mpd_status_get_next_song_id(status);
    mpd_state->queue_version = mpd_status_get_queue_version(status);
    mpd_state->queue_length = mpd_status_get_queue_length(status);
    mpd_state->crossfade = mpd_status_get_crossfade(status);

    const int total_time = mpd_status_get_total_time(status);
    const int elapsed_time =  mpd_status_get_elapsed_time(status);
    if (total_time > 10) {
        time_t now = time(NULL);
        mpd_state->song_end_time = now + total_time - elapsed_time - 10;
        mpd_state->song_start_time = now - elapsed_time;
        int half_time = total_time / 2;
        
        if (half_time > 240) {
            mpd_state->set_song_played_time = now - elapsed_time + 240;
        }
        else {
            mpd_state->set_song_played_time = elapsed_time < half_time ? now - elapsed_time + half_time : now;
        }
    }
    else {
        //don't track songs with length < 10s
        mpd_state->song_end_time = 0;
        mpd_state->song_start_time = 0;
        mpd_state->set_song_played_time = 0;
    }
    
    if (buffer != NULL) {
        len = mpd_client_put_state(mpd_state, status, buffer);
    }
    mpd_status_free(status);
    return len;
}

int mpd_client_put_state(t_mpd_state *mpd_state, struct mpd_status *status, char *buffer) {
    int len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    const struct mpd_audio_format *audioformat = mpd_status_get_audio_format(status);

    len = json_printf(&out, "{type: update_state, data: {state: %d, volume: %d, songPos: %d, elapsedTime: %d, "
        "totalTime: %d, currentSongId: %d, kbitrate: %d, audioFormat: { sampleRate: %d, bits: %d, channels: %d}, "
        "queueLength: %d, nextSongPos: %d, nextSongId: %d, lastSongId: %d, queueVersion: %d}}", 
        mpd_status_get_state(status),
        mpd_status_get_volume(status), 
        mpd_status_get_song_pos(status),
        mpd_status_get_elapsed_time(status),
        mpd_status_get_total_time(status),
        mpd_status_get_song_id(status),
        mpd_status_get_kbit_rate(status),
        audioformat ? audioformat->sample_rate : 0, 
        audioformat ? audioformat->bits : 0, 
        audioformat ? audioformat->channels : 0,
        mpd_status_get_queue_length(status),
        mpd_status_get_next_song_pos(status),
        mpd_status_get_next_song_id(status),
        mpd_state->last_song_id ? mpd_state->last_song_id : -1,
        mpd_status_get_queue_version(status)
    );
   
    CHECK_RETURN_LEN();
}

int mpd_client_put_volume(t_mpd_state *mpd_state, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }
    len = json_printf(&out, "{type: update_volume, data: {volume: %d}}",
        mpd_status_get_volume(status)
    );
    mpd_status_free(status);

    CHECK_RETURN_LEN();
}

static int mpd_client_put_settings(t_mpd_state *mpd_state, char *buffer) {
    char *replaygain = NULL;
    size_t len, nr;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }

    if (!mpd_send_command(mpd_state->conn, "replay_gain_status", NULL)) {
        RETURN_ERROR_AND_RECOVER("replay_gain_status");
    }
    struct mpd_pair *pair = mpd_recv_pair(mpd_state->conn);
    if (!pair) {
        RETURN_ERROR_AND_RECOVER("replay_gain_status");
    }
    replaygain = strdup(pair->value);
    mpd_return_pair(mpd_state->conn, pair);
    mpd_response_finish(mpd_state->conn);
    
    len = json_printf(&out, "{type: settings, data: {"
        "repeat: %d, single: %d, crossfade: %d, consume: %d, random: %d, "
        "mixrampdb: %f, mixrampdelay: %f, replaygain: %Q, featPlaylists: %B,"
        "featTags: %B, featLibrary: %B, featAdvsearch: %B, featStickers: %B,"
        "featSmartpls: %B, featLove: %B, featCoverimage: %B, featFingerprint: %B, "
        "musicDirectoryValue: %Q, mpdConnected: %B, tags: [", 
        mpd_status_get_repeat(status),
        mpd_status_get_single(status),
        mpd_status_get_crossfade(status),
        mpd_status_get_consume(status),
        mpd_status_get_random(status),
        mpd_status_get_mixrampdb(status),
        mpd_status_get_mixrampdelay(status),
        replaygain == NULL ? "" : replaygain,
        mpd_state->feat_playlists,
        mpd_state->feat_tags,
        mpd_state->feat_library,
        mpd_state->feat_advsearch,
        mpd_state->feat_sticker,
        mpd_state->feat_smartpls,
        mpd_state->feat_love,
        mpd_state->feat_coverimage,
        mpd_state->feat_fingerprint,
        mpd_state->music_directory_value,
        true
    );
    mpd_status_free(status);
    FREE_PTR(replaygain);
    
    for (nr = 0; nr < mpd_state->mympd_tag_types->len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->mympd_tag_types->tags[nr]));
    }
    
    len += json_printf(&out, "], searchtags: [");
        for (nr = 0; nr < mpd_state->search_tag_types->len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->search_tag_types->tags[nr]));
    }
    
    len += json_printf(&out, "], browsetags: [");
    for (nr = 0; nr < mpd_state->browse_tag_types->len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->browse_tag_types->tags[nr]));
    }

    len += json_printf(&out, "], allmpdtags: [");
    for (nr = 0; nr < mpd_state->mpd_tag_types->len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->mpd_tag_types->tags[nr]));
    }

    len += json_printf(&out, "]}}");
    
    CHECK_RETURN_LEN();
}

int mpd_client_put_outputs(t_mpd_state *mpd_state, char *buffer) {
    struct mpd_output *output;
    size_t len;
    int nr;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    if (!mpd_send_outputs(mpd_state->conn)) {
        RETURN_ERROR_AND_RECOVER("outputs");
    }

    len = json_printf(&out, "{type: outputs, data: {outputs: [");
    nr = 0;    
    while ((output = mpd_recv_output(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
        if (nr++) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "{id: %d, name: %Q, state: %d}",
            mpd_output_get_id(output),
            mpd_output_get_name(output),
            mpd_output_get_enabled(output)
        );
        mpd_output_free(output);
    }
    if (!mpd_response_finish(mpd_state->conn))
        LOG_ERROR_AND_RECOVER("outputs");

    len += json_printf(&out,"]}}");
    
    CHECK_RETURN_LEN();
}

int mpd_client_put_current_song(t_config *config, t_mpd_state *mpd_state, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    struct mpd_song *song = mpd_run_current_song(mpd_state->conn);
    if (song == NULL) {
        len = json_printf(&out, "{type: result, data: ok}");
        return len;
    }
    
    len = json_printf(&out, "{type: song_change, data: {pos: %d, currentSongId: %d, ",
        mpd_song_get_pos(song),
        mpd_state->song_id
    );
    PUT_SONG_TAG_ALL();

    mpd_response_finish(mpd_state->conn);
    
    sds cover = sdsempty();
    cover = mpd_client_get_cover(config, mpd_state, mpd_song_get_uri(song), cover);
    len += json_printf(&out, ", cover: %Q", cover);
    
    if (mpd_state->feat_sticker) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        assert(sticker);
        mpd_client_get_sticker(mpd_state, mpd_song_get_uri(song), sticker);
        len += json_printf(&out, ", playCount: %d, skipCount: %d, like: %d, lastPlayed: %d, lastSkipped: %d",
            sticker->playCount,
            sticker->skipCount,
            sticker->like,
            sticker->lastPlayed,
            sticker->lastSkipped
        );
        FREE_PTR(sticker);
    }
    len += json_printf(&out, "}}");
    mpd_song_free(song);
    sdsfree(cover);

    CHECK_RETURN_LEN();
}
