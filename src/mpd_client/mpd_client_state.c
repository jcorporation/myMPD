/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>

#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "mympd_config_defs.h"
#include "../lua_mympd_state.h"
#include "../utility.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared/mpd_shared_sticker.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"
#include "mpd_client_cover.h"
#include "mpd_client_api.h"
#include "mpd_client_sticker.h"
#include "mpd_client_state.h"

//private definitions
static sds _mpd_client_put_outputs(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id);
static int _mpd_client_get_volume(t_mpd_client_state *mpd_client_state);

//public functions
sds mpd_client_get_updatedb_state(t_mpd_client_state *mpd_client_state, sds buffer) {
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    if (status == NULL) {
        buffer = check_error_and_recover_notify(mpd_client_state->mpd_state, buffer);
        return buffer;
    }
    unsigned update_id = mpd_status_get_update_id(status);
    MYMPD_LOG_NOTICE("Update database ID: %u", update_id);
    if (update_id > 0) {
        buffer = jsonrpc_notify_start(buffer, "update_started");
        buffer = tojson_long(buffer, "jobid", update_id, false);
        buffer = jsonrpc_result_end(buffer);
    }
    else {
        buffer = jsonrpc_event(buffer, "update_finished");
    }
    mpd_status_free(status);    
    return buffer;    
}

sds mpd_client_put_state(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id) {
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
        buffer = jsonrpc_notify_start(buffer, "update_state");
    }
    else {
        buffer = jsonrpc_result_start(buffer, method, request_id);
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
    if (mpd_client_state->feat_mpd_partitions == true) {
        buffer = tojson_char(buffer, "partition", mpd_status_get_partition(status), true);
    }
    buffer = sdscat(buffer, "\"audioFormat\":{");
    buffer = tojson_long(buffer, "sampleRate", (audioformat ? audioformat->sample_rate : 0), true);
    buffer = tojson_long(buffer, "bits", (audioformat ? audioformat->bits : 0), true);
    buffer = tojson_long(buffer, "channels", (audioformat ? audioformat->channels : 0), false);
    buffer = sdscat(buffer, "}");
    buffer = jsonrpc_result_end(buffer);
    
    mpd_status_free(status);
    return buffer;
}

bool mpd_client_get_lua_mympd_state(t_config *config, t_mpd_client_state *mpd_client_state, struct list *lua_mympd_state) {
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    if (status == NULL) {
        return false;
    }
    set_lua_mympd_state_i(lua_mympd_state, "play_state", mpd_status_get_state(status));
    set_lua_mympd_state_i(lua_mympd_state, "volume", mpd_status_get_volume(status));
    set_lua_mympd_state_i(lua_mympd_state, "song_pos", mpd_status_get_song_pos(status));
    set_lua_mympd_state_i(lua_mympd_state, "elapsed_time", mpd_status_get_elapsed_time(status));
    set_lua_mympd_state_i(lua_mympd_state, "total_time", mpd_status_get_total_time(status));
    set_lua_mympd_state_i(lua_mympd_state, "song_id", mpd_status_get_song_id(status));
    set_lua_mympd_state_i(lua_mympd_state, "next_song_id", mpd_status_get_next_song_id(status));
    set_lua_mympd_state_i(lua_mympd_state, "next_song_pos", mpd_status_get_next_song_pos(status));
    set_lua_mympd_state_i(lua_mympd_state, "queue_length", mpd_status_get_queue_length(status));
    set_lua_mympd_state_i(lua_mympd_state, "queue_version", mpd_status_get_queue_version(status));
    set_lua_mympd_state_b(lua_mympd_state, "repeat", mpd_status_get_repeat(status));
    set_lua_mympd_state_b(lua_mympd_state, "random", mpd_status_get_random(status));
    set_lua_mympd_state_i(lua_mympd_state, "single_state", mpd_status_get_single_state(status));
    set_lua_mympd_state_i(lua_mympd_state, "consume", mpd_status_get_consume(status));
    set_lua_mympd_state_i(lua_mympd_state, "crossfade", mpd_status_get_crossfade(status));
    set_lua_mympd_state_f(lua_mympd_state, "mixrampdb", mpd_status_get_mixrampdb(status));
    set_lua_mympd_state_f(lua_mympd_state, "mixrampdelay", mpd_status_get_mixrampdelay(status));
    set_lua_mympd_state_p(lua_mympd_state, "music_directory", mpd_client_state->music_directory_value);
    set_lua_mympd_state_p(lua_mympd_state, "varlibdir", config->varlibdir);
    set_lua_mympd_state_i(lua_mympd_state, "jukebox_mode", mpd_client_state->jukebox_mode);
    set_lua_mympd_state_p(lua_mympd_state, "jukebox_playlist", mpd_client_state->jukebox_playlist);
    set_lua_mympd_state_i(lua_mympd_state, "jukebox_queue_length", mpd_client_state->jukebox_queue_length);
    set_lua_mympd_state_i(lua_mympd_state, "jukebox_last_played", mpd_client_state->jukebox_last_played);
    if (mpd_client_state->feat_mpd_partitions == true) {
        set_lua_mympd_state_p(lua_mympd_state, "partition", mpd_status_get_partition(status));
    }
    mpd_status_free(status);
    return true;
}

