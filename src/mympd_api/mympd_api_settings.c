/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_settings.h"

#include "../../dist/src/mjson/mjson.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"
#include "../lib/state_files.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_client/mpd_client_trigger.h"
#include "../mpd_client/mpd_client_utility.h"
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

//default navbar icons
static const char *default_navbar_icons = "[{\"ligature\":\"home\",\"title\":\"Home\",\"options\":[\"Home\"],\"badge\":\"\"},"\
    "{\"ligature\":\"equalizer\",\"title\":\"Playback\",\"options\":[\"Playback\"],\"badge\":\"\"},"\
    "{\"ligature\":\"queue_music\",\"title\":\"Queue\",\"options\":[\"Queue\"],\"badge\":\"<span id=\\\"badgeQueueItems\\\" class=\\\"badge badge-secondary\\\"></span>\"},"\
    "{\"ligature\":\"library_music\",\"title\":\"Browse\",\"options\":[\"Browse\"],\"badge\":\"\"},"\
    "{\"ligature\":\"search\",\"title\":\"Search\",\"options\":[\"Search\"],\"badge\":\"\"}]";

//public functions
bool mympd_api_connection_save(sds key, sds value, int vtype, validate_callback vcb, void *userdata) {
    (void) vcb;
    struct t_mympd_state *mympd_state = (struct t_mympd_state *)userdata;
    bool check_for_mpd_error = false;

    MYMPD_LOG_DEBUG("Parse setting \"%s\": \"%s\"", key, value);

    if (strcmp(key, "mpdPass") == 0) {
        if (strcmp(value, "dontsetpassword") != 0) {
            if (vcb_isname(value) == false) {
                MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
                return false;
            }
            mympd_state->mpd_state->mpd_pass = sdsreplace(mympd_state->mpd_state->mpd_pass, value);
        }
        else {
            //keep old password
            return true;
        }
    }
    else if (strcmp(key, "mpdHost") == 0) {
        if (vcb_isfilepath(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->mpd_state->mpd_host = sdsreplace(mympd_state->mpd_state->mpd_host, value);
    }
    else if (strcmp(key, "mpdPort") == 0) {
        int mpd_port = (int)strtoimax(value, NULL, 10);
        if (mpd_port < 1024 || mpd_port > 65535) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->mpd_state->mpd_port = mpd_port;
    }
    else if (strcmp(key, "mpdStreamPort") == 0) {
        int mpd_stream_port = (int)strtoimax(value, NULL, 10);
        if (mpd_stream_port < 1024 || mpd_stream_port > 65535) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->mpd_stream_port = mpd_stream_port;
    }
    else if (strcmp(key, "musicDirectory") == 0) {
        if (vcb_isfilepath(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->music_directory = sdsreplace(mympd_state->music_directory, value);
        strip_slash(mympd_state->music_directory);
    }
    else if (strcmp(key, "playlistDirectory") == 0) {
        if (vcb_isfilepath(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->playlist_directory = sdsreplace(mympd_state->playlist_directory, value);
        strip_slash(mympd_state->playlist_directory);
    }
    else if (strcmp(key, "mpdBinarylimit") == 0) {
        unsigned binarylimit = strtoumax(value, NULL, 10);
        if (binarylimit < 4096 || binarylimit > 262144) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        if (binarylimit != mympd_state->mpd_state->mpd_binarylimit) {
            mympd_state->mpd_state->mpd_binarylimit = binarylimit;
            if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                mpd_client_set_binarylimit(mympd_state);
            }
        }
    }
    else if (strcmp(key, "mpdTimeout") == 0) {
        int mpd_timeout = (int)strtoimax(value, NULL, 10);
        if (mpd_timeout < 1000 || mpd_timeout > 1000000) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
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
        bool keepalive = false;
        if (vtype == MJSON_TOK_TRUE) {
            keepalive = true;
        }
        else if (vtype == MJSON_TOK_FALSE) {
            keepalive = false;
        }
        else {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        if (keepalive != mympd_state->mpd_state->mpd_keepalive) {
            mympd_state->mpd_state->mpd_keepalive = keepalive;
            if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                mpd_connection_set_keepalive(mympd_state->mpd_state->conn, mympd_state->mpd_state->mpd_keepalive);
                check_for_mpd_error = true;
            }
        }
    }
    else {
        MYMPD_LOG_WARN("Unknown setting \"%s\": \"%s\"", key, value);
        return true;
    }

    bool rc = true;
    if (check_for_mpd_error == true) {
        sds message = check_error_and_recover_notify(mympd_state->mpd_state, sdsempty());
        if (sdslen(message) > 0) {
            ws_notify(message);
            rc = false;
        }
        sdsfree(message);
    }

    if (rc == true) {
        sds state_filename = camel_to_snake(key);
        rc = state_file_write(mympd_state->config->workdir, "state", state_filename, value);
        sdsfree(state_filename);
    }
    return rc;
}

bool mympd_api_cols_save(struct t_mympd_state *mympd_state, sds table, sds cols) {
    if (strcmp(table, "colsQueueCurrent") == 0) {
        mympd_state->cols_queue_current = sdsreplace(mympd_state->cols_queue_current, cols);
    }
    else if (strcmp(table, "colsQueueLastPlayed") == 0) {
        mympd_state->cols_queue_last_played = sdsreplace(mympd_state->cols_queue_last_played, cols);
    }
    else if (strcmp(table, "colsSearch") == 0) {
        mympd_state->cols_search = sdsreplace(mympd_state->cols_search, cols);
    }
    else if (strcmp(table, "colsBrowseDatabaseDetail") == 0) {
        mympd_state->cols_browse_database_detail = sdsreplace(mympd_state->cols_browse_database_detail, cols);
    }
    else if (strcmp(table, "colsBrowsePlaylistsDetail") == 0) {
        mympd_state->cols_browse_playlists_detail = sdsreplace(mympd_state->cols_browse_playlists_detail, cols);
    }
    else if (strcmp(table, "colsBrowseFilesystem") == 0) {
        mympd_state->cols_browse_filesystem = sdsreplace(mympd_state->cols_browse_filesystem, cols);
    }
    else if (strcmp(table, "colsPlayback") == 0) {
        mympd_state->cols_playback = sdsreplace(mympd_state->cols_playback, cols);
    }
    else if (strcmp(table, "colsQueueJukebox") == 0) {
        mympd_state->cols_queue_jukebox = sdsreplace(mympd_state->cols_queue_jukebox, cols);
    }
    else {
        return false;
    }
    sds tablename = camel_to_snake(table);
    if (!state_file_write(mympd_state->config->workdir, "state", tablename, cols)) {
        sdsfree(tablename);
        return false;
    }
    sdsfree(tablename);
    return true;
}

bool mympd_api_settings_set(sds key, sds value, int vtype, validate_callback vcb, void *userdata) {
    (void) vcb;
    struct t_mympd_state *mympd_state = (struct t_mympd_state *)userdata;

    MYMPD_LOG_DEBUG("Parse setting \"%s\": \"%s\"", key, value);

    if (strcmp(key, "coverimageNames") == 0) {
        if (vcb_isfilename(value) == true) {
            mympd_state->coverimage_names = sdsreplace(mympd_state->coverimage_names, value);
        }
        else {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
    }
    else if (strcmp(key, "bookletName") == 0) {
        if (vcb_isfilename(value) == true) {
            mympd_state->booklet_name = sdsreplace(mympd_state->booklet_name, value);
        }
        else {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
    }
    else if (strcmp(key, "lastPlayedCount") == 0) {
        int last_played_count = (int)strtoimax(value, NULL, 10);
        if (last_played_count <= 0) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->last_played_count = last_played_count;
    }
    else if (strcmp(key, "volumeMin") == 0) {
        int volume_min = (int)strtoimax(value, NULL, 10);
        if (volume_min < 0 || volume_min > 100) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->volume_min = volume_min;
    }
    else if (strcmp(key, "volumeMax") == 0) {
        int volume_max = (int)strtoimax(value, NULL, 10);
        if (volume_max < 0 || volume_max > 100) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->volume_max = volume_max;
    }
    else if (strcmp(key, "volumeStep") == 0) {
        int volume_step = (int)strtoimax(value, NULL, 10);
        if (volume_step < 0 || volume_step > 100) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->volume_step = volume_step;
    }
    else if (strcmp(key, "tagList") == 0) {
        if (vcb_istaglist(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->mpd_state->tag_list = sdsreplace(mympd_state->mpd_state->tag_list, value);
    }
    else if (strcmp(key, "tagListSearch") == 0) {
        if (vcb_istaglist(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->tag_list_search = sdsreplace(mympd_state->tag_list_search, value);
    }
    else if (strcmp(key, "tagListBrowse") == 0) {
        if (vcb_istaglist(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->tag_list_browse = sdsreplace(mympd_state->tag_list_browse, value);
    }
    else if (strcmp(key, "smartpls") == 0) {
        if (vtype == MJSON_TOK_TRUE) {
            mympd_state->smartpls = true;
        }
        else if (vtype == MJSON_TOK_FALSE) {
            mympd_state->smartpls = false;
        }
        else {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
    }
    else if (strcmp(key, "smartplsSort") == 0) {
        if (sdslen(value) > 0 && vcb_ismpdsort(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->smartpls_sort = sdsreplace(mympd_state->smartpls_sort, value);
    }
    else if (strcmp(key, "smartplsPrefix") == 0) {
        if (vcb_isfilename(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->smartpls_prefix = sdsreplacelen(mympd_state->smartpls_prefix, value, sdslen(value));
    }
    else if (strcmp(key, "smartplsInterval") == 0) {
        time_t interval = strtoimax(value, NULL, 10);
        if (interval < 360 || interval > 604800) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        if (interval != mympd_state->smartpls_interval) {
            mympd_state->smartpls_interval = interval;
            replace_timer(&mympd_state->timer_list, interval, (int)interval, timer_handler_smartpls_update, 2, NULL, NULL);
        }
    }
    else if (strcmp(key, "smartplsGenerateTagList") == 0) {
        if (vcb_ismpdtag(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->smartpls_generate_tag_list = sdsreplacelen(mympd_state->smartpls_generate_tag_list, value, sdslen(value));
    }
    else if (strcmp(key, "webuiSettings") == 0) {
        mympd_state->webui_settings = sdsreplacelen(mympd_state->webui_settings, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsUsltExt") == 0) {
        if (vcb_isalnum(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->lyrics_uslt_ext = sdsreplacelen(mympd_state->lyrics_uslt_ext, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsSyltExt") == 0) {
        if (vcb_isalnum(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->lyrics_sylt_ext = sdsreplacelen(mympd_state->lyrics_sylt_ext, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsVorbisUslt") == 0) {
        if (vcb_isalnum(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->lyrics_vorbis_uslt = sdsreplacelen(mympd_state->lyrics_vorbis_uslt, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsVorbisSylt") == 0) {
        if (vcb_isalnum(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->lyrics_vorbis_sylt = sdsreplacelen(mympd_state->lyrics_vorbis_sylt, value, sdslen(value));
    }
    else if (strcmp(key, "covercacheKeepDays") == 0) {
        int covercache_keep_days = (int)strtoimax(value, NULL, 10);
        if (covercache_keep_days < 0 || covercache_keep_days > 365) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->covercache_keep_days = covercache_keep_days;
    }
    else {
        MYMPD_LOG_WARN("Unknown setting \"%s\": \"%s\"", key, value);
        return true;
    }
    sds state_filename = camel_to_snake(key);
    bool rc = state_file_write(mympd_state->config->workdir, "state", state_filename, value);
    sdsfree(state_filename);
    return rc;
}

bool mpdclient_api_options_set(sds key, sds value, int vtype, validate_callback vcb, void *userdata) {
    (void) vcb;
    struct t_mympd_state *mympd_state = (struct t_mympd_state *)userdata;

    bool rc = false;
    bool write_state_file = true;
    bool jukebox_changed = false;

    MYMPD_LOG_DEBUG("Parse setting \"%s\": \"%s\"", key, value);
    if (strcmp(key, "autoPlay") == 0) {
        if (vtype == MJSON_TOK_TRUE) {
            mympd_state->auto_play = true;
        }
        else if (vtype == MJSON_TOK_FALSE) {
            mympd_state->auto_play = false;
        }
        else {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
    }
    else if (strcmp(key, "jukeboxMode") == 0) {
        unsigned jukebox_mode = strtoumax(value, NULL, 10);
        if (jukebox_mode > 2) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        if (mympd_state->jukebox_mode != jukebox_mode) {
            mympd_state->jukebox_mode = jukebox_mode;
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxPlaylist") == 0) {
        if (vcb_isfilename(value) == false) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        if (strcmp(mympd_state->jukebox_playlist, value) != 0) {
            mympd_state->jukebox_playlist = sdsreplace(mympd_state->jukebox_playlist, value);
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxQueueLength") == 0) {
        int jukebox_queue_length = (int)strtoimax(value, NULL, 10);
        if (jukebox_queue_length <= 0 || jukebox_queue_length > 999) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        mympd_state->jukebox_queue_length = jukebox_queue_length;
    }
    else if (strcmp(key, "jukeboxUniqueTag") == 0) {
        enum mpd_tag_type unique_tag = mpd_tag_name_parse(value);
        if (unique_tag == MPD_TAG_UNKNOWN) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        if (mympd_state->jukebox_unique_tag.tags[0] != unique_tag) {
            mympd_state->jukebox_unique_tag.tags[0] = unique_tag;
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxLastPlayed") == 0) {
        int jukebox_last_played = (int)strtoimax(value, NULL, 10);
        if (jukebox_last_played < 0 || jukebox_last_played > 5000) {
            MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
            return false;
        }
        if (jukebox_last_played != mympd_state->jukebox_last_played) {
            mympd_state->jukebox_last_played = jukebox_last_played;
            jukebox_changed = true;
        }
    }
    else if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
        if (strcmp(key, "random") == 0) {
            unsigned uint_buf = 0;
            if (value[0] == '0') { uint_buf = 0; }
            else if (value[0] == '1') { uint_buf = 1; }
            else {
                MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
                return false;
            }
            rc = mpd_run_random(mympd_state->mpd_state->conn, uint_buf);
        }
        else if (strcmp(key, "repeat") == 0) {
            unsigned uint_buf = 0;
            if (value[0] == '0') { uint_buf = 0; }
            else if (value[0] == '1') { uint_buf = 1; }
            else {
                MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
                return false;
            }
            rc = mpd_run_repeat(mympd_state->mpd_state->conn, uint_buf);
        }
        else if (strcmp(key, "consume") == 0) {
            unsigned uint_buf = 0;
            if (value[0] == '0') { uint_buf = 0; }
            else if (value[0] == '1') { uint_buf = 1; }
            else {
                MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
                return false;
            }
            rc = mpd_run_consume(mympd_state->mpd_state->conn, uint_buf);
        }
        else if (strcmp(key, "single") == 0) {
            if (mympd_state->mpd_state->feat_single_oneshot == true) {
                enum mpd_single_state state;
                if (value[0] == '0') { state = MPD_SINGLE_OFF; }
                else if (value[0] == '1') { state = MPD_SINGLE_ON; }
                else if (value[0] == '2') { state = MPD_SINGLE_ONESHOT; }
                else { 
                    MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
                    return false;
                }
                rc = mpd_run_single_state(mympd_state->mpd_state->conn, state);
            }
            else {
                unsigned uint_buf = 0;
                if (value[0] == '0') { uint_buf = 0; }
                else if (value[0] == '1') { uint_buf = 1; }
                else { 
                    MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
                    return false;
                }
                rc = mpd_run_single(mympd_state->mpd_state->conn, uint_buf);
            }
        }
        else if (strcmp(key, "crossfade") == 0) {
            unsigned uint_buf = strtoumax(value, NULL, 10);
            if (uint_buf > 100) {
                MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
                return false;
            }
            rc = mpd_run_crossfade(mympd_state->mpd_state->conn, uint_buf);
        }
        else if (strcmp(key, "replaygain") == 0) {
            enum mpd_replay_gain_mode mode = mpd_parse_replay_gain_name(value);
            if (mode == MPD_REPLAY_UNKNOWN) {
                MYMPD_LOG_WARN("Invalid value for \"%s\": \"%s\"", key, value);
                return false;
            }
            rc = mpd_run_replay_gain_mode(mympd_state->mpd_state->conn, mode);
        }
        sds message = check_error_and_recover_notify(mympd_state->mpd_state, sdsempty());
        if (sdslen(message) > 0) {
            ws_notify(message);
            rc = false;
        }
        sdsfree(message);
        write_state_file = false;
    }
    else {
        MYMPD_LOG_WARN("Unknown setting \"%s\": \"%s\"", key, value);
        return false;
    }
    if (jukebox_changed == true && mympd_state->jukebox_queue.length > 0) {
        MYMPD_LOG_INFO("Jukebox options changed, clearing jukebox queue");
        list_free(&mympd_state->jukebox_queue);
    }
    if (write_state_file == true) {
        sds state_filename = camel_to_snake(key);
        rc = state_file_write(mympd_state->config->workdir, "state", state_filename, value);
        sdsfree(state_filename);
    }
    return rc;
}

void mympd_api_read_statefiles(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_NOTICE("Reading states");
    mympd_state->mpd_state->mpd_host = state_file_rw_string_sds(mympd_state->config->workdir, "state", "mpd_host", mympd_state->mpd_state->mpd_host, false);
    mympd_state->mpd_state->mpd_port = state_file_rw_int(mympd_state->config->workdir, "state", "mpd_port", mympd_state->mpd_state->mpd_port, false);
    mympd_state->mpd_state->mpd_pass = state_file_rw_string_sds(mympd_state->config->workdir, "state", "mpd_pass", mympd_state->mpd_state->mpd_pass, false);
    mympd_state->mpd_state->mpd_binarylimit = state_file_rw_uint(mympd_state->config->workdir, "state", "mpd_binarylimit", mympd_state->mpd_state->mpd_binarylimit, false);
    mympd_state->mpd_state->mpd_timeout = state_file_rw_int(mympd_state->config->workdir, "state", "mpd_timeout", mympd_state->mpd_state->mpd_timeout, false);
    mympd_state->mpd_state->mpd_keepalive = state_file_rw_bool(mympd_state->config->workdir, "state", "mpd_keepalive", mympd_state->mpd_state->mpd_keepalive, false);
    mympd_state->mpd_state->tag_list = state_file_rw_string_sds(mympd_state->config->workdir, "state", "tag_list", mympd_state->mpd_state->tag_list, false);
    mympd_state->tag_list_search = state_file_rw_string_sds(mympd_state->config->workdir, "state", "tag_list_search", mympd_state->tag_list_search, false);
    mympd_state->tag_list_browse = state_file_rw_string_sds(mympd_state->config->workdir, "state", "tag_list_browse", mympd_state->tag_list_browse, false);
    mympd_state->smartpls = state_file_rw_bool(mympd_state->config->workdir, "state", "smartpls", mympd_state->smartpls, false);
    mympd_state->smartpls_sort = state_file_rw_string_sds(mympd_state->config->workdir, "state", "smartpls_sort", mympd_state->smartpls_sort, false);
    mympd_state->smartpls_prefix = state_file_rw_string_sds(mympd_state->config->workdir, "state", "smartpls_prefix", mympd_state->smartpls_prefix, false);
    mympd_state->smartpls_interval = state_file_rw_int(mympd_state->config->workdir, "state", "smartpls_interval", (int)mympd_state->smartpls_interval, false);
    mympd_state->smartpls_generate_tag_list = state_file_rw_string_sds(mympd_state->config->workdir, "state", "smartpls_generate_tag_list", mympd_state->smartpls_generate_tag_list, false);
    mympd_state->last_played_count = state_file_rw_uint(mympd_state->config->workdir, "state", "last_played_count", mympd_state->last_played_count, false);
    mympd_state->auto_play = state_file_rw_bool(mympd_state->config->workdir, "state", "auto_play", mympd_state->auto_play, false);
    mympd_state->jukebox_mode = state_file_rw_int(mympd_state->config->workdir, "state", "jukebox_mode", mympd_state->jukebox_mode, false);
    mympd_state->jukebox_playlist = state_file_rw_string_sds(mympd_state->config->workdir, "state", "jukebox_playlist", mympd_state->jukebox_playlist, false);
    mympd_state->jukebox_queue_length = state_file_rw_uint(mympd_state->config->workdir, "state", "jukebox_queue_length", mympd_state->jukebox_queue_length, false);
    mympd_state->jukebox_last_played = state_file_rw_int(mympd_state->config->workdir, "state", "jukebox_last_played", mympd_state->jukebox_last_played, false);
    mympd_state->jukebox_unique_tag.tags[0] = state_file_rw_int(mympd_state->config->workdir, "state", "jukebox_unique_tag", mympd_state->jukebox_unique_tag.tags[0], false);
    mympd_state->cols_queue_current = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_queue_current", mympd_state->cols_queue_current, false);
    mympd_state->cols_search = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_search", mympd_state->cols_search, false);
    mympd_state->cols_browse_database_detail = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_browse_database_detail", mympd_state->cols_browse_database_detail, false);
    mympd_state->cols_browse_playlists_detail = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_browse_playlists_detail", mympd_state->cols_browse_playlists_detail, false);
    mympd_state->cols_browse_filesystem = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_browse_filesystem", mympd_state->cols_browse_filesystem, false);
    mympd_state->cols_playback = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_playback", mympd_state->cols_playback, false);
    mympd_state->cols_queue_last_played = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_queue_last_played", mympd_state->cols_queue_last_played, false);
    mympd_state->cols_queue_jukebox = state_file_rw_string_sds(mympd_state->config->workdir, "state", "cols_queue_jukebox", mympd_state->cols_queue_jukebox, false);
    mympd_state->coverimage_names = state_file_rw_string_sds(mympd_state->config->workdir, "state", "coverimage_names", mympd_state->coverimage_names, false);
    mympd_state->music_directory = state_file_rw_string_sds(mympd_state->config->workdir, "state", "music_directory", mympd_state->music_directory, false);
    mympd_state->playlist_directory = state_file_rw_string_sds(mympd_state->config->workdir, "state", "playlist_directory", mympd_state->playlist_directory, false);
    mympd_state->booklet_name = state_file_rw_string_sds(mympd_state->config->workdir, "state", "booklet_name", mympd_state->booklet_name, false);
    mympd_state->volume_min = state_file_rw_uint(mympd_state->config->workdir, "state", "volume_min", mympd_state->volume_min, false);
    mympd_state->volume_max = state_file_rw_uint(mympd_state->config->workdir, "state", "volume_max", mympd_state->volume_max, false);
    mympd_state->volume_step = state_file_rw_uint(mympd_state->config->workdir, "state", "volume_step", mympd_state->volume_step, false);
    mympd_state->webui_settings = state_file_rw_string_sds(mympd_state->config->workdir, "state", "webui_settings", mympd_state->webui_settings, false);
    mympd_state->mpd_stream_port = state_file_rw_int(mympd_state->config->workdir, "state", "mpd_stream_port", mympd_state->mpd_stream_port, false);
    mympd_state->lyrics_uslt_ext = state_file_rw_string_sds(mympd_state->config->workdir, "state", "lyrics_uslt_ext", mympd_state->lyrics_uslt_ext, false);
    mympd_state->lyrics_sylt_ext = state_file_rw_string_sds(mympd_state->config->workdir, "state", "lyrics_sylt_ext", mympd_state->lyrics_sylt_ext, false);
    mympd_state->lyrics_vorbis_uslt = state_file_rw_string_sds(mympd_state->config->workdir, "state", "lyrics_vorbis_uslt", mympd_state->lyrics_vorbis_uslt, false);
    mympd_state->lyrics_vorbis_sylt = state_file_rw_string_sds(mympd_state->config->workdir, "state", "lyrics_vorbis_sylt", mympd_state->lyrics_vorbis_sylt, false);
    mympd_state->covercache_keep_days = state_file_rw_int(mympd_state->config->workdir, "state", "covercache_keep_days", mympd_state->covercache_keep_days, false);

    strip_slash(mympd_state->music_directory);
    strip_slash(mympd_state->playlist_directory);
    mympd_state->navbar_icons = read_navbar_icons(mympd_state->config);
}

sds mympd_api_settings_put(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = tojson_char(buffer, "mympdVersion", MYMPD_VERSION, true);
    buffer = tojson_char(buffer, "mpdHost", mympd_state->mpd_state->mpd_host, true);
    buffer = tojson_long(buffer, "mpdPort", mympd_state->mpd_state->mpd_port, true);
    buffer = tojson_char(buffer, "mpdPass", "dontsetpassword", true);
    buffer = tojson_long(buffer, "mpdStreamPort", mympd_state->mpd_stream_port, true);
    buffer = tojson_long(buffer, "mpdTimeout", mympd_state->mpd_state->mpd_timeout, true);
    buffer = tojson_bool(buffer, "mpdKeepalive", mympd_state->mpd_state->mpd_keepalive, true);
    buffer = tojson_long(buffer, "mpdBinarylimit", mympd_state->mpd_state->mpd_binarylimit, true);
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
    buffer = tojson_char(buffer, "coverimageNames", mympd_state->coverimage_names, true);
    buffer = tojson_long(buffer, "jukeboxMode", mympd_state->jukebox_mode, true);
    buffer = tojson_char(buffer, "jukeboxPlaylist", mympd_state->jukebox_playlist, true);
    buffer = tojson_long(buffer, "jukeboxQueueLength", mympd_state->jukebox_queue_length, true);
    buffer = tojson_char(buffer, "jukeboxUniqueTag", mpd_tag_name(mympd_state->jukebox_unique_tag.tags[0]), true);
    buffer = tojson_long(buffer, "jukeboxLastPlayed", mympd_state->jukebox_last_played, true);
    buffer = tojson_bool(buffer, "autoPlay", mympd_state->auto_play, true);
    buffer = tojson_long(buffer, "loglevel", loglevel, true);
    buffer = tojson_bool(buffer, "smartpls", mympd_state->smartpls, true);
    buffer = tojson_char(buffer, "smartplsSort", mympd_state->smartpls_sort, true);
    buffer = tojson_char(buffer, "smartplsPrefix", mympd_state->smartpls_prefix, true);
    buffer = tojson_long(buffer, "smartplsInterval", mympd_state->smartpls_interval, true);
    buffer = tojson_long(buffer, "lastPlayedCount", mympd_state->last_played_count, true);
    buffer = tojson_char(buffer, "musicDirectory", mympd_state->music_directory, true);
    buffer = tojson_char(buffer, "playlistDirectory", mympd_state->playlist_directory, true);
    buffer = tojson_char(buffer, "bookletName", mympd_state->booklet_name, true);
    buffer = tojson_long(buffer, "volumeMin", mympd_state->volume_min, true);
    buffer = tojson_long(buffer, "volumeMax", mympd_state->volume_max, true);
    buffer = tojson_long(buffer, "volumeStep", mympd_state->volume_step, true);
    buffer = tojson_char(buffer, "lyricsUsltExt", mympd_state->lyrics_uslt_ext, true);
    buffer = tojson_char(buffer, "lyricsSyltExt", mympd_state->lyrics_sylt_ext, true);
    buffer = tojson_char(buffer, "lyricsVorbisUslt", mympd_state->lyrics_vorbis_uslt, true);
    buffer = tojson_char(buffer, "lyricsVorbisSylt", mympd_state->lyrics_vorbis_sylt, true);
    buffer = tojson_long(buffer, "covercacheKeepDays", mympd_state->covercache_keep_days, true);
    buffer = sdscatfmt(buffer, "\"colsQueueCurrent\":%s,", mympd_state->cols_queue_current);
    buffer = sdscatfmt(buffer, "\"colsSearch\":%s,", mympd_state->cols_search);
    buffer = sdscatfmt(buffer, "\"colsBrowseDatabaseDetail\":%s,", mympd_state->cols_browse_database_detail);
    buffer = sdscatfmt(buffer, "\"colsBrowsePlaylistsDetail\":%s,", mympd_state->cols_browse_playlists_detail);
    buffer = sdscatfmt(buffer, "\"colsBrowseFilesystem\":%s,", mympd_state->cols_browse_filesystem);
    buffer = sdscatfmt(buffer, "\"colsPlayback\":%s,", mympd_state->cols_playback);
    buffer = sdscatfmt(buffer, "\"colsQueueLastPlayed\":%s,", mympd_state->cols_queue_last_played);
    buffer = sdscatfmt(buffer, "\"colsQueueJukebox\":%s,", mympd_state->cols_queue_jukebox);
    buffer = sdscatfmt(buffer, "\"navbarIcons\":%s,", mympd_state->navbar_icons);
    buffer = sdscatfmt(buffer, "\"webuiSettings\":%s,", mympd_state->webui_settings);
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
        
        buffer = tojson_long(buffer, "repeat", mpd_status_get_repeat(status), true);
        if (mympd_state->mpd_state->feat_single_oneshot == true) {
            buffer = tojson_long(buffer, "single", mpd_status_get_single_state(status), true);
        }
        else {
            buffer = tojson_long(buffer, "single", mpd_status_get_single(status), true);
        }
        if (mympd_state->mpd_state->feat_mpd_partitions == true) {
            buffer = tojson_char(buffer, "partition", mpd_status_get_partition(status), true);
        }
        buffer = tojson_long(buffer, "crossfade", mpd_status_get_crossfade(status), true);
        buffer = tojson_long(buffer, "random", mpd_status_get_random(status), true);
        buffer = tojson_long(buffer, "consume", mpd_status_get_consume(status), true);
        buffer = tojson_char(buffer, "replaygain", replaygain == NULL ? "" : replaygain, true);
        mpd_status_free(status);
        
        buffer = tojson_bool(buffer, "featPlaylists", mympd_state->mpd_state->feat_playlists, true);
        buffer = tojson_bool(buffer, "featTags", mympd_state->mpd_state->feat_tags, true);
        buffer = tojson_bool(buffer, "featLibrary", mympd_state->mpd_state->feat_library, true);
        buffer = tojson_bool(buffer, "featAdvsearch", mympd_state->mpd_state->feat_advsearch, true);
        buffer = tojson_bool(buffer, "featStickers", mympd_state->mpd_state->feat_stickers, true);
        buffer = tojson_bool(buffer, "featFingerprint", mympd_state->mpd_state->feat_fingerprint, true);
        buffer = tojson_bool(buffer, "featSingleOneshot", mympd_state->mpd_state->feat_single_oneshot, true);
        buffer = tojson_bool(buffer, "featPartitions", mympd_state->mpd_state->feat_mpd_partitions, true);
        buffer = tojson_char(buffer, "musicDirectoryValue", mympd_state->music_directory_value, true);
        buffer = tojson_bool(buffer, "featMounts", mympd_state->mpd_state->feat_mpd_mount, true);
        buffer = tojson_bool(buffer, "featNeighbors", mympd_state->mpd_state->feat_mpd_neighbor, true);
        buffer = tojson_bool(buffer, "featBinarylimit", mympd_state->mpd_state->feat_mpd_binarylimit, true);
        buffer = tojson_bool(buffer, "featSmartpls", mympd_state->mpd_state->feat_smartpls, true);
        
        buffer = print_tags_array(buffer, "tagList", mympd_state->mpd_state->tag_types_mympd);
        buffer = sdscat(buffer, ",");
        buffer = print_tags_array(buffer, "tagListSearch", mympd_state->tag_types_search);
        buffer = sdscat(buffer, ",");
        buffer = print_tags_array(buffer, "tagListBrowse", mympd_state->tag_types_browse);
        buffer = sdscat(buffer, ",");
        buffer = print_tags_array(buffer, "tagListMpd", mympd_state->mpd_state->tag_types_mpd);
        buffer = sdscat(buffer, ",");
        buffer = print_tags_array(buffer, "smartplsGenerateTagList", mympd_state->smartpls_generate_tag_types);
        
        buffer = sdscat(buffer, ",\"triggerEvents\":{");
        buffer = print_trigger_list(buffer);
        buffer = sdscat(buffer, "}");

    } 
    else {
        buffer = tojson_bool(buffer, "mpdConnected", false, false);
    }
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

sds mympd_api_picture_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    sds pic_dirname = sdscatfmt(sdsempty(), "%s/pics", mympd_state->config->workdir);
    errno = 0;
    DIR *pic_dir = opendir(pic_dirname);
    if (pic_dir == NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true,
            "general", "error", "Can not open directory pics");
        MYMPD_LOG_ERROR("Can not open directory \"%s\"", pic_dirname);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(pic_dirname);
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
                strcasecmp(ext, ".tiff") == 0 || strcasecmp(ext, ".svg") == 0 ||
                strcasecmp(ext, ".bmp") == 0) 
            {
                if (returned_entities++) {
                    buffer = sdscat(buffer, ",");
                }
                buffer = sdscatjson(buffer, next_file->d_name, strlen(next_file->d_name));
            }
        }
    }
    closedir(pic_dir);
    sdsfree(pic_dirname);
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
    buffer = sdscat(buffer, default_navbar_icons);
    errno = 0;
    FILE *fp = fopen(file_name, OPEN_FLAGS_WRITE);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", file_name);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(file_name);
        return buffer;
    }
    int rc = fputs(buffer, fp);
    if (rc == EOF) {
        MYMPD_LOG_ERROR("Can not write to file \"%s\"", file_name);
    }
    fclose(fp);
    sdsfree(file_name);
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
        sdsfree(file_name);
        return buffer;
    }
    sdsfree(file_name);
    sdsgetfile(&buffer, fp, 2000);
    fclose(fp);
    
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
    for (size_t i = 0; i < tags.len; i++) {
        if (i > 0) {
            buffer = sdscat(buffer, ",");
        }
        const char *tagname = mpd_tag_name(tags.tags[i]);
        buffer = sdscatjson(buffer, tagname, strlen(tagname));
    }
    buffer = sdscat(buffer, "]");
    return buffer;
}
