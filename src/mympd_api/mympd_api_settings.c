/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_settings.h"

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
const char *default_navbar_icons = "[{\"ligature\":\"home\",\"title\":\"Home\",\"options\":[\"Home\"],\"badge\":\"\"},"\
    "{\"ligature\":\"equalizer\",\"title\":\"Playback\",\"options\":[\"Playback\"],\"badge\":\"\"},"\
    "{\"ligature\":\"queue_music\",\"title\":\"Queue\",\"options\":[\"Queue\"],\"badge\":\"<span id=\\\"badgeQueueItems\\\" class=\\\"badge badge-secondary\\\"></span>\"},"\
    "{\"ligature\":\"library_music\",\"title\":\"Browse\",\"options\":[\"Browse\"],\"badge\":\"\"},"\
    "{\"ligature\":\"search\",\"title\":\"Search\",\"options\":[\"Search\"],\"badge\":\"\"}]";

//public functions
bool mympd_api_connection_save(struct t_mympd_state *mympd_state, struct json_token *key, 
                               struct json_token *val, bool *mpd_host_changed)
{
    //TODO: validate values and write validated values only
    char *crap;
    sds settingname = camel_to_snake(key->ptr, key->len);
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    if (strncmp(key->ptr, "mpdPass", key->len) == 0) {
        if (strcmp(settingvalue, "dontsetpassword") != 0) {
            mympd_state->mpd_state->mpd_pass = sdsreplacelen(mympd_state->mpd_state->mpd_pass, settingvalue, sdslen(settingvalue));
            *mpd_host_changed = true;
        }
        else {
            //keep old password
            sdsfree(settingname);
            sdsfree(settingvalue);
            return true;
        }
    }
    else if (strncmp(key->ptr, "mpdHost", key->len) == 0) {
        if (strcmp(settingvalue, mympd_state->mpd_state->mpd_host) != 0) {
            *mpd_host_changed = true;
            mympd_state->mpd_state->mpd_host = sdsreplacelen(mympd_state->mpd_state->mpd_host, settingvalue, sdslen(settingvalue));
        }
    }
    else if (strncmp(key->ptr, "mpdPort", key->len) == 0) {
        int mpd_port = (int)strtoimax(settingvalue, &crap, 10);
        if (mpd_port < 1024 || mpd_port > 65535) {
            MYMPD_LOG_ERROR("Invalid value for mpdPort: %s", settingvalue);
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        if (mympd_state->mpd_state->mpd_port != mpd_port) {
            *mpd_host_changed = true;
            mympd_state->mpd_state->mpd_port = mpd_port;
        }
    }
    else if (strncmp(key->ptr, "mpdStreamPort", key->len) == 0) {
        int mpd_stream_port = (int)strtoimax(settingvalue, &crap, 10);
        if (mpd_stream_port < 1024 || mpd_stream_port > 65535) {
            MYMPD_LOG_ERROR("Invalid value for mpdStreamPort: %s", settingvalue);
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->mpd_stream_port = mpd_stream_port;
    }
    else if (strncmp(key->ptr, "musicDirectory", key->len) == 0) {
        mympd_state->music_directory = sdsreplacelen(mympd_state->music_directory, settingvalue, sdslen(settingvalue));
        strip_slash(mympd_state->music_directory);
    }
    else if (strncmp(key->ptr, "playlistDirectory", key->len) == 0) {
        mympd_state->playlist_directory = sdsreplacelen(mympd_state->playlist_directory, settingvalue, sdslen(settingvalue));
        strip_slash(mympd_state->playlist_directory);
    }
    else if (strncmp(key->ptr, "mpdBinarylimit", key->len) == 0) {
        unsigned binarylimit = strtoumax(settingvalue, &crap, 10);
        if (binarylimit < 4096 || binarylimit > 262144) {
            MYMPD_LOG_ERROR("Invalid value for binarylimit: %s", settingvalue);
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        if (binarylimit != mympd_state->mpd_state->mpd_binarylimit) {
            mympd_state->mpd_state->mpd_binarylimit = binarylimit;
            if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                mpd_client_set_binarylimit(mympd_state);
            }
        }
    }
    else if (strncmp(key->ptr, "mpdTimeout", key->len) == 0) {
        int mpd_timeout = (int)strtoimax(settingvalue, &crap, 10);
        if (mpd_timeout < 1000 || mpd_timeout > 1000000) {
            MYMPD_LOG_ERROR("Invalid value for mpdTimeoutt: %s", settingvalue);
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        if (mpd_timeout != mympd_state->mpd_state->mpd_timeout) {
            mympd_state->mpd_state->mpd_timeout = mpd_timeout;
            if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                mpd_connection_set_timeout(mympd_state->mpd_state->conn, mympd_state->mpd_state->mpd_timeout);
            }
        }
    }
    else if (strncmp(key->ptr, "mpdKeepalive", key->len) == 0) {
        bool keepalive = val->type == JSON_TYPE_TRUE ? true : false;
        if (keepalive != mympd_state->mpd_state->mpd_keepalive) {
            mympd_state->mpd_state->mpd_keepalive = keepalive;
            if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
                mpd_connection_set_keepalive(mympd_state->mpd_state->conn, mympd_state->mpd_state->mpd_keepalive);
            }
        }
    }
    else {
        MYMPD_LOG_WARN("Unknown setting %s: %s", settingname, settingvalue);
        sdsfree(settingname);
        sdsfree(settingvalue);
        return true;
    }

    bool rc = state_file_write(mympd_state->config->workdir, "state", settingname, settingvalue);
    sdsfree(settingname);
    sdsfree(settingvalue);
    return rc;
}

bool mympd_api_cols_save(struct t_mympd_state *mympd_state, const char *table,
                         const char *cols)
{
    sds tablename = camel_to_snake(table, strlen(table));
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
        sdsfree(tablename);
        return false;
    }
    
