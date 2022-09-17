/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "status.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/lua_mympd_state.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../mpd_client/errorhandler.h"
#include "../mpd_client/tags.h"
#include "../mpd_client/volume.h"
#include "extra_media.h"
#include "sticker.h"
#include "webradios.h"

/**
 * Private definitions
 */

static time_t get_current_song_start_time(struct t_partition_state *partition_state);
static const char *get_playstate_name(enum mpd_state play_state);

/**
 * Array to resolv the mpd state to a string
 */
static const char *playstate_names[] = {
    [MPD_STATE_UNKNOWN] = "unknown",
    [MPD_STATE_STOP] = "stop",
    [MPD_STATE_PLAY] = "play",
    [MPD_STATE_PAUSE] = "pause"
};

/**
 * Public functions
 */

/**
 * Replacement for deprecated mpd_status_get_elapsed_time
 * @param status pointer to mpd_status struct
 * @return elapsed seconds
 */
unsigned mympd_api_get_elapsed_seconds(struct mpd_status *status) {
    return mpd_status_get_elapsed_ms(status) / 1000;
}

/**
 * Prints the mpd_status as jsonrpc object string
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param status pointer to mpd_status struct
 * @return pointer to buffer
 */
sds mympd_api_status_print(struct t_partition_state *partition_state, sds buffer, struct mpd_status *status) {
    enum mpd_state playstate = mpd_status_get_state(status);

    buffer = tojson_char(buffer, "state", get_playstate_name(playstate), true);
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
    buffer = tojson_long(buffer, "lastSongId", (partition_state->last_song_id ?
        partition_state->last_song_id : -1), true);
    if (partition_state->mpd_state->feat_partitions == true) {
        buffer = tojson_char(buffer, "partition", mpd_status_get_partition(status), true);
    }
    const struct mpd_audio_format *audioformat = mpd_status_get_audio_format(status);
    buffer = printAudioFormat(buffer, audioformat);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_char(buffer, "lastError", mpd_status_get_error(status), false);
    return buffer;
}

/**
 * Gets the update database status from mpd as jsonrpc notification
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @return pointer to buffer
 */
sds mympd_api_status_updatedb_state(struct t_partition_state *partition_state, sds buffer) {
    long update_id = mympd_api_status_updatedb_id(partition_state);
    if (update_id == -1) {
        buffer = jsonrpc_notify(buffer, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_ERROR, "Error getting MPD status");
    }
    else if (update_id > 0) {
        buffer = jsonrpc_notify_start(buffer, JSONRPC_EVENT_UPDATE_STARTED);
        buffer = tojson_long(buffer, "jobid", update_id, false);
        buffer = jsonrpc_end(buffer);
    }
    else {
        buffer = jsonrpc_event(buffer, JSONRPC_EVENT_UPDATE_FINISHED);
    }
    return buffer;
}

/**
 * Gets the update database id
 * @param partition_state pointer to partition state
 * @return database update id
 */
long mympd_api_status_updatedb_id(struct t_partition_state *partition_state) {
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status == NULL) {
        mympd_check_error_and_recover(partition_state);
        return -1;
    }
    long update_id = (long)mpd_status_get_update_id(status);
    MYMPD_LOG_NOTICE("Update database ID: %ld", update_id);
    mpd_status_free(status);
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state);
    return update_id;
}

/**
 * Gets the mpd status, updates internal myMPD states and returns a jsonrpc notify or response
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id, REQUEST_ID_NOTIFY or REQUEST_ID_RESPONSE
 * @return pointer to buffer
 */
