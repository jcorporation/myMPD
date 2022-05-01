/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_settings.h"

#include "../../dist/mjson/mjson.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"
#include "../lib/state_files.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_client/mpd_client_jukebox.h"
#include "../mympd_api/mympd_api_trigger.h"
#include "../mympd_api/mympd_api_utility.h"
#include "../mpd_shared.h"
#include "mympd_api_timer.h"
#include "mympd_api_timer_handlers.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//private definitions
static sds set_default_navbar_icons(struct t_config *config, sds buffer);
static sds read_navbar_icons(struct t_config *config);
static sds print_tags_array(sds buffer, const char *tagsname, struct t_tags tags);
static sds set_invalid_value(sds error, sds key, sds value);

//public functions
bool mympd_api_settings_connection_save(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error) {
    (void) vcb;
    struct t_mympd_state *mympd_state = (struct t_mympd_state *)userdata;
    bool check_for_mpd_error = false;

    MYMPD_LOG_DEBUG("Parse setting \"%s\": \"%s\" (%s)", key, value, get_mjson_toktype_name(vtype));

    if (strcmp(key, "mpdPass") == 0 && vtype == MJSON_TOK_STRING) {
        if (strcmp(value, "dontsetpassword") != 0) {
            if (vcb_isname(value) == false) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            mympd_state->mpd_state->mpd_pass = sds_replace(mympd_state->mpd_state->mpd_pass, value);
        }
        else {
            //keep old password
            return true;
        }
    }
    else if (strcmp(key, "mpdHost") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilepath(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, value);
    }
    else if (strcmp(key, "mpdPort") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned mpd_port = (unsigned)strtoumax(value, NULL, 10);
        if (mpd_port < MPD_PORT_MIN || mpd_port > MPD_PORT_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->mpd_state->mpd_port = mpd_port;
    }
    else if (strcmp(key, "mpdStreamPort") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned mpd_stream_port = (unsigned)strtoumax(value, NULL, 10);
        if (mpd_stream_port < MPD_PORT_MIN || mpd_stream_port > MPD_PORT_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->mpd_stream_port = mpd_stream_port;
    }
    else if (strcmp(key, "musicDirectory") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilepath(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->music_directory = sds_replace(mympd_state->music_directory, value);
        sds_strip_slash(mympd_state->music_directory);
    }
    else if (strcmp(key, "playlistDirectory") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilepath(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->playlist_directory = sds_replace(mympd_state->playlist_directory, value);
        sds_strip_slash(mympd_state->playlist_directory);
    }
    else if (strcmp(key, "mpdBinarylimit") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned binarylimit = (unsigned)strtoumax(value, NULL, 10);
        if (binarylimit < MPD_BINARY_SIZE_MIN || binarylimit > MPD_BINARY_SIZE_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (binarylimit != mympd_state->mpd_state->mpd_binarylimit) {
            mympd_state->mpd_state->mpd_binarylimit = binarylimit;
            if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                mympd_api_set_binarylimit(mympd_state);
            }
        }
    }
    else if (strcmp(key, "mpdTimeout") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned mpd_timeout = (unsigned)strtoumax(value, NULL, 10);
        if (mpd_timeout < MPD_TIMEOUT_MIN || mpd_timeout > MPD_TIMEOUT_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (mpd_timeout != mympd_state->mpd_state->mpd_timeout) {
            mympd_state->mpd_state->mpd_timeout = mpd_timeout;
            if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                mpd_connection_set_timeout(mympd_state->mpd_state->conn, mympd_state->mpd_state->mpd_timeout);
                check_for_mpd_error = true;
            }
        }
    }
    else if (strcmp(key, "mpdKeepalive") == 0) {
        if (vtype != MJSON_TOK_TRUE && vtype != MJSON_TOK_FALSE) {
            *error = sdscatfmt(*error, "Invalid value for \"%s\": \"%s\"", key, value);
            MYMPD_LOG_WARN("%s", *error);
            return false;
        }
        bool keepalive = vtype == MJSON_TOK_TRUE ? true : false;
        if (keepalive != mympd_state->mpd_state->mpd_keepalive) {
            mympd_state->mpd_state->mpd_keepalive = keepalive;
            if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                mpd_connection_set_keepalive(mympd_state->mpd_state->conn, mympd_state->mpd_state->mpd_keepalive);
                check_for_mpd_error = true;
            }
        }
    }
    else {
        *error = sdscatfmt(*error, "Unknown setting \"%s\": \"%s\"", key, value);
        MYMPD_LOG_WARN("%s", *error);
        return true;
    }

    bool rc = true;
    if (check_for_mpd_error == true) {
        *error = check_error_and_recover(mympd_state->mpd_state, *error, NULL, 0);
        if (sdslen(*error) > 0) {
            rc = false;
        }
    }

    if (rc == true) {
        sds state_filename = camel_to_snake(key);
        rc = state_file_write(mympd_state->config->workdir, "state", state_filename, value);
        FREE_SDS(state_filename);
    }
    return rc;
}

bool mympd_api_settings_cols_save(struct t_mympd_state *mympd_state, sds table, sds cols) {
    if (strcmp(table, "colsQueueCurrent") == 0) {
        mympd_state->cols_queue_current = sds_replace(mympd_state->cols_queue_current, cols);
    }
    else if (strcmp(table, "colsQueueLastPlayed") == 0) {
        mympd_state->cols_queue_last_played = sds_replace(mympd_state->cols_queue_last_played, cols);
    }
    else if (strcmp(table, "colsSearch") == 0) {
        mympd_state->cols_search = sds_replace(mympd_state->cols_search, cols);
    }
    else if (strcmp(table, "colsBrowseDatabaseDetail") == 0) {
        mympd_state->cols_browse_database_detail = sds_replace(mympd_state->cols_browse_database_detail, cols);
    }
    else if (strcmp(table, "colsBrowsePlaylistsDetail") == 0) {
        mympd_state->cols_browse_playlists_detail = sds_replace(mympd_state->cols_browse_playlists_detail, cols);
    }
    else if (strcmp(table, "colsBrowseFilesystem") == 0) {
        mympd_state->cols_browse_filesystem = sds_replace(mympd_state->cols_browse_filesystem, cols);
    }
    else if (strcmp(table, "colsPlayback") == 0) {
        mympd_state->cols_playback = sds_replace(mympd_state->cols_playback, cols);
    }
    else if (strcmp(table, "colsQueueJukebox") == 0) {
        mympd_state->cols_queue_jukebox = sds_replace(mympd_state->cols_queue_jukebox, cols);
    }
    else if (strcmp(table, "colsBrowseRadioWebradiodb") == 0) {
        mympd_state->cols_browse_radio_webradiodb = sds_replace(mympd_state->cols_browse_radio_webradiodb, cols);
    }
    else if (strcmp(table, "colsBrowseRadioRadiobrowser") == 0) {
        mympd_state->cols_browse_radio_radiobrowser = sds_replace(mympd_state->cols_browse_radio_radiobrowser, cols);
    }
    else {
        return false;
    }
    sds tablename = camel_to_snake(table);
    bool rc = state_file_write(mympd_state->config->workdir, "state", tablename, cols);
    FREE_SDS(tablename);
    return rc;
}

bool mympd_api_settings_set(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error) {
    (void) vcb;
    struct t_mympd_state *mympd_state = (struct t_mympd_state *)userdata;

    MYMPD_LOG_DEBUG("Parse setting \"%s\": \"%s\" (%s)", key, value, get_mjson_toktype_name(vtype));

    if (strcmp(key, "coverimageNames") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilename(value) == true) {
            mympd_state->coverimage_names = sds_replace(mympd_state->coverimage_names, value);
        }
        else {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
    }
    else if (strcmp(key, "thumbnailNames") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilename(value) == true) {
            mympd_state->thumbnail_names = sds_replace(mympd_state->thumbnail_names, value);
        }
        else {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
    }
    else if (strcmp(key, "bookletName") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilename(value) == true) {
            mympd_state->booklet_name = sds_replace(mympd_state->booklet_name, value);
        }
        else {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
    }
    else if (strcmp(key, "lastPlayedCount") == 0 && vtype == MJSON_TOK_NUMBER) {
        long last_played_count = (long)strtoimax(value, NULL, 10);
        if (last_played_count < 0 || last_played_count > MPD_PLAYLIST_LENGTH_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->last_played_count = last_played_count;
    }
    else if (strcmp(key, "volumeMin") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned volume_min = (unsigned)strtoumax(value, NULL, 10);
        if (volume_min > VOLUME_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->volume_min = volume_min;
    }
    else if (strcmp(key, "volumeMax") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned volume_max = (unsigned)strtoumax(value, NULL, 10);
        if (volume_max > VOLUME_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->volume_max = volume_max;
    }
    else if (strcmp(key, "volumeStep") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned volume_step = (unsigned)strtoimax(value, NULL, 10);
        if (volume_step < VOLUME_STEP_MIN || volume_step > VOLUME_STEP_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->volume_step = volume_step;
    }
    else if (strcmp(key, "tagList") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_istaglist(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->mpd_state->tag_list = sds_replace(mympd_state->mpd_state->tag_list, value);
    }
    else if (strcmp(key, "tagListSearch") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_istaglist(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->tag_list_search = sds_replace(mympd_state->tag_list_search, value);
    }
    else if (strcmp(key, "tagListBrowse") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_istaglist(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->tag_list_browse = sds_replace(mympd_state->tag_list_browse, value);
    }
    else if (strcmp(key, "smartpls") == 0) {
        if (vtype == MJSON_TOK_TRUE) {
            mympd_state->smartpls = true;
        }
        else if (vtype == MJSON_TOK_FALSE) {
            mympd_state->smartpls = false;
        }
        else {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
    }
    else if (strcmp(key, "smartplsSort") == 0 && vtype == MJSON_TOK_STRING) {
        if (sdslen(value) > 0 && vcb_ismpdsort(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->smartpls_sort = sds_replace(mympd_state->smartpls_sort, value);
    }
    else if (strcmp(key, "smartplsPrefix") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilename(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->smartpls_prefix = sds_replacelen(mympd_state->smartpls_prefix, value, sdslen(value));
    }
    else if (strcmp(key, "smartplsInterval") == 0 && vtype == MJSON_TOK_NUMBER) {
        time_t interval = (time_t)strtoimax(value, NULL, 10);
        if (interval < TIMER_INTERVAL_MIN || interval > TIMER_INTERVAL_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (interval != mympd_state->smartpls_interval) {
            mympd_state->smartpls_interval = interval;
            mympd_api_timer_replace(&mympd_state->timer_list, interval, (int)interval, timer_handler_smartpls_update, 2, NULL, NULL);
        }
    }
    else if (strcmp(key, "smartplsGenerateTagList") == 0 && vtype == MJSON_TOK_STRING) {
        if (sdslen(value) > 0 && vcb_istaglist(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->smartpls_generate_tag_list = sds_replacelen(mympd_state->smartpls_generate_tag_list, value, sdslen(value));
    }
    else if (strcmp(key, "webuiSettings") == 0 && vtype == MJSON_TOK_OBJECT) {
        mympd_state->webui_settings = sds_replacelen(mympd_state->webui_settings, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsUsltExt") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->lyrics_uslt_ext = sds_replacelen(mympd_state->lyrics_uslt_ext, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsSyltExt") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->lyrics_sylt_ext = sds_replacelen(mympd_state->lyrics_sylt_ext, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsVorbisUslt") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->lyrics_vorbis_uslt = sds_replacelen(mympd_state->lyrics_vorbis_uslt, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsVorbisSylt") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->lyrics_vorbis_sylt = sds_replacelen(mympd_state->lyrics_vorbis_sylt, value, sdslen(value));
    }
    else if (strcmp(key, "covercacheKeepDays") == 0 && vtype == MJSON_TOK_NUMBER) {
        int covercache_keep_days = (int)strtoimax(value, NULL, 10);
        if (covercache_keep_days < COVERCACHE_AGE_MIN || covercache_keep_days > COVERCACHE_AGE_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->covercache_keep_days = covercache_keep_days;
    }
    else if (strcmp(key, "listenbrainzToken") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->listenbrainz_token = sds_replacelen(mympd_state->listenbrainz_token, value, sdslen(value));
    }
    else {
        *error = sdscatfmt(*error, "Unknown setting \"%s\": \"%s\"", key, value);
        MYMPD_LOG_WARN("%s", *error);
        return true;
    }
    sds state_filename = camel_to_snake(key);
    bool rc = state_file_write(mympd_state->config->workdir, "state", state_filename, value);
    FREE_SDS(state_filename);
    return rc;
}

bool mympd_api_settings_mpd_options_set(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error) {
    (void) vcb;
    struct t_mympd_state *mympd_state = (struct t_mympd_state *)userdata;

    bool rc = false;
    bool write_state_file = true;
    bool jukebox_changed = false;

    MYMPD_LOG_DEBUG("Parse setting \"%s\": \"%s\" (%s)", key, value, get_mjson_toktype_name(vtype));
    if (strcmp(key, "autoPlay") == 0) {
        if (vtype == MJSON_TOK_TRUE) {
            mympd_state->auto_play = true;
        }
        else if (vtype == MJSON_TOK_FALSE) {
            mympd_state->auto_play = false;
        }
        else {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
    }
    else if (strcmp(key, "jukeboxMode") == 0 && vtype == MJSON_TOK_STRING) {
        enum jukebox_modes jukebox_mode = mympd_parse_jukebox_mode(value);

        if (jukebox_mode == JUKEBOX_UNKNOWN) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (mympd_state->jukebox_mode != jukebox_mode) {
            mympd_state->jukebox_mode = jukebox_mode;
            jukebox_changed = true;
        }
        sdsclear(value);
        value = sdscatprintf(value, "%d", jukebox_mode);
    }
    else if (strcmp(key, "jukeboxPlaylist") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilename(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (strcmp(mympd_state->jukebox_playlist, value) != 0) {
            mympd_state->jukebox_playlist = sds_replace(mympd_state->jukebox_playlist, value);
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxQueueLength") == 0 && vtype == MJSON_TOK_NUMBER) {
        long jukebox_queue_length = (long)strtoimax(value, NULL, 10);
        if (jukebox_queue_length <= 0 || jukebox_queue_length > JUKEBOX_QUEUE_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->jukebox_queue_length = jukebox_queue_length;
    }
    else if (strcmp(key, "jukeboxUniqueTag") == 0 && vtype == MJSON_TOK_STRING) {
        enum mpd_tag_type unique_tag = mpd_tag_name_parse(value);
        if (unique_tag == MPD_TAG_UNKNOWN) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (mympd_state->jukebox_unique_tag.tags[0] != unique_tag) {
            mympd_state->jukebox_unique_tag.tags[0] = unique_tag;
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxLastPlayed") == 0 && vtype == MJSON_TOK_NUMBER) {
        long jukebox_last_played = (long)strtoimax(value, NULL, 10);
        if (jukebox_last_played < 0 || jukebox_last_played > JUKEBOX_LAST_PLAYED_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (jukebox_last_played != mympd_state->jukebox_last_played) {
            mympd_state->jukebox_last_played = jukebox_last_played;
            jukebox_changed = true;
        }
    }
    else if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
        if (strcmp(key, "random") == 0) {
            if (vtype != MJSON_TOK_TRUE && vtype != MJSON_TOK_FALSE) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            bool bool_buf = vtype == MJSON_TOK_TRUE ? true : false;
            rc = mpd_run_random(mympd_state->mpd_state->conn, bool_buf);
        }
        else if (strcmp(key, "repeat") == 0) {
            if (vtype != MJSON_TOK_TRUE && vtype != MJSON_TOK_FALSE) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            bool bool_buf = vtype == MJSON_TOK_TRUE ? true : false;
            rc = mpd_run_repeat(mympd_state->mpd_state->conn, bool_buf);
        }
        else if (strcmp(key, "consume") == 0) {
            if (vtype != MJSON_TOK_TRUE && vtype != MJSON_TOK_FALSE) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            bool bool_buf = vtype == MJSON_TOK_TRUE ? true : false;
            rc = mpd_run_consume(mympd_state->mpd_state->conn, bool_buf);
        }
        else if (strcmp(key, "single") == 0) {
            enum mpd_single_state state = mpd_parse_single_state(value);
            if (state == MPD_SINGLE_UNKNOWN) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            rc = mpd_run_single_state(mympd_state->mpd_state->conn, state);
        }
        else if (strcmp(key, "crossfade") == 0 && vtype == MJSON_TOK_NUMBER) {
            unsigned uint_buf = (unsigned)strtoumax(value, NULL, 10);
            if (uint_buf > MPD_CROSSFADE_MAX) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            rc = mpd_run_crossfade(mympd_state->mpd_state->conn, uint_buf);
        }
        else if (strcmp(key, "replaygain") == 0 && vtype == MJSON_TOK_STRING) {
            enum mpd_replay_gain_mode mode = mpd_parse_replay_gain_name(value);
            if (mode == MPD_REPLAY_UNKNOWN) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            rc = mpd_run_replay_gain_mode(mympd_state->mpd_state->conn, mode);
        }
        else if (strcmp(key, "mixrampDb") == 0 && vtype == MJSON_TOK_NUMBER) {
            float db = strtof(value, NULL);
            if (db < -100 || db > 0) {
                //mixrampdb should be a negative value
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            rc = mpd_run_mixrampdb(mympd_state->mpd_state->conn, db);
        }
        else if (strcmp(key, "mixrampDelay") == 0 && vtype == MJSON_TOK_NUMBER) {
            float delay = strtof(value, NULL);
            if (delay < -1.0 || delay > 100) {
                //mixrampdb should be a positive value
                //0 disables mixramp
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            rc = mpd_run_mixrampdelay(mympd_state->mpd_state->conn, delay);
        }
        sds message = check_error_and_recover_notify(mympd_state->mpd_state, sdsempty());
        if (sdslen(message) > 0) {
            ws_notify(message);
            rc = false;
        }
        FREE_SDS(message);
        write_state_file = false;
    }
    else {
        MYMPD_LOG_WARN("Unknown setting \"%s\": \"%s\"", key, value);
        return false;
    }
    if (jukebox_changed == true && mympd_state->jukebox_queue.length > 0) {
        MYMPD_LOG_INFO("Jukebox options changed, clearing jukebox queue");
        mpd_client_clear_jukebox(&mympd_state->jukebox_queue);
    }
    if (write_state_file == true) {
        sds state_filename = camel_to_snake(key);
        rc = state_file_write(mympd_state->config->workdir, "state", state_filename, value);
        FREE_SDS(state_filename);
    }
    return rc;
}

void mympd_api_settings_statefiles_read(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_NOTICE("Reading states");
    mympd_state->mpd_state->mpd_host = state_file_rw_string_sds(mympd_state->config->workdir, "state", "mpd_host", mympd_state->mpd_state->mpd_host, vcb_isname, false);
    mympd_state->mpd_state->mpd_port = state_file_rw_uint(mympd_state->config->workdir, "state", "mpd_port", mympd_state->mpd_state->mpd_port, MPD_PORT_MIN, MPD_PORT_MAX, false);
    mympd_state->mpd_state->mpd_pass = state_file_rw_string_sds(mympd_state->config->workdir, "state", "mpd_pass", mympd_state->mpd_state->mpd_pass, vcb_isname, false);
    mympd_state->mpd_state->mpd_binarylimit = state_file_rw_uint(mympd_state->config->workdir, "state", "mpd_binarylimit", mympd_state->mpd_state->mpd_binarylimit, MPD_BINARY_SIZE_MIN, MPD_BINARY_SIZE_MAX, false);
    mympd_state->mpd_state->mpd_timeout = state_file_rw_uint(mympd_state->config->workdir, "state", "mpd_timeout", mympd_state->mpd_state->mpd_timeout, MPD_TIMEOUT_MIN, MPD_TIMEOUT_MAX, false);
    mympd_state->mpd_state->mpd_keepalive = state_file_rw_bool(mympd_state->config->workdir, "state", "mpd_keepalive", mympd_state->mpd_state->mpd_keepalive, false);
    mympd_state->mpd_state->tag_list = state_file_rw_string_sds(mympd_state->config->workdir, "state", "tag_list", mympd_state->mpd_state->tag_list, vcb_istaglist, false);
    mympd_state->tag_list_search = state_file_rw_string_sds(mympd_state->config->workdir, "state", "tag_list_search", mympd_state->tag_list_search, vcb_istaglist, false);
    mympd_state->tag_list_browse = state_file_rw_string_sds(mympd_state->config->workdir, "state", "tag_list_browse", mympd_state->tag_list_browse, vcb_istaglist, false);
    mympd_state->smartpls = state_file_rw_bool(mympd_state->config->workdir, "state", "smartpls", mympd_state->smartpls, false);
    mympd_state->smartpls_sort = state_file_rw_string_sds(mympd_state->config->workdir, "state", "smartpls_sort", mympd_state->smartpls_sort, vcb_ismpdsort, false);
    mympd_state->smartpls_prefix = state_file_rw_string_sds(mympd_state->config->workdir, "state", "smartpls_prefix", mympd_state->smartpls_prefix, vcb_isname, false);
    mympd_state->smartpls_interval = state_file_rw_int(mympd_state->config->workdir, "state", "smartpls_interval", (int)mympd_state->smartpls_interval, TIMER_INTERVAL_MIN, TIMER_INTERVAL_MAX, false);
    mympd_state->smartpls_generate_tag_list = state_file_rw_string_sds(mympd_state->config->workdir, "state", "smartpls_generate_tag_list", mympd_state->smartpls_generate_tag_list, vcb_istaglist, false);
    mympd_state->last_played_count = state_file_rw_long(mympd_state->config->workdir, "state", "last_played_count", mympd_state->last_played_count, 0, MPD_PLAYLIST_LENGTH_MAX, false);
    mympd_state->auto_play = state_file_rw_bool(mympd_state->config->workdir, "state", "auto_play", mympd_state->auto_play, false);
    mympd_state->jukebox_mode = state_file_rw_uint(mympd_state->config->workdir, "state", "jukebox_mode", mympd_state->jukebox_mode, 0, 2, false);
    mympd_state->jukebox_playlist = state_file_rw_string_sds(mympd_state->config->workdir, "state", "jukebox_playlist", mympd_state->jukebox_playlist, vcb_isfilename, false);
    mympd_state->jukebox_queue_length = state_file_rw_long(mympd_state->config->workdir, "state", "jukebox_queue_length", mympd_state->jukebox_queue_length, 0, JUKEBOX_QUEUE_MAX, false);
    mympd_state->jukebox_last_played = state_file_rw_long(mympd_state->config->workdir, "state", "jukebox_last_played", mympd_state->jukebox_last_played, 0, JUKEBOX_LAST_PLAYED_MAX, false);
    mympd_state->jukebox_unique_tag.tags[0] = state_file_rw_int(mympd_state->config->workdir, "state", "jukebox_unique_tag", mympd_state->jukebox_unique_tag.tags[0], 0, 64, false);
    mympd_state->cols_queue_current = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_queue_current", mympd_state->cols_queue_current, vcb_isname, false);
    mympd_state->cols_search = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_search", mympd_state->cols_search, vcb_isname, false);
    mympd_state->cols_browse_database_detail = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_browse_database_detail", mympd_state->cols_browse_database_detail, vcb_isname, false);
    mympd_state->cols_browse_playlists_detail = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_browse_playlists_detail", mympd_state->cols_browse_playlists_detail, vcb_isname, false);
    mympd_state->cols_browse_filesystem = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_browse_filesystem", mympd_state->cols_browse_filesystem, vcb_isname, false);
    mympd_state->cols_playback = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_playback", mympd_state->cols_playback, vcb_isname, false);
    mympd_state->cols_queue_last_played = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_queue_last_played", mympd_state->cols_queue_last_played, vcb_isname, false);
    mympd_state->cols_queue_jukebox = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_queue_jukebox", mympd_state->cols_queue_jukebox, vcb_isname, false);
    mympd_state->cols_browse_radio_webradiodb = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_browse_radio_webradiodb", mympd_state->cols_browse_radio_webradiodb, vcb_isname, false);
    mympd_state->cols_browse_radio_radiobrowser = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_browse_radio_radiobrowser", mympd_state->cols_browse_radio_radiobrowser, vcb_isname, false);
    mympd_state->coverimage_names = state_file_rw_string_sds(mympd_state->config->workdir, "state", "coverimage_names", mympd_state->coverimage_names, vcb_isfilename, false);
    mympd_state->thumbnail_names = state_file_rw_string_sds(mympd_state->config->workdir, "state", "thumbnail_names", mympd_state->thumbnail_names, vcb_isfilename, false);
    mympd_state->music_directory = state_file_rw_string_sds(mympd_state->config->workdir, "state", "music_directory", mympd_state->music_directory, vcb_isfilepath, false);
    mympd_state->playlist_directory = state_file_rw_string_sds(mympd_state->config->workdir, "state", "playlist_directory", mympd_state->playlist_directory, vcb_isfilepath, false);
    mympd_state->booklet_name = state_file_rw_string_sds(mympd_state->config->workdir, "state", "booklet_name", mympd_state->booklet_name, vcb_isfilename, false);
    mympd_state->volume_min = state_file_rw_uint(mympd_state->config->workdir, "state", "volume_min", mympd_state->volume_min, VOLUME_MIN, VOLUME_MAX, false);
    mympd_state->volume_max = state_file_rw_uint(mympd_state->config->workdir, "state", "volume_max", mympd_state->volume_max, VOLUME_MIN, VOLUME_MAX, false);
    mympd_state->volume_step = state_file_rw_uint(mympd_state->config->workdir, "state", "volume_step", mympd_state->volume_step, VOLUME_STEP_MIN, VOLUME_STEP_MAX, false);
    mympd_state->webui_settings = state_file_rw_string_sds(mympd_state->config->workdir, "state", "webui_settings", mympd_state->webui_settings, validate_json, false);
    mympd_state->mpd_stream_port = state_file_rw_uint(mympd_state->config->workdir, "state", "mpd_stream_port", mympd_state->mpd_stream_port, MPD_PORT_MIN, MPD_PORT_MAX, false);
    mympd_state->lyrics_uslt_ext = state_file_rw_string_sds(mympd_state->config->workdir, "state", "lyrics_uslt_ext", mympd_state->lyrics_uslt_ext, vcb_isalnum, false);
    mympd_state->lyrics_sylt_ext = state_file_rw_string_sds(mympd_state->config->workdir, "state", "lyrics_sylt_ext", mympd_state->lyrics_sylt_ext, vcb_isalnum, false);
    mympd_state->lyrics_vorbis_uslt = state_file_rw_string_sds(mympd_state->config->workdir, "state", "lyrics_vorbis_uslt", mympd_state->lyrics_vorbis_uslt, vcb_isalnum, false);
    mympd_state->lyrics_vorbis_sylt = state_file_rw_string_sds(mympd_state->config->workdir, "state", "lyrics_vorbis_sylt", mympd_state->lyrics_vorbis_sylt, vcb_isalnum, false);
    mympd_state->covercache_keep_days = state_file_rw_int(mympd_state->config->workdir, "state", "covercache_keep_days", mympd_state->covercache_keep_days, COVERCACHE_AGE_MIN, COVERCACHE_AGE_MAX, false);
    mympd_state->listenbrainz_token = state_file_rw_string_sds(mympd_state->config->workdir, "state", "listenbrainz_token", mympd_state->listenbrainz_token, vcb_isalnum, false);

    sds_strip_slash(mympd_state->music_directory);
    sds_strip_slash(mympd_state->playlist_directory);
    mympd_state->navbar_icons = read_navbar_icons(mympd_state->config);
}

sds mympd_api_settings_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = tojson_char(buffer, "mympdVersion", MYMPD_VERSION, true);
    buffer = tojson_char(buffer, "mpdHost", mympd_state->mpd_state->mpd_host, true);
    buffer = tojson_uint(buffer, "mpdPort", mympd_state->mpd_state->mpd_port, true);
    buffer = tojson_char(buffer, "mpdPass", "dontsetpassword", true);
    buffer = tojson_uint(buffer, "mpdStreamPort", mympd_state->mpd_stream_port, true);
    buffer = tojson_uint(buffer, "mpdTimeout", mympd_state->mpd_state->mpd_timeout, true);
    buffer = tojson_bool(buffer, "mpdKeepalive", mympd_state->mpd_state->mpd_keepalive, true);
    buffer = tojson_uint(buffer, "mpdBinarylimit", mympd_state->mpd_state->mpd_binarylimit, true);
#ifdef ENABLE_SSL
    buffer = tojson_bool(buffer, "pin", (sdslen(mympd_state->config->pin_hash) == 0 ? false : true), true);
    buffer = tojson_bool(buffer, "featCacert", (mympd_state->config->custom_cert == false && mympd_state->config->ssl == true ? true : false), true);
#else
    buffer = tojson_bool(buffer, "pin", false, true);
    buffer = tojson_bool(buffer, "featCacert", false, true);
#endif
#ifdef ENABLE_LUA
    buffer = tojson_bool(buffer, "featScripting", true, true);
#else
    buffer = tojson_bool(buffer, "featScripting", false, true);
#endif
    const char *jukebox_mode_str = mympd_lookup_jukebox_mode(mympd_state->jukebox_mode);
    buffer = tojson_char(buffer, "jukeboxMode", jukebox_mode_str, true);

    buffer = tojson_char(buffer, "coverimageNames", mympd_state->coverimage_names, true);
    buffer = tojson_char(buffer, "thumbnailNames", mympd_state->thumbnail_names, true);
    buffer = tojson_char(buffer, "jukeboxPlaylist", mympd_state->jukebox_playlist, true);
    buffer = tojson_long(buffer, "jukeboxQueueLength", mympd_state->jukebox_queue_length, true);
    buffer = tojson_char(buffer, "jukeboxUniqueTag", mpd_tag_name(mympd_state->jukebox_unique_tag.tags[0]), true);
    buffer = tojson_long(buffer, "jukeboxLastPlayed", mympd_state->jukebox_last_played, true);
    buffer = tojson_bool(buffer, "autoPlay", mympd_state->auto_play, true);
    buffer = tojson_int(buffer, "loglevel", loglevel, true);
    buffer = tojson_bool(buffer, "smartpls", mympd_state->smartpls, true);
    buffer = tojson_char(buffer, "smartplsSort", mympd_state->smartpls_sort, true);
    buffer = tojson_char(buffer, "smartplsPrefix", mympd_state->smartpls_prefix, true);
    buffer = tojson_llong(buffer, "smartplsInterval", (long long)mympd_state->smartpls_interval, true);
    buffer = tojson_long(buffer, "lastPlayedCount", mympd_state->last_played_count, true);
    buffer = tojson_char(buffer, "musicDirectory", mympd_state->music_directory, true);
    buffer = tojson_char(buffer, "playlistDirectory", mympd_state->playlist_directory, true);
    buffer = tojson_char(buffer, "bookletName", mympd_state->booklet_name, true);
    buffer = tojson_uint(buffer, "volumeMin", mympd_state->volume_min, true);
    buffer = tojson_uint(buffer, "volumeMax", mympd_state->volume_max, true);
    buffer = tojson_uint(buffer, "volumeStep", mympd_state->volume_step, true);
    buffer = tojson_char(buffer, "lyricsUsltExt", mympd_state->lyrics_uslt_ext, true);
    buffer = tojson_char(buffer, "lyricsSyltExt", mympd_state->lyrics_sylt_ext, true);
    buffer = tojson_char(buffer, "lyricsVorbisUslt", mympd_state->lyrics_vorbis_uslt, true);
    buffer = tojson_char(buffer, "lyricsVorbisSylt", mympd_state->lyrics_vorbis_sylt, true);
    buffer = tojson_int(buffer, "covercacheKeepDays", mympd_state->covercache_keep_days, true);
    buffer = tojson_raw(buffer, "colsQueueCurrent", mympd_state->cols_queue_current, true);
    buffer = tojson_raw(buffer, "colsSearch", mympd_state->cols_search, true);
    buffer = tojson_raw(buffer, "colsBrowseDatabaseDetail", mympd_state->cols_browse_database_detail, true);
    buffer = tojson_raw(buffer, "colsBrowsePlaylistsDetail", mympd_state->cols_browse_playlists_detail, true);
    buffer = tojson_raw(buffer, "colsBrowseFilesystem", mympd_state->cols_browse_filesystem, true);
    buffer = tojson_raw(buffer, "colsPlayback", mympd_state->cols_playback, true);
    buffer = tojson_raw(buffer, "colsQueueLastPlayed", mympd_state->cols_queue_last_played, true);
    buffer = tojson_raw(buffer, "colsQueueJukebox", mympd_state->cols_queue_jukebox, true);
    buffer = tojson_raw(buffer, "colsBrowseRadioWebradiodb", mympd_state->cols_browse_radio_webradiodb, true);
    buffer = tojson_raw(buffer, "colsBrowseRadioRadiobrowser", mympd_state->cols_browse_radio_radiobrowser, true);
    buffer = tojson_raw(buffer, "navbarIcons", mympd_state->navbar_icons, true);
    buffer = tojson_char(buffer, "listenbrainzToken", mympd_state->listenbrainz_token, true);
    buffer = tojson_raw(buffer, "webuiSettings", mympd_state->webui_settings, true);
    if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
        buffer = tojson_bool(buffer, "mpdConnected", true, true);
        struct mpd_status *status = mpd_run_status(mympd_state->mpd_state->conn);
        if (status == NULL) {
            buffer = check_error_and_recover(mympd_state->mpd_state, buffer, method, request_id);
            return buffer;
        }

        enum mpd_replay_gain_mode replay_gain_mode = mpd_run_replay_gain_status(mympd_state->mpd_state->conn);
        if (replay_gain_mode == MPD_REPLAY_UNKNOWN) {
            if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
                mpd_status_free(status);
                return buffer;
            }
        }
        const char *replaygain = mpd_lookup_replay_gain_mode(replay_gain_mode);
        buffer = tojson_char(buffer, "replaygain", replaygain == NULL ? "" : replaygain, true);

        enum mpd_single_state single_state = mpd_status_get_single_state(status);
        buffer = tojson_char(buffer, "single", mpd_lookup_single_state(single_state), true);

        if (mympd_state->mpd_state->feat_mpd_partitions == true) {
            buffer = tojson_char(buffer, "partition", mpd_status_get_partition(status), true);
        }
        buffer = tojson_uint(buffer, "crossfade", mpd_status_get_crossfade(status), true);
        buffer = tojson_double(buffer, "mixrampDb", mpd_status_get_mixrampdb(status), true);
        buffer = tojson_double(buffer, "mixrampDelay", mpd_status_get_mixrampdelay(status), true);
        buffer = tojson_bool(buffer, "repeat", mpd_status_get_repeat(status), true);
        buffer = tojson_bool(buffer, "random", mpd_status_get_random(status), true);
        buffer = tojson_bool(buffer, "consume", mpd_status_get_consume(status), true);

        mpd_status_free(status);

        buffer = tojson_bool(buffer, "featPlaylists", mympd_state->mpd_state->feat_mpd_playlists, true);
        buffer = tojson_bool(buffer, "featTags", mympd_state->mpd_state->feat_mpd_tags, true);
        buffer = tojson_bool(buffer, "featLibrary", mympd_state->mpd_state->feat_mpd_library, true);
        buffer = tojson_bool(buffer, "featAdvsearch", mympd_state->mpd_state->feat_mpd_advsearch, true);
        buffer = tojson_bool(buffer, "featStickers", mympd_state->mpd_state->feat_mpd_stickers, true);
        buffer = tojson_bool(buffer, "featFingerprint", mympd_state->mpd_state->feat_mpd_fingerprint, true);
        buffer = tojson_bool(buffer, "featSingleOneshot", mympd_state->mpd_state->feat_mpd_single_oneshot, true);
        buffer = tojson_bool(buffer, "featPartitions", mympd_state->mpd_state->feat_mpd_partitions, true);
        buffer = tojson_char(buffer, "musicDirectoryValue", mympd_state->music_directory_value, true);
        buffer = tojson_bool(buffer, "featMounts", mympd_state->mpd_state->feat_mpd_mount, true);
        buffer = tojson_bool(buffer, "featNeighbors", mympd_state->mpd_state->feat_mpd_neighbor, true);
        buffer = tojson_bool(buffer, "featBinarylimit", mympd_state->mpd_state->feat_mpd_binarylimit, true);
        buffer = tojson_bool(buffer, "featSmartpls", mympd_state->mpd_state->feat_mpd_smartpls, true);
        buffer = tojson_bool(buffer, "featPlaylistRmRange", mympd_state->mpd_state->feat_mpd_playlist_rm_range, true);
        buffer = tojson_bool(buffer, "featWhence", mympd_state->mpd_state->feat_mpd_whence, true);
        buffer = tojson_bool(buffer, "featAdvqueue", mympd_state->mpd_state->feat_mpd_advqueue, true);

        buffer = print_tags_array(buffer, "tagList", mympd_state->mpd_state->tag_types_mympd);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = print_tags_array(buffer, "tagListSearch", mympd_state->tag_types_search);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = print_tags_array(buffer, "tagListBrowse", mympd_state->tag_types_browse);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = print_tags_array(buffer, "tagListMpd", mympd_state->mpd_state->tag_types_mpd);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = print_tags_array(buffer, "smartplsGenerateTagList", mympd_state->smartpls_generate_tag_types);

        buffer = sdscat(buffer, ",\"triggerEvents\":{");
        buffer = mympd_api_trigger_print_trigger_list(buffer);
        buffer = sdscatlen(buffer, "}", 1);

    }
    else {
        buffer = tojson_bool(buffer, "mpdConnected", false, false);
    }
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

sds mympd_api_settings_picture_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, sds type) {
    sds pic_dirname = sdscatfmt(sdsempty(), "%s/pics/%s", mympd_state->config->workdir, type);
    errno = 0;
    DIR *pic_dir = opendir(pic_dirname);
    if (pic_dir == NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true,
            "general", "error", "Can not open directory pics");
        MYMPD_LOG_ERROR("Can not open directory \"%s\"", pic_dirname);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(pic_dirname);
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int returned_entities = 0;
    struct dirent *next_file;
    while ((next_file = readdir(pic_dir)) != NULL ) {
        if (next_file->d_type == DT_REG) {
            const char *ext = strrchr(next_file->d_name, '.');
            if (ext == NULL) {
                continue;
            }
            if (strcasecmp(ext, ".webp") == 0 || strcasecmp(ext, ".jpg") == 0 ||
                strcasecmp(ext, ".jpeg") == 0 || strcasecmp(ext, ".png") == 0 ||
                strcasecmp(ext, ".avif") == 0 || strcasecmp(ext, ".svg") == 0)
            {
                if (returned_entities++) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sds_catjson(buffer, next_file->d_name, strlen(next_file->d_name));
            }
        }
    }
    closedir(pic_dir);
    FREE_SDS(pic_dirname);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "returnedEntities", returned_entities, false);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

//privat functions
static sds set_default_navbar_icons(struct t_config *config, sds buffer) {
    MYMPD_LOG_NOTICE("Writing default navbar_icons");
    sds file_name = sdscatfmt(sdsempty(), "%s/state/navbar_icons", config->workdir);
    sdsclear(buffer);
    buffer = sdscat(buffer, MYMPD_NAVBAR_ICONS);
    errno = 0;
    FILE *fp = fopen(file_name, OPEN_FLAGS_WRITE);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", file_name);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(file_name);
        return buffer;
    }
    if (fputs(buffer, fp) == EOF) {
        MYMPD_LOG_ERROR("Could not write to file \"%s\"", file_name);
    }
    if (fclose(fp) != 0) {
        MYMPD_LOG_ERROR("Could not close file \"%s\"", file_name);
    }
    FREE_SDS(file_name);
    return buffer;
}

static sds read_navbar_icons(struct t_config *config) {
    sds file_name = sdscatfmt(sdsempty(), "%s/state/navbar_icons", config->workdir);
    sds buffer = sdsempty();
    errno = 0;
    FILE *fp = fopen(file_name, OPEN_FLAGS_READ);
    if (fp == NULL) {
        if (errno != ENOENT) {
            MYMPD_LOG_ERROR("Can not open file \"%s\"", file_name);
            MYMPD_LOG_ERRNO(errno);
        }
        buffer = set_default_navbar_icons(config, buffer);
        FREE_SDS(file_name);
        return buffer;
    }
    FREE_SDS(file_name);
    sds_getfile(&buffer, fp, 2000);
    (void) fclose(fp);

    if (validate_json_array(buffer) == false) {
        MYMPD_LOG_ERROR("Invalid navbar icons");
        sdsclear(buffer);
    }

    if (sdslen(buffer) == 0) {
        buffer = set_default_navbar_icons(config, buffer);
    }
    return buffer;
}

static sds print_tags_array(sds buffer, const char *tagsname, struct t_tags tags) {
    buffer = sdscatfmt(buffer, "\"%s\": [", tagsname);
    for (unsigned i = 0; i < tags.len; i++) {
        if (i > 0) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        const char *tagname = mpd_tag_name(tags.tags[i]);
        buffer = sds_catjson(buffer, tagname, strlen(tagname));
    }
    buffer = sdscatlen(buffer, "]", 1);
    return buffer;
}

static sds set_invalid_value(sds error, sds key, sds value) {
    error = sdscatfmt(error, "Invalid value for \"%s\": \"%s\"", key, value);
    MYMPD_LOG_WARN("%s", error);
    return error;
}
