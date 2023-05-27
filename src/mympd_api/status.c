/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/status.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/lua_mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/tags.h"
#include "src/mpd_client/volume.h"
#include "src/mympd_api/extra_media.h"
#include "src/mympd_api/sticker.h"
#include "src/mympd_api/webradios.h"

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
    buffer = tojson_uint(buffer, "updateState", mpd_status_get_update_id(status), true);
    const bool updateCacheState = partition_state->mpd_state->album_cache.building ||
        partition_state->mpd_state->sticker_cache.building;
    buffer = tojson_bool(buffer, "updateCacheState", updateCacheState, true);
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
    long update_id = -1;
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status != NULL) {
        update_id = (long)mpd_status_get_update_id(status);
        MYMPD_LOG_NOTICE("Update database ID: %ld", update_id);
        mpd_status_free(status);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_status") == false) {
        update_id = -1;
    }
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
    int song_id = -1;
    if (status != NULL) {
        time_t now = time(NULL);
        song_id = mpd_status_get_song_id(status);
        if (partition_state->song_id != song_id) {
            //song has changed, save old state
            partition_state->last_song_id = partition_state->song_id;
            partition_state->last_song_end_time = partition_state->song_end_time;
            partition_state->last_song_start_time = partition_state->song_start_time;
            partition_state->last_song_scrobble_time = partition_state->song_scrobble_time;
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
        //scrobble time is half length of song or SCROBBLE_TIME_MAX (4 minutes) whatever is shorter
        time_t scrobble_time = total_time > SCROBBLE_TIME_TOTAL
            ? SCROBBLE_TIME_MAX
            : total_time / 2;

        partition_state->song_start_time = now - elapsed_time;
        partition_state->song_end_time = total_time == 0 ? 0 : now + total_time - elapsed_time;

        if (total_time <= SCROBBLE_TIME_MIN ||  //don't track songs with length < SCROBBLE_TIME_MIN (10s)
            elapsed_time > scrobble_time)       //don't track songs that exceeded scrobble time
        {
            partition_state->song_scrobble_time = 0;
        }
        else {
            partition_state->song_scrobble_time = now - elapsed_time + scrobble_time;
        }
        MYMPD_LOG_DEBUG("Now %lld, start time %lld, scrobble time %lld, end time %lld",
            (long long)now, (long long)partition_state->song_start_time, 
            (long long)partition_state->song_scrobble_time, (long long)partition_state->song_end_time);

        if (request_id == REQUEST_ID_NOTIFY) {
            buffer = jsonrpc_notify_start(buffer, JSONRPC_EVENT_UPDATE_STATE);
        }
        else {
            buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        }
        buffer = mympd_api_status_print(partition_state, buffer, status);
        buffer = jsonrpc_end(buffer);

        mpd_status_free(status);
    }
    mpd_response_finish(partition_state->conn);
    if (request_id == REQUEST_ID_NOTIFY) {
        mympd_check_error_and_recover_notify(partition_state, &buffer, "mpd_run_status");
    }
    else {
        mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_run_status");
    }
    //update song uri if song has changed
    if (partition_state->song_id != song_id) {
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
    }
    if (request_id == REQUEST_ID_NOTIFY) {
        mympd_check_error_and_recover_notify(partition_state, &buffer, "mpd_run_status");
    }
    else {
        mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_run_status");
    }
    return buffer;
}

/**
 * Copies mpd and myMPD states to the lua_mympd_state struct
 * @param lua_partition_state pointer to struct t_list
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mympd_api_status_lua_mympd_state_set(struct t_list *lua_partition_state, struct t_partition_state *partition_state) {
    //TODO: use command list to get replay gain and status in one call to mpd
    enum mpd_replay_gain_mode replay_gain_mode = mpd_run_replay_gain_status(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_replay_gain_status") == false) {
        return false;
    }

    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status != NULL) {
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
        lua_mympd_state_set_i(lua_partition_state, "consume_state", mpd_status_get_consume_state(status));
        lua_mympd_state_set_u(lua_partition_state, "crossfade", mpd_status_get_crossfade(status));
        lua_mympd_state_set_f(lua_partition_state, "mixrampdelay", mpd_status_get_mixrampdelay(status));
        lua_mympd_state_set_f(lua_partition_state, "mixrampdb", mpd_status_get_mixrampdb(status));
        lua_mympd_state_set_i(lua_partition_state, "replaygain", replay_gain_mode);
        lua_mympd_state_set_p(lua_partition_state, "music_directory", partition_state->mpd_state->music_directory_value);
        lua_mympd_state_set_p(lua_partition_state, "playlist_directory", partition_state->mpd_state->playlist_directory_value);
        lua_mympd_state_set_p(lua_partition_state, "workdir", partition_state->mympd_state->config->workdir);
        lua_mympd_state_set_p(lua_partition_state, "cachedir", partition_state->mympd_state->config->cachedir);
        lua_mympd_state_set_b(lua_partition_state, "auto_play", partition_state->auto_play);
        lua_mympd_state_set_i(lua_partition_state, "jukebox_mode", partition_state->jukebox_mode);
        lua_mympd_state_set_p(lua_partition_state, "jukebox_playlist", partition_state->jukebox_playlist);
        lua_mympd_state_set_i(lua_partition_state, "jukebox_queue_length", partition_state->jukebox_queue_length);
        lua_mympd_state_set_i(lua_partition_state, "jukebox_last_played", partition_state->jukebox_last_played);
        lua_mympd_state_set_b(lua_partition_state, "jukebox_ignore_hated", partition_state->jukebox_ignore_hated);
        lua_mympd_state_set_p(lua_partition_state, "jukebox_unique_tag", mpd_tag_name(partition_state->jukebox_unique_tag.tags[0]));
        lua_mympd_state_set_p(lua_partition_state, "listenbrainz_token", partition_state->mympd_state->listenbrainz_token);
        if (partition_state->mpd_state->feat_partitions == true) {
            lua_mympd_state_set_p(lua_partition_state, "partition", mpd_status_get_partition(status));
        }
        mpd_status_free(status);
    }
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_run_status");
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
    //TODO: use command list and to get status and current song with one call to mpd
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYER_CURRENT_SONG;
    struct mpd_song *song = mpd_run_current_song(partition_state->conn);
    if (song == NULL &&
        mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_run_current_song") == true)
    {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_PLAYER, JSONRPC_SEVERITY_INFO, "No current song");
    }
    
    const char *uri = mpd_song_get_uri(song);

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = tojson_uint(buffer, "pos", mpd_song_get_pos(song), true);
    buffer = tojson_long(buffer, "currentSongId", partition_state->song_id, true);
    buffer = get_song_tags(buffer, partition_state->mpd_state->feat_tags, &partition_state->mpd_state->tags_mympd, song);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = mympd_api_sticker_get_print(buffer, &partition_state->mpd_state->sticker_cache, uri);
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
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_run_current_song") == false) {
        return buffer;
    }

    buffer = sdscatlen(buffer, ",", 1);
    time_t start_time = get_current_song_start_time(partition_state);
    buffer = tojson_time(buffer, "startTime", start_time, false);
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
    time_t start_time = 0;
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status != NULL) {
        start_time = time(NULL) - (time_t)mympd_api_get_elapsed_seconds(status);
        mpd_status_free(status);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_status") == false) {
        start_time = 0;
    }
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
