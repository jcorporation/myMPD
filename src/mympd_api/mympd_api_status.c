/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_status.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/lua_mympd_state.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../mpd_client/mpd_client_errorhandler.h"
#include "../mpd_client/mpd_client_volume.h"
#include "../mpd_client/mpd_client_tags.h"
#include "mympd_api_extra_media.h"
#include "mympd_api_sticker.h"
#include "mympd_api_webradios.h"

//private definitions
static time_t _get_current_song_start_time(struct t_mpd_state *mpd_state);
static const char *_get_playstate_name(enum mpd_state play_state);

static const char *playstate_names[] = {
    [MPD_STATE_UNKNOWN] = "unknown",
    [MPD_STATE_STOP] = "stop",
    [MPD_STATE_PLAY] = "play",
    [MPD_STATE_PAUSE] = "pause"
};

//public functions

//replacement for deprecated mpd_status_get_elapsed_time
unsigned mympd_api_get_elapsed_seconds(struct mpd_status *status) {
    return mpd_status_get_elapsed_ms(status) / 1000;
}

sds mympd_api_status_print(struct t_mpd_state *mpd_state, sds buffer, struct mpd_status *status) {
    enum mpd_state playstate = mpd_status_get_state(status);

    buffer = tojson_char(buffer, "state", _get_playstate_name(playstate), true);
    buffer = tojson_long(buffer, "volume", mpd_status_get_volume(status), true);
    buffer = tojson_long(buffer, "songPos", mpd_status_get_song_pos(status), true);
    buffer = tojson_uint(buffer, "elapsedTime", mympd_api_get_elapsed_seconds(status), true);
    buffer = tojson_uint(buffer, "totalTime", mpd_status_get_total_time(status), true);
    buffer = tojson_long(buffer, "currentSongId", mpd_status_get_song_id(status), true);
    buffer = tojson_uint(buffer, "kbitrate", mpd_status_get_kbit_rate(status), true);
    buffer = tojson_uint(buffer, "queueLength", mpd_status_get_queue_length(status), true);
    buffer = tojson_uint(buffer, "queueVersion", mpd_status_get_queue_version(status), true);
    buffer = tojson_long(buffer, "nextSongPos", mpd_status_get_next_song_pos(status), true);
    buffer = tojson_long(buffer, "nextSongId", mpd_status_get_next_song_id(status), true);
    buffer = tojson_long(buffer, "lastSongId", (mpd_state->last_song_id ?
        mpd_state->last_song_id : -1), true);
    if (mpd_state->feat_mpd_partitions == true) {
        buffer = tojson_char(buffer, "partition", mpd_status_get_partition(status), true);
    }
    const struct mpd_audio_format *audioformat = mpd_status_get_audio_format(status);
    buffer = printAudioFormat(buffer, audioformat);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_char(buffer, "lastError", mpd_status_get_error(status), false);
    return buffer;
}

sds mympd_api_status_updatedb_state(struct t_mpd_state *mpd_state, sds buffer) {
    long update_id = mympd_api_status_updatedb_id(mpd_state);
    if (update_id == -1) {
        buffer = jsonrpc_notify(buffer, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Error getting MPD status");
    }
    else if (update_id > 0) {
        buffer = jsonrpc_notify_start(buffer, JSONRPC_EVENT_UPDATE_STARTED);
        buffer = tojson_long(buffer, "jobid", update_id, false);
        buffer = jsonrpc_respond_end(buffer);
    }
    else {
        buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_FINISHED);
    }
    return buffer;
}

long mympd_api_status_updatedb_id(struct t_mpd_state *mpd_state) {
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (status == NULL) {
        mympd_check_error_and_recover(mpd_state);
        return -1;
    }
    long update_id = (long)mpd_status_get_update_id(status);
    MYMPD_LOG_NOTICE("Update database ID: %ld", update_id);
    mpd_status_free(status);
    mpd_response_finish(mpd_state->conn);
    mympd_check_error_and_recover(mpd_state);
    return update_id;
}

