/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../lua_mympd_state.h"
#include "../utility.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"
#include "mpd_client_cover.h"
#include "mpd_client_api.h"
#include "mpd_client_sticker.h"
#include "mpd_client_state.h"

sds mpd_client_get_updatedb_state(t_mpd_client_state *mpd_client_state, sds buffer) {
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    if (status == NULL) {
        buffer = check_error_and_recover_notify(mpd_client_state->mpd_state, buffer);
    }
    unsigned update_id = mpd_status_get_update_id(status);
    LOG_INFO("Update database ID: %u", update_id);
    if ( update_id > 0) {
        buffer = jsonrpc_start_notify(buffer, "update_started");
        buffer = tojson_long(buffer, "jobid", update_id, false);
        buffer = jsonrpc_end_notify(buffer);
    }
    else {
        buffer = jsonrpc_start_notify(buffer, "update_finished");
        buffer = jsonrpc_end_notify(buffer);
    }
    mpd_status_free(status);    
    return buffer;    
}

sds mpd_client_put_state(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id) {
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    if (status == NULL) {
        if (method == NULL) {
            buffer = check_error_and_recover_notify(mpd_client_state->mpd_state, buffer);
        }
        else {
            buffer = check_error_and_recover(mpd_client_state->mpd_state, buffer, method, request_id);
        }
        return buffer;
    }

    int song_id = mpd_status_get_song_id(status);
    if (mpd_client_state->song_id != song_id) {
        mpd_client_state->last_song_id = mpd_client_state->song_id;
        mpd_client_state->last_song_end_time = mpd_client_state->song_end_time;
        mpd_client_state->last_song_start_time = mpd_client_state->song_start_time;
        struct mpd_song *song = mpd_run_current_song(mpd_client_state->mpd_state->conn);
        if (song != NULL) {
            mpd_client_state->last_song_uri = sdsreplace(mpd_client_state->last_song_uri, mpd_client_state->song_uri);
            mpd_client_state->song_uri = sdsreplace(mpd_client_state->song_uri, mpd_song_get_uri(song));
            mpd_song_free(song);
        }
        else {
            mpd_client_state->song_uri = sdscrop(mpd_client_state->song_uri);
        }
        mpd_response_finish(mpd_client_state->mpd_state->conn);
    }

    mpd_client_state->mpd_state->state = mpd_status_get_state(status);
    mpd_client_state->song_id = song_id;
    mpd_client_state->next_song_id = mpd_status_get_next_song_id(status);
    mpd_client_state->queue_version = mpd_status_get_queue_version(status);
    mpd_client_state->queue_length = mpd_status_get_queue_length(status);
    mpd_client_state->crossfade = mpd_status_get_crossfade(status);

    const unsigned total_time = mpd_status_get_total_time(status);
    const unsigned elapsed_time =  mpd_status_get_elapsed_time(status);
    unsigned uptime = time(NULL) - config->startup_time;
    if (total_time > 10 && uptime > elapsed_time) {
        time_t now = time(NULL);
        mpd_client_state->song_end_time = now + total_time - elapsed_time - 10;
        mpd_client_state->song_start_time = now - elapsed_time;
        unsigned half_time = total_time / 2;
        
        if (half_time > 240) {
            mpd_client_state->set_song_played_time = now - elapsed_time + 240;
        }
        else {
            mpd_client_state->set_song_played_time = elapsed_time < half_time ? now - (long)elapsed_time + (long)half_time : now;
        }
    }
    else {
        //don't track songs with length < 10s
        mpd_client_state->song_end_time = 0;
        mpd_client_state->song_start_time = 0;
        mpd_client_state->set_song_played_time = 0;
    }
    
    if (method == NULL) {
        buffer = jsonrpc_start_notify(buffer, "update_state");
    }
    else {
        buffer = jsonrpc_start_result(buffer, method, request_id);
        buffer = sdscat(buffer, ",");
    }
    const struct mpd_audio_format *audioformat = mpd_status_get_audio_format(status);
    buffer = tojson_long(buffer, "state", mpd_status_get_state(status), true);
    buffer = tojson_long(buffer, "volume", mpd_status_get_volume(status), true);
    buffer = tojson_long(buffer, "songPos", mpd_status_get_song_pos(status), true);
    buffer = tojson_long(buffer, "elapsedTime", mpd_status_get_elapsed_time(status), true);
    buffer = tojson_long(buffer, "totalTime", mpd_status_get_total_time(status), true);
    buffer = tojson_long(buffer, "currentSongId", mpd_status_get_song_id(status), true);
    buffer = tojson_long(buffer, "kbitrate", mpd_status_get_kbit_rate(status), true);
    buffer = tojson_long(buffer, "queueLength", mpd_status_get_queue_length(status), true);
    buffer = tojson_long(buffer, "queueVersion", mpd_status_get_queue_version(status), true);
    buffer = tojson_long(buffer, "nextSongPos", mpd_status_get_next_song_pos(status), true);
    buffer = tojson_long(buffer, "nextSongId", mpd_status_get_next_song_id(status), true);
    buffer = tojson_long(buffer, "lastSongId", (mpd_client_state->last_song_id ? mpd_client_state->last_song_id : -1), true);
    buffer = sdscat(buffer, "\"audioFormat\":{");
    buffer = tojson_long(buffer, "sampleRate", (audioformat ? audioformat->sample_rate : 0), true);
    buffer = tojson_long(buffer, "bits", (audioformat ? audioformat->bits : 0), true);
    buffer = tojson_long(buffer, "channels", (audioformat ? audioformat->channels : 0), false);
    buffer = sdscat(buffer, "}");
    if (method == NULL) {
        buffer = jsonrpc_end_notify(buffer);
    }
    else {
        buffer = jsonrpc_end_result(buffer);
    }
    mpd_status_free(status);
    return buffer;
}