    if (!state_file_write(mympd_state->config->workdir, "state", tablename, cols)) {
        sdsfree(tablename);
        return false;
    }
    sdsfree(tablename);
    return true;
}

bool mympd_api_settings_set(struct t_mympd_state *mympd_state, struct json_token *key, struct json_token *val) {
    sds settingname = camel_to_snake(key->ptr, key->len);
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    char *crap;
    bool rc = false;

    MYMPD_LOG_DEBUG("Parse setting %.*s: %.*s", key->len, key->ptr, val->len, val->ptr);
    if (strncmp(key->ptr, "coverimageNames", key->len) == 0) {
        if (validate_string(settingvalue) && sdslen(settingvalue) > 0) {
            mympd_state->coverimage_names = sdsreplacelen(mympd_state->coverimage_names, settingvalue, sdslen(settingvalue));
        }
        else {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
    }
    else if (strncmp(key->ptr, "bookletName", key->len) == 0) {
        mympd_state->booklet_name = sdsreplacelen(mympd_state->booklet_name, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "lastPlayedCount", key->len) == 0) {
        int last_played_count = (int)strtoimax(settingvalue, &crap, 10);
        if (last_played_count <= 0) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->last_played_count = last_played_count;
    }
    else if (strncmp(key->ptr, "volumeMin", key->len) == 0) {
        int volume_min = (int)strtoimax(settingvalue, &crap, 10);
        if (volume_min < 0 || volume_min > 100) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->volume_min = volume_min;
    }
    else if (strncmp(key->ptr, "volumeMax", key->len) == 0) {
        int volume_max = (int)strtoimax(settingvalue, &crap, 10);
        if (volume_max < 0 || volume_max > 100) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->volume_max = volume_max;
    }
    else if (strncmp(key->ptr, "volumeStep", key->len) == 0) {
        int volume_step = (int)strtoimax(settingvalue, &crap, 10);
        if (volume_step < 0 || volume_step > 100) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->volume_step = volume_step;
    }
    else if (strncmp(key->ptr, "tagList", key->len) == 0) {
        mympd_state->mpd_state->tag_list = sdsreplacelen(mympd_state->mpd_state->tag_list, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "tagListSearch", key->len) == 0) {
        mympd_state->tag_list_search = sdsreplacelen(mympd_state->tag_list_search, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "tagListBrowse", key->len) == 0) {
        mympd_state->tag_list_browse = sdsreplacelen(mympd_state->tag_list_browse, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "smartpls", key->len) == 0) {
        mympd_state->smartpls = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "smartplsSort", key->len) == 0) {
        mympd_state->smartpls_sort = sdsreplacelen(mympd_state->smartpls_sort, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "smartplsPrefix", key->len) == 0) {
        mympd_state->smartpls_prefix = sdsreplacelen(mympd_state->smartpls_prefix, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "smartplsInterval", key->len) == 0) {
        time_t interval = strtoimax(settingvalue, &crap, 10);
        if (interval != mympd_state->smartpls_interval) {
            mympd_state->smartpls_interval = interval;
            replace_timer(&mympd_state->timer_list, interval, (int)interval, timer_handler_smartpls_update, 2, NULL, NULL);
        }
    }
    else if (strncmp(key->ptr, "smartplsGenerateTagList", key->len) == 0) {
        mympd_state->smartpls_generate_tag_list = sdsreplacelen(mympd_state->smartpls_generate_tag_list, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "webuiSettings", key->len) == 0) {
        mympd_state->webui_settings = sdsreplacelen(mympd_state->webui_settings, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "lyricsUsltExt", key->len) == 0) {
        mympd_state->lyrics_uslt_ext = sdsreplacelen(mympd_state->lyrics_uslt_ext, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "lyricsSyltExt", key->len) == 0) {
        mympd_state->lyrics_sylt_ext = sdsreplacelen(mympd_state->lyrics_sylt_ext, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "lyricsVorbisUslt", key->len) == 0) {
        mympd_state->lyrics_vorbis_uslt = sdsreplacelen(mympd_state->lyrics_vorbis_uslt, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "lyricsVorbisSylt", key->len) == 0) {
        mympd_state->lyrics_vorbis_sylt = sdsreplacelen(mympd_state->lyrics_vorbis_sylt, settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "covercacheKeepDays", key->len) == 0) {
        mympd_state->covercache_keep_days = (int)strtoimax(settingvalue, &crap, 10);
    }
    else {
        MYMPD_LOG_WARN("Unknown setting \"%s\": \"%s\"", settingname, settingvalue);
        sdsfree(settingname);
        sdsfree(settingvalue);
        return true;
    }
    rc = state_file_write(mympd_state->config->workdir, "state", settingname, settingvalue);
    sdsfree(settingname);
    sdsfree(settingvalue);
    return rc;
}