sds mympd_api_status_get(struct t_mympd_state *mympd_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_STATE;
    struct mpd_status *status = mpd_run_status(mympd_state->mpd_state->conn);
    if (status == NULL) {
        if (request_id == REQUEST_ID_NOTIFY) {
            mympd_check_error_and_recover_notify(mympd_state->mpd_state, &buffer);
        }
        else {
            mympd_check_error_and_recover_respond(mympd_state->mpd_state, &buffer, cmd_id, request_id);
        }
        return buffer;
    }

    int song_id = mpd_status_get_song_id(status);
    if (mympd_state->mpd_state->song_id != song_id) {
        mympd_state->mpd_state->last_song_id = mympd_state->mpd_state->song_id;
        mympd_state->mpd_state->last_song_end_time = mympd_state->mpd_state->song_end_time;
        mympd_state->mpd_state->last_song_start_time = mympd_state->mpd_state->song_start_time;
        mympd_state->mpd_state->last_song_set_song_played_time = mympd_state->mpd_state->set_song_played_time;
        struct mpd_song *song = mpd_run_current_song(mympd_state->mpd_state->conn);
        if (song != NULL) {
            mympd_state->mpd_state->last_song_uri = sds_replace(mympd_state->mpd_state->last_song_uri, mympd_state->mpd_state->song_uri);
            mympd_state->mpd_state->song_uri = sds_replace(mympd_state->mpd_state->song_uri, mpd_song_get_uri(song));
            mpd_song_free(song);
        }
        else {
            sdsclear(mympd_state->mpd_state->song_uri);
        }
        mpd_response_finish(mympd_state->mpd_state->conn);
        mympd_check_error_and_recover(mympd_state->mpd_state);
    }

    mympd_state->mpd_state->state = mpd_status_get_state(status);
    mympd_state->mpd_state->song_id = song_id;
    mympd_state->mpd_state->song_pos = mpd_status_get_song_pos(status);
    mympd_state->mpd_state->next_song_id = mpd_status_get_next_song_id(status);
    mympd_state->mpd_state->queue_version = mpd_status_get_queue_version(status);
    mympd_state->mpd_state->queue_length = (long long)mpd_status_get_queue_length(status);
    mympd_state->mpd_state->crossfade = (time_t)mpd_status_get_crossfade(status);

    time_t total_time = (time_t)mpd_status_get_total_time(status);
    time_t elapsed_time = (time_t)mympd_api_get_elapsed_seconds(status);

    time_t now = time(NULL);
    time_t uptime = now - mympd_state->config->startup_time;

    if (total_time == 0) {
        //no song end time for streams
        mympd_state->mpd_state->song_end_time = 0;
    }
    else {
        mympd_state->mpd_state->song_end_time = now + total_time - elapsed_time;
    }
    mympd_state->mpd_state->song_start_time = now - elapsed_time;
    time_t half_time = total_time / 2;

    if (total_time <= 10 ||  //don't track songs with length < 10s
        uptime < half_time)  //don't track songs with played more then half before startup
    {
        mympd_state->mpd_state->set_song_played_time = 0;
    }
    else if (half_time > 240) {  //set played after 4 minutes
        mympd_state->mpd_state->set_song_played_time = now - elapsed_time + 240;
    }
    else { //set played after halftime of song
        mympd_state->mpd_state->set_song_played_time = elapsed_time < half_time ? now - (long)elapsed_time + (long)half_time : now;
    }

    if (request_id == REQUEST_ID_NOTIFY) {
        buffer = jsonrpc_notify_start(buffer, JSONRPC_EVENT_UPDATE_STATE);
    }
    else {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    }
    buffer = mympd_api_status_print(mympd_state->mpd_state, buffer, status);
    buffer = jsonrpc_respond_end(buffer);

    mpd_status_free(status);
    return buffer;
}

bool mympd_api_status_lua_mympd_state_set(struct t_mympd_state *mympd_state, struct t_list *lua_mympd_state) {
    struct mpd_status *status = mpd_run_status(mympd_state->mpd_state->conn);
    if (status == NULL) {
        mympd_check_error_and_recover(mympd_state->mpd_state);
        return false;
    }
    lua_mympd_state_set_i(lua_mympd_state, "play_state", mpd_status_get_state(status));
    lua_mympd_state_set_i(lua_mympd_state, "volume", mpd_status_get_volume(status));
    lua_mympd_state_set_i(lua_mympd_state, "song_pos", mpd_status_get_song_pos(status));
    lua_mympd_state_set_u(lua_mympd_state, "elapsed_time", mympd_api_get_elapsed_seconds(status));
    lua_mympd_state_set_u(lua_mympd_state, "total_time", mpd_status_get_total_time(status));
    lua_mympd_state_set_i(lua_mympd_state, "song_id", mpd_status_get_song_id(status));
    lua_mympd_state_set_i(lua_mympd_state, "next_song_id", mpd_status_get_next_song_id(status));
    lua_mympd_state_set_i(lua_mympd_state, "next_song_pos", mpd_status_get_next_song_pos(status));
    lua_mympd_state_set_u(lua_mympd_state, "queue_length", mpd_status_get_queue_length(status));
    lua_mympd_state_set_u(lua_mympd_state, "queue_version", mpd_status_get_queue_version(status));
    lua_mympd_state_set_b(lua_mympd_state, "repeat", mpd_status_get_repeat(status));
    lua_mympd_state_set_b(lua_mympd_state, "random", mpd_status_get_random(status));
    lua_mympd_state_set_i(lua_mympd_state, "single_state", mpd_status_get_single_state(status));
    lua_mympd_state_set_b(lua_mympd_state, "consume", mpd_status_get_consume(status));
    lua_mympd_state_set_u(lua_mympd_state, "crossfade", mpd_status_get_crossfade(status));
    lua_mympd_state_set_p(lua_mympd_state, "music_directory", mympd_state->music_directory_value);
    lua_mympd_state_set_p(lua_mympd_state, "workdir", mympd_state->config->workdir);
    lua_mympd_state_set_i(lua_mympd_state, "jukebox_mode", mympd_state->jukebox_mode);
    lua_mympd_state_set_p(lua_mympd_state, "jukebox_playlist", mympd_state->jukebox_playlist);
    lua_mympd_state_set_i(lua_mympd_state, "jukebox_queue_length", mympd_state->jukebox_queue_length);
    lua_mympd_state_set_i(lua_mympd_state, "jukebox_last_played", mympd_state->jukebox_last_played);
    if (mympd_state->mpd_state->feat_mpd_partitions == true) {
        lua_mympd_state_set_p(lua_mympd_state, "partition", mpd_status_get_partition(status));
    }
    mpd_status_free(status);
    mpd_response_finish(mympd_state->mpd_state->conn);
    mympd_check_error_and_recover(mympd_state->mpd_state);
    lua_mympd_state_set_p(lua_mympd_state, "jukebox_unique_tag", mpd_tag_name(mympd_state->jukebox_unique_tag.tags[0]));
    lua_mympd_state_set_p(lua_mympd_state, "listenbrainz_token", mympd_state->listenbrainz_token);
    return true;
}