bool mpd_client_get_lua_mympd_state(t_config *config, t_mpd_client_state *mpd_client_state, t_lua_mympd_state *lua_mympd_state) {
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    if (status == NULL) {
        return false;
    }
    lua_mympd_state->play_state = mpd_status_get_state(status);
    lua_mympd_state->volume = mpd_status_get_volume(status);
    lua_mympd_state->song_pos = mpd_status_get_song_pos(status);
    lua_mympd_state->elapsed_time = mpd_status_get_elapsed_time(status);
    lua_mympd_state->total_time = mpd_status_get_total_time(status);
    lua_mympd_state->song_id = mpd_status_get_song_id(status);
    lua_mympd_state->next_song_id = mpd_status_get_next_song_id(status);
    lua_mympd_state->next_song_pos = mpd_status_get_next_song_pos(status);
    lua_mympd_state->queue_length = mpd_status_get_queue_length(status);
    lua_mympd_state->queue_version = mpd_status_get_queue_version(status);
    lua_mympd_state->repeat = mpd_status_get_repeat(status);
    lua_mympd_state->random = mpd_status_get_random(status);
    lua_mympd_state->single_state = mpd_status_get_single_state(status);
    lua_mympd_state->consume = mpd_status_get_consume(status);
    lua_mympd_state->crossfade = mpd_status_get_crossfade(status);
    lua_mympd_state->mixrampdb = mpd_status_get_mixrampdb(status);
    lua_mympd_state->mixrampdelay = mpd_status_get_mixrampdelay(status);
    lua_mympd_state->music_directory = sdsnew(mpd_client_state->music_directory_value);
    lua_mympd_state->varlibdir = sdsdup(config->varlibdir);
    mpd_status_free(status);
    return true;
}

sds mpd_client_put_volume(t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id) {
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    if (status == NULL) {
        if (method == NULL) {
            buffer = check_error_and_recover_notify(mpd_client_state->mpd_state, buffer);
        }
        else {
            buffer = check_error_and_recover(mpd_client_state->mpd_state, buffer, method, request_id);
        }
        return buffer;
    }
    if (method == NULL) {
        buffer = jsonrpc_start_notify(buffer, "update_volume");
    }
    else {
        buffer = jsonrpc_start_result(buffer, method, request_id);
        buffer = sdscat(buffer, ",");
    }
    buffer = tojson_long(buffer, "volume", mpd_status_get_volume(status), false);
    if (method == NULL) {
        buffer = jsonrpc_end_notify(buffer);
    }
    else {
        buffer = jsonrpc_end_result(buffer);
    }
    mpd_status_free(status);

    return buffer;
}

sds mpd_client_put_outputs(t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id) {
    bool rc = mpd_send_outputs(mpd_client_state->mpd_state->conn);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_outputs") == false) {
        return buffer;
    }

    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    int nr = 0;
    struct mpd_output *output;
    while ((output = mpd_recv_output(mpd_client_state->mpd_state->conn)) != NULL) {
        if (nr++) {
            buffer = sdscat(buffer, ",");
        }
        buffer = sdscat(buffer, "{");
        buffer = tojson_long(buffer, "id", mpd_output_get_id(output), true);
        buffer = tojson_char(buffer, "name", mpd_output_get_name(output), true);
        buffer = tojson_long(buffer, "state", mpd_output_get_enabled(output), false);
        buffer = sdscat(buffer, "}");
        mpd_output_free(output);
    }
    mpd_response_finish(mpd_client_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "numOutputs", nr, false);
    buffer = jsonrpc_end_result(buffer);
    
    return buffer;
}

sds mpd_client_put_current_song(t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id) {
    struct mpd_song *song = mpd_run_current_song(mpd_client_state->mpd_state->conn);
    if (song == NULL) {
        if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
            return buffer;
        }
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No current song", false);
        return buffer;
    }
    
    const char *uri = mpd_song_get_uri(song);

    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",");
    buffer = tojson_long(buffer, "pos", mpd_song_get_pos(song), true);
    buffer = tojson_long(buffer, "currentSongId", mpd_client_state->song_id, true);
    buffer = put_song_tags(buffer, mpd_client_state->mpd_state, &mpd_client_state->mpd_state->mympd_tag_types, song);

    if (mpd_client_state->feat_sticker) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        assert(sticker);
        mpd_client_get_sticker(mpd_client_state, uri, sticker);
        buffer = sdscat(buffer, ",");
        buffer = tojson_long(buffer, "playCount", sticker->playCount, true);
        buffer = tojson_long(buffer, "skipCount", sticker->skipCount, true);
        buffer = tojson_long(buffer, "like", sticker->like, true);
        buffer = tojson_long(buffer, "lastPlayed", sticker->lastPlayed, true);
        buffer = tojson_long(buffer, "lastSkipped", sticker->lastSkipped, false);
        FREE_PTR(sticker);
    }

    //waits for further implementation in 7.1.0 release
    //buffer = sdscat(buffer, ",");
    //buffer = put_extra_files(mpd_client_state, buffer, uri);
    
    mpd_song_free(song);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}