sds mympd_api_status_get(struct t_partition_state *partition_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_STATE;
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status == NULL) {
        if (request_id == REQUEST_ID_NOTIFY) {
            mympd_check_error_and_recover_notify(partition_state, &buffer);
        }
        else {
            mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id);
        }
        return buffer;
    }

    int song_id = mpd_status_get_song_id(status);
    if (partition_state->song_id != song_id) {
        partition_state->last_song_id = partition_state->song_id;
        partition_state->last_song_end_time = partition_state->song_end_time;
        partition_state->last_song_start_time = partition_state->song_start_time;
        partition_state->last_song_set_song_played_time = partition_state->set_song_played_time;
        struct mpd_song *song = mpd_run_current_song(partition_state->conn);
        if (song != NULL) {
            partition_state->last_song_uri = sds_replace(partition_state->last_song_uri, partition_state->song_uri);
            partition_state->song_uri = sds_replace(partition_state->song_uri, mpd_song_get_uri(song));
            mpd_song_free(song);
        }
        else {
            sdsclear(partition_state->song_uri);
        }
        mpd_response_finish(partition_state->conn);
        mympd_check_error_and_recover(partition_state);
    }

    partition_state->play_state = mpd_status_get_state(status);
    partition_state->song_id = song_id;
    partition_state->song_pos = mpd_status_get_song_pos(status);
    partition_state->next_song_id = mpd_status_get_next_song_id(status);
    partition_state->queue_version = mpd_status_get_queue_version(status);
    partition_state->queue_length = (long long)mpd_status_get_queue_length(status);
    partition_state->crossfade = (time_t)mpd_status_get_crossfade(status);

    time_t total_time = (time_t)mpd_status_get_total_time(status);
    time_t elapsed_time = (time_t)mympd_api_get_elapsed_seconds(status);

    time_t now = time(NULL);
    time_t uptime = now - partition_state->mympd_state->config->startup_time;

    if (total_time == 0) {
        //no song end time for streams
        partition_state->song_end_time = 0;
    }
    else {
        partition_state->song_end_time = now + total_time - elapsed_time;
    }
    partition_state->song_start_time = now - elapsed_time;
    time_t half_time = total_time / 2;

    if (total_time <= 10 ||  //don't track songs with length < 10s
        uptime < half_time)  //don't track songs with played more then half before startup
    {
        partition_state->set_song_played_time = 0;
    }
    else if (half_time > 240) {  //set played after 4 minutes
        partition_state->set_song_played_time = now - elapsed_time + 240;
    }
    else { //set played after halftime of song
        partition_state->set_song_played_time = elapsed_time < half_time ? now - (long)elapsed_time + (long)half_time : now;
    }

    if (request_id == REQUEST_ID_NOTIFY) {
        buffer = jsonrpc_notify_start(buffer, JSONRPC_EVENT_UPDATE_STATE);
    }
    else {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    }
    buffer = mympd_api_status_print(partition_state, buffer, status);
    buffer = jsonrpc_end(buffer);

    mpd_status_free(status);
    return buffer;
}

/**
 * Copies mpd and myMPD states to the lua_mympd_state struct
 * @param lua_partition_state pointer to struct t_list
 * @param partition_state pointer to partition state
 * @param listenbrainz_token listenbrainz token
 * @return true on success, else false
 */
bool mympd_api_status_lua_mympd_state_set(struct t_list *lua_partition_state, struct t_partition_state *partition_state,
        sds listenbrainz_token)
{
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status == NULL) {
        mympd_check_error_and_recover(partition_state);
        return false;
    }
    lua_mympd_state_set_i(lua_partition_state, "play_state", mpd_status_get_state(status));
    lua_mympd_state_set_i(lua_partition_state, "volume", mpd_status_get_volume(status));
    lua_mympd_state_set_i(lua_partition_state, "song_pos", mpd_status_get_song_pos(status));
    lua_mympd_state_set_u(lua_partition_state, "elapsed_time", mympd_api_get_elapsed_seconds(status));
    lua_mympd_state_set_u(lua_partition_state, "total_time", mpd_status_get_total_time(status));
    lua_mympd_state_set_i(lua_partition_state, "song_id", mpd_status_get_song_id(status));
    lua_mympd_state_set_i(lua_partition_state, "next_song_id", mpd_status_get_next_song_id(status));
    lua_mympd_state_set_i(lua_partition_state, "next_song_pos", mpd_status_get_next_song_pos(status));
    lua_mympd_state_set_u(lua_partition_state, "queue_length", mpd_status_get_queue_length(status));
    lua_mympd_state_set_u(lua_partition_state, "queue_version", mpd_status_get_queue_version(status));
    lua_mympd_state_set_b(lua_partition_state, "repeat", mpd_status_get_repeat(status));
    lua_mympd_state_set_b(lua_partition_state, "random", mpd_status_get_random(status));
    lua_mympd_state_set_i(lua_partition_state, "single_state", mpd_status_get_single_state(status));
    lua_mympd_state_set_b(lua_partition_state, "consume", mpd_status_get_consume(status));
    lua_mympd_state_set_u(lua_partition_state, "crossfade", mpd_status_get_crossfade(status));
    lua_mympd_state_set_p(lua_partition_state, "music_directory", partition_state->mpd_state->music_directory_value);
    lua_mympd_state_set_p(lua_partition_state, "workdir", partition_state->mympd_state->config->workdir);
    lua_mympd_state_set_i(lua_partition_state, "jukebox_mode", partition_state->jukebox_mode);
    lua_mympd_state_set_p(lua_partition_state, "jukebox_playlist", partition_state->jukebox_playlist);
    lua_mympd_state_set_i(lua_partition_state, "jukebox_queue_length", partition_state->jukebox_queue_length);
    lua_mympd_state_set_i(lua_partition_state, "jukebox_last_played", partition_state->jukebox_last_played);
    if (partition_state->mpd_state->feat_partitions == true) {
        lua_mympd_state_set_p(lua_partition_state, "partition", mpd_status_get_partition(status));
    }
    mpd_status_free(status);
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state);
    lua_mympd_state_set_p(lua_partition_state, "jukebox_unique_tag", mpd_tag_name(partition_state->jukebox_unique_tag.tags[0]));
    lua_mympd_state_set_p(lua_partition_state, "listenbrainz_token", listenbrainz_token);
    return true;
}