sds mympd_api_status_volume_get(struct t_mpd_state *mpd_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_VOLUME_GET;
    int volume = mpd_client_get_volume(mpd_state);
    if (request_id == REQUEST_ID_NOTIFY) {
        buffer = jsonrpc_notify_start(buffer, JSONRPC_EVENT_UPDATE_VOLUME);
    }
    else {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    }
    buffer = tojson_long(buffer, "volume", volume, false);
    buffer = jsonrpc_respond_end(buffer);
    return buffer;
}

sds mympd_api_status_current_song(struct t_mympd_state *mympd_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_CURRENT_SONG;
    struct mpd_song *song = mpd_run_current_song(mympd_state->mpd_state->conn);
    if (song == NULL) {
        if (mympd_check_error_and_recover_respond(mympd_state->mpd_state, &buffer, cmd_id, request_id) == false) {
            return buffer;
        }
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_PLAYER, JSONRPC_SEVERITY_INFO, "No current song");
        return buffer;
    }

    const char *uri = mpd_song_get_uri(song);

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = tojson_uint(buffer, "pos", mpd_song_get_pos(song), true);
    buffer = tojson_long(buffer, "currentSongId", mympd_state->mpd_state->song_id, true);
    buffer = get_song_tags(buffer, mympd_state->mpd_state, &mympd_state->mpd_state->tag_types_mympd, song);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = mympd_api_sticker_list(buffer, &mympd_state->sticker_cache, mpd_song_get_uri(song));
    buffer = sdscatlen(buffer, ",", 1);
    buffer = get_extra_media(mympd_state, buffer, uri, false);
    if (is_streamuri(uri) == true) {
        sds webradio = get_webradio_from_uri(mympd_state->config->workdir, uri);
        if (sdslen(webradio) > 0) {
            buffer = sdscat(buffer, ",\"webradio\":{");
            buffer = sdscatsds(buffer, webradio);
            buffer = sdscatlen(buffer, "}", 1);
        }
        FREE_SDS(webradio);
    }
    mpd_song_free(song);

    mpd_response_finish(mympd_state->mpd_state->conn);
    if (mympd_check_error_and_recover_respond(mympd_state->mpd_state, &buffer, cmd_id, request_id) == false) {
        return buffer;
    }
    buffer = sdscatlen(buffer, ",", 1);
    time_t start_time = _get_current_song_start_time(mympd_state->mpd_state);
    buffer = tojson_llong(buffer, "startTime", (long long)start_time, false);
    buffer = jsonrpc_respond_end(buffer);
    return buffer;
}

//private functions
static time_t _get_current_song_start_time(struct t_mpd_state *mpd_state) {
    if (mpd_state->song_start_time > 0) {
        return mpd_state->song_start_time;
    }
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (status == NULL) {
        mpd_response_finish(mpd_state->conn);
        mympd_check_error_and_recover(mpd_state);
        return 0;
    }
    const time_t start_time = time(NULL) - (time_t)mympd_api_get_elapsed_seconds(status);
    mpd_status_free(status);
    mpd_response_finish(mpd_state->conn);
    mympd_check_error_and_recover(mpd_state);
    return start_time;
}

static const char *_get_playstate_name(enum mpd_state play_state) {
    if ((unsigned)play_state >= 4) {
        return playstate_names[MPD_STATE_UNKNOWN];
    }
    return playstate_names[play_state];
}