sds mpd_client_put_volume(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id) {
    int volume = _mpd_client_get_volume(mpd_client_state);
    if (method == NULL) {
        buffer = jsonrpc_notify_start(buffer, "update_volume");
    }
    else {
        buffer = jsonrpc_result_start(buffer, method, request_id);
    }
    buffer = tojson_long(buffer, "volume", volume, false);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

sds mpd_client_put_partition_outputs(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id,
                                     const char *partition)
{
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    const char *oldpartition = mpd_status_get_partition(status);
    bool rc = mpd_run_switch_partition(mpd_client_state->mpd_state->conn, partition);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_run_switch_partition") == false) {
        mpd_status_free(status);
        return buffer;
    }

    buffer = _mpd_client_put_outputs(mpd_client_state, buffer, method, request_id);
    
    rc = mpd_run_switch_partition(mpd_client_state->mpd_state->conn, oldpartition);
    check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_run_switch_partition");
    mpd_status_free(status);
    return buffer;
}

sds mpd_client_put_outputs(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id) {
    return _mpd_client_put_outputs(mpd_client_state, buffer, method, request_id);
}

sds mpd_client_put_current_song(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id) {
    struct mpd_song *song = mpd_run_current_song(mpd_client_state->mpd_state->conn);
    if (song == NULL) {
        if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
            return buffer;
        }
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "player", "info", "No current song");
        return buffer;
    }
    
    const char *uri = mpd_song_get_uri(song);

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = tojson_long(buffer, "pos", mpd_song_get_pos(song), true);
    buffer = tojson_long(buffer, "currentSongId", mpd_client_state->song_id, true);
    buffer = tojson_long(buffer, "startTime", mpd_client_state->song_start_time, true);
    buffer = put_song_tags(buffer, mpd_client_state->mpd_state, &mpd_client_state->mpd_state->mympd_tag_types, song);

    if (mpd_client_state->mpd_state->feat_stickers && mpd_client_state->sticker_cache != NULL) {
        buffer = sdscat(buffer, ",");
        buffer = mpd_shared_sticker_list(buffer, mpd_client_state->sticker_cache, mpd_song_get_uri(song));
    }

    buffer = sdscat(buffer, ",");
    buffer = put_extra_files(mpd_client_state, buffer, uri, false);
    
    mpd_song_free(song);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

//private functions
static sds _mpd_client_put_outputs(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id) {
    bool rc = mpd_send_outputs(mpd_client_state->mpd_state->conn);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_outputs") == false) {
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int nr = 0;
    struct mpd_output *output;
    while ((output = mpd_recv_output(mpd_client_state->mpd_state->conn)) != NULL) {
        if (nr++) {
            buffer = sdscat(buffer, ",");
        }
        buffer = sdscat(buffer, "{");
        buffer = tojson_long(buffer, "id", mpd_output_get_id(output), true);
        buffer = tojson_char(buffer, "name", mpd_output_get_name(output), true);
        buffer = tojson_long(buffer, "state", mpd_output_get_enabled(output), true);
        buffer = tojson_char(buffer, "plugin", mpd_output_get_plugin(output), true);
        buffer = sdscat(buffer,  "\"attributes\":{");
        const struct mpd_pair *attributes = mpd_output_first_attribute(output);
        if (attributes != NULL) {
            buffer = tojson_char(buffer, attributes->name, attributes->value, false);
            while ((attributes = mpd_output_next_attribute(output)) != NULL) {
                buffer = sdscat(buffer, ",");
                buffer = tojson_char(buffer, attributes->name, attributes->value, false);
            }
        }
        buffer = sdscat(buffer, "}}");
        mpd_output_free(output);
    }
    mpd_response_finish(mpd_client_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "numOutputs", nr, false);
    buffer = jsonrpc_result_end(buffer);
    
    return buffer;
}

static int _mpd_client_get_volume(t_mpd_client_state *mpd_client_state) {
    int volume = -1;
    if (mpd_connection_cmp_server_version(mpd_client_state->mpd_state->conn, 0, 23, 0) >= 0) {
        volume = mpd_run_get_volume(mpd_client_state->mpd_state->conn);
        check_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0);
    }
    else {
        struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
        if (status == NULL) {
            check_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0);
            return -1;
        }
        volume = mpd_status_get_volume(status);
        mpd_status_free(status);
    }
    return volume;
}