/**
 * Gets the mpd volume as jsonrpc notify
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds mympd_api_status_volume_get(struct t_partition_state *partition_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_VOLUME_GET;
    int volume = mpd_client_get_volume(partition_state);
    if (request_id == REQUEST_ID_NOTIFY) {
        buffer = jsonrpc_notify_start(buffer, JSONRPC_EVENT_UPDATE_VOLUME);
    }
    else {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    }
    buffer = tojson_long(buffer, "volume", volume, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Gets the current playing song as jsonrpc response
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds mympd_api_status_current_song(struct t_partition_state *partition_state, sds buffer, long request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_CURRENT_SONG;
    struct mpd_song *song = mpd_run_current_song(partition_state->conn);
    if (song == NULL) {
        if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
            return buffer;
        }
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_PLAYER, JSONRPC_SEVERITY_INFO, "No current song");
        return buffer;
    }

    const char *uri = mpd_song_get_uri(song);

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = tojson_uint(buffer, "pos", mpd_song_get_pos(song), true);
    buffer = tojson_long(buffer, "currentSongId", partition_state->song_id, true);
    buffer = get_song_tags(buffer, partition_state, &partition_state->mpd_state->tags_mympd, song);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = mympd_api_sticker_list(buffer, &partition_state->mpd_state->sticker_cache, mpd_song_get_uri(song));
    buffer = sdscatlen(buffer, ",", 1);
    buffer = mympd_api_get_extra_media(partition_state->mpd_state, buffer, uri, false);
    if (is_streamuri(uri) == true) {
        sds webradio = get_webradio_from_uri(partition_state->mympd_state->config->workdir, uri);
        if (sdslen(webradio) > 0) {
            buffer = sdscat(buffer, ",\"webradio\":{");
            buffer = sdscatsds(buffer, webradio);
            buffer = sdscatlen(buffer, "}", 1);
        }
        FREE_SDS(webradio);
    }
    mpd_song_free(song);

    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
        return buffer;
    }
    buffer = sdscatlen(buffer, ",", 1);
    time_t start_time = get_current_song_start_time(partition_state);
    buffer = tojson_llong(buffer, "startTime", (long long)start_time, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Private functions
 */

/**
 * Gets the start time of current song as unix timestamp
 * @param partition_state pointer to partition state
 * @return start time of current song as unix timestamp
 */
static time_t get_current_song_start_time(struct t_partition_state *partition_state) {
    if (partition_state->song_start_time > 0) {
        return partition_state->song_start_time;
    }
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status == NULL) {
        mpd_response_finish(partition_state->conn);
        mympd_check_error_and_recover(partition_state);
        return 0;
    }
    const time_t start_time = time(NULL) - (time_t)mympd_api_get_elapsed_seconds(status);
    mpd_status_free(status);
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state);
    return start_time;
}

/**
 * Resolves the mpd_state enum to a string
 * @param play_state mpd_state
 * @return play state as string
 */
static const char *get_playstate_name(enum mpd_state play_state) {
    if ((unsigned)play_state >= 4) {
        return playstate_names[MPD_STATE_UNKNOWN];
    }
    return playstate_names[play_state];
}