bool mpdclient_api_options_set(struct t_mympd_state *mympd_state, struct json_token *key, struct json_token *val,
                            bool *jukebox_changed, bool *check_mpd_error)
{
    sds settingname = camel_to_snake(key->ptr, key->len);
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    char *crap;
    bool rc = false;
    bool write_state_file = true;

    MYMPD_LOG_DEBUG("Parse setting %.*s: %.*s", key->len, key->ptr, val->len, val->ptr);
    if (strncmp(key->ptr, "autoPlay", key->len) == 0) {
        mympd_state->auto_play = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "jukeboxMode", key->len) == 0) {
        unsigned jukebox_mode = strtoumax(settingvalue, &crap, 10);
        if (jukebox_mode > 2) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        if (mympd_state->jukebox_mode != jukebox_mode) {
            mympd_state->jukebox_mode = jukebox_mode;
            *jukebox_changed = true;
        }
    }
    else if (strncmp(key->ptr, "jukeboxPlaylist", key->len) == 0) {
        if (strcmp(mympd_state->jukebox_playlist, settingvalue) != 0) {
            mympd_state->jukebox_playlist = sdsreplacelen(mympd_state->jukebox_playlist, settingvalue, sdslen(settingvalue));
            *jukebox_changed = true;
        }
    }
    else if (strncmp(key->ptr, "jukeboxQueueLength", key->len) == 0) {
        int jukebox_queue_length = (int)strtoimax(settingvalue, &crap, 10);
        if (jukebox_queue_length <= 0 || jukebox_queue_length > 999) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->jukebox_queue_length = jukebox_queue_length;
    }
    else if (strncmp(key->ptr, "jukeboxUniqueTag", key->len) == 0) {
        enum mpd_tag_type unique_tag = mpd_tag_name_parse(settingvalue);
        if (unique_tag == MPD_TAG_UNKNOWN) {
            unique_tag = MPD_TAG_TITLE;
            sdsclear(settingvalue);
            settingvalue = sdscatprintf(settingvalue, "%u", unique_tag);
        }
        if (mympd_state->jukebox_unique_tag.tags[0] != unique_tag) {
            mympd_state->jukebox_unique_tag.tags[0] = unique_tag;
            *jukebox_changed = true;
        }
    }
    else if (strncmp(key->ptr, "jukeboxLastPlayed", key->len) == 0) {
        int jukebox_last_played = (int)strtoimax(settingvalue, &crap, 10);
        if (jukebox_last_played != mympd_state->jukebox_last_played) {
            mympd_state->jukebox_last_played = jukebox_last_played;
            *jukebox_changed = true;
        }
    }
    else if (mympd_state->mpd_state->conn_state == MPD_CONNECTED) {
        if (strncmp(key->ptr, "random", key->len) == 0) {
            unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
            rc = mpd_run_random(mympd_state->mpd_state->conn, uint_buf);
        }
        else if (strncmp(key->ptr, "repeat", key->len) == 0) {
            unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
            rc = mpd_run_repeat(mympd_state->mpd_state->conn, uint_buf);
        }
        else if (strncmp(key->ptr, "consume", key->len) == 0) {
            unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
            rc = mpd_run_consume(mympd_state->mpd_state->conn, uint_buf);
        }
        else if (strncmp(key->ptr, "single", key->len) == 0) {
            unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
            if (mympd_state->mpd_state->feat_single_oneshot == true) {
                enum mpd_single_state state;
                if (uint_buf == 0) { state = MPD_SINGLE_OFF; }
                else if (uint_buf == 1) { state = MPD_SINGLE_ON; }
                else if (uint_buf == 2) { state = MPD_SINGLE_ONESHOT; }
                else { state = MPD_SINGLE_UNKNOWN; }
                rc = mpd_run_single_state(mympd_state->mpd_state->conn, state);
            }
            else {
                rc = mpd_run_single(mympd_state->mpd_state->conn, uint_buf);
            }
        }
        else if (strncmp(key->ptr, "crossfade", key->len) == 0) {
            unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
            rc = mpd_run_crossfade(mympd_state->mpd_state->conn, uint_buf);
        }
        else if (strncmp(key->ptr, "replaygain", key->len) == 0) {
            enum mpd_replay_gain_mode mode = mpd_parse_replay_gain_name(settingvalue);
            if (mode == MPD_REPLAY_UNKNOWN) {
                MYMPD_LOG_ERROR("Unknown replay gain mode: %s", settingvalue);
            }
            else {
                rc = mpd_run_replay_gain_mode(mympd_state->mpd_state->conn, mode);
            }
        }
        *check_mpd_error = true;
        write_state_file = false;
    }
    else {
        MYMPD_LOG_WARN("Unknown setting \"%s\": \"%s\"", settingname, settingvalue);
        sdsfree(settingname);
        sdsfree(settingvalue);
        return false;
    }
    if (write_state_file == true) {
        rc = state_file_write(mympd_state->config->workdir, "state", settingname, settingvalue);
    }
    sdsfree(settingname);
    sdsfree(settingvalue);
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
