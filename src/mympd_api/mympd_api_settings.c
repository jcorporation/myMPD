/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <libgen.h>
#include <dirent.h>
#include <stdbool.h>
#include <inttypes.h>

#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../../dist/src/rax/rax.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../log.h"
#include "../list.h"
#include "mympd_config_defs.h"
#include "../mympd_state.h"
#include "../state_files.h"
#include "../utility.h"
#include "../mpd_shared.h"
#include "../mpd_client/mpd_client_trigger.h"
#include "mympd_api_utility.h"
#include "mympd_api_timer.h"
#include "mympd_api_timer_handlers.h"
#include "mympd_api_settings.h"

//private definitions
static sds default_navbar_icons(struct t_config *config, sds buffer);
static sds read_navbar_icons(struct t_config *config);
static sds print_tags_array(sds buffer, const char *tagsname, struct t_tags tags);

//public functions
void mympd_api_settings_delete(struct t_config *config) {
    const char* state_files[]={"auto_play", "browsetaglist", "cols_browse_database",
        "cols_browse_filesystem", "cols_browse_playlists_detail", "cols_playback", "cols_queue_current", "cols_queue_last_played",
        "cols_search", "cols_queue_jukebox", "coverimage_names", "jukebox_mode", "jukebox_playlist", "jukebox_queue_length",
        "jukebox_unique_tag", "jukebox_last_played", "generate_pls_tags", "smartpls", "smartpls_sort", "smartpls_prefix", "smartpls_interval",
        "last_played_count", "locale", "searchtaglist", "taglist", "booklet_name", "advanced", 
        0};
    const char** ptr = state_files;
    while (*ptr != 0) {
        sds filename = sdscatfmt(sdsempty(), "%s/state/%s", config->workdir, *ptr);
        int rc = unlink(filename);
        if (rc != 0 && rc != ENOENT) {
            MYMPD_LOG_ERROR("Error removing file \"%s\": %s", filename, strerror(errno));
        }
        else if (rc != 0) {
            //ignore error
            MYMPD_LOG_DEBUG("Error removing file \"%s\": %s", filename, strerror(errno));
        }
        sdsfree(filename);
        ++ptr;
    }
}

bool mympd_api_connection_save(struct t_mympd_state *mympd_state, struct json_token *key, 
                               struct json_token *val, bool *mpd_host_changed)
{
    char *crap;
    sds settingname = sdsempty();
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    *mpd_host_changed = false;
    if (strncmp(key->ptr, "mpdPass", key->len) == 0) {
        if (strncmp(val->ptr, "dontsetpassword", val->len) != 0) {
            mympd_state->mpd_state->mpd_pass = sdsreplacelen(mympd_state->mpd_state->mpd_pass, settingvalue, sdslen(settingvalue));
            settingname = sdscat(settingname, "mpd_pass");
            *mpd_host_changed = true;
        }
        else {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return true;
        }
    }
    else if (strncmp(key->ptr, "mpdHost", key->len) == 0) {
        if (strncmp(val->ptr, mympd_state->mpd_state->mpd_host, val->len) != 0) {
            *mpd_host_changed = true;
            mympd_state->mpd_state->mpd_host = sdsreplacelen(mympd_state->mpd_state->mpd_host, settingvalue, sdslen(settingvalue));
        }
        settingname = sdscat(settingname, "mpd_host");
    }
    else if (strncmp(key->ptr, "mpdPort", key->len) == 0) {
        int mpd_port = strtoimax(settingvalue, &crap, 10);
        if (mympd_state->mpd_state->mpd_port != mpd_port) {
            *mpd_host_changed = true;
            mympd_state->mpd_state->mpd_port = mpd_port;
        }
        settingname = sdscat(settingname, "mpd_port");
    }
    else if (strncmp(key->ptr, "mpdStreamPort", key->len) == 0) {
        mympd_state->mpd_stream_port = strtoimax(settingvalue, &crap, 10);
        settingname = sdscat(settingname, "mpd_port");
    }
    else if (strncmp(key->ptr, "musicDirectory", key->len) == 0) {
        mympd_state->music_directory = sdsreplacelen(mympd_state->music_directory, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "music_directory");
        strip_slash(mympd_state->music_directory);
    }
    else if (strncmp(key->ptr, "playlistDirectory", key->len) == 0) {
        mympd_state->playlist_directory = sdsreplacelen(mympd_state->playlist_directory, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "playlist_directory");
        strip_slash(mympd_state->playlist_directory);
    }
    else if (strncmp(key->ptr, "mpdBinarylimit", key->len) == 0) {
        mympd_state->mpd_state->mpd_binarylimit = strtoimax(settingvalue, &crap, 10);
        settingname = sdscat(settingname, "mpd_binarylimit");
    }
    else {
        MYMPD_LOG_WARN("Unknown setting %s: %s", settingname, settingvalue);
        sdsfree(settingname);
        sdsfree(settingvalue);
        return true;
    }

    bool rc = state_file_write(mympd_state->config, "state", settingname, settingvalue);
    sdsfree(settingname);
    sdsfree(settingvalue);
    return rc;
}

bool mympd_api_cols_save(struct t_mympd_state *mympd_state, const char *table,
                         const char *cols)
{
    sds tablename = sdsempty();
    if (strcmp(table, "colsQueueCurrent") == 0) {
        mympd_state->cols_queue_current = sdsreplace(mympd_state->cols_queue_current, cols);
        tablename = sdsreplace(tablename, "cols_queue_current");
    }
    else if (strcmp(table, "colsQueueLastPlayed") == 0) {
        mympd_state->cols_queue_last_played = sdsreplace(mympd_state->cols_queue_last_played, cols);
        tablename = sdsreplace(tablename, "cols_queue_last_played");
    }
    else if (strcmp(table, "colsSearch") == 0) {
        mympd_state->cols_search = sdsreplace(mympd_state->cols_search, cols);
        tablename = sdsreplace(tablename, "cols_search");
    }
    else if (strcmp(table, "colsBrowseDatabaseDetail") == 0) {
        mympd_state->cols_browse_database = sdsreplace(mympd_state->cols_browse_database, cols);
        tablename = sdsreplace(tablename, "cols_browse_database");
    }
    else if (strcmp(table, "colsBrowsePlaylistsDetail") == 0) {
        mympd_state->cols_browse_playlists_detail = sdsreplace(mympd_state->cols_browse_playlists_detail, cols);
        tablename = sdsreplace(tablename, "cols_browse_playlists_detail");
    }
    else if (strcmp(table, "colsBrowseFilesystem") == 0) {
        mympd_state->cols_browse_filesystem = sdsreplace(mympd_state->cols_browse_filesystem, cols);
        tablename = sdsreplace(tablename, "cols_browse_filesystem");
    }
    else if (strcmp(table, "colsPlayback") == 0) {
        mympd_state->cols_playback = sdsreplace(mympd_state->cols_playback, cols);
        tablename = sdsreplace(tablename, "cols_playback");
    }
    else if (strcmp(table, "colsQueueJukebox") == 0) {
        mympd_state->cols_queue_jukebox = sdsreplace(mympd_state->cols_queue_jukebox, cols);
        tablename = sdsreplace(tablename, "cols_queue_jukebox");
    }
    else {
        sdsfree(tablename);
        return false;
    }
    
    if (!state_file_write(mympd_state->config, "state", tablename, cols)) {
        sdsfree(tablename);
        return false;
    }
    sdsfree(tablename);
    return true;
}

bool mympd_api_settings_set(struct t_mympd_state *mympd_state, struct json_token *key, struct json_token *val) {
    sds settingname = sdsempty();
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    char *crap;
    bool rc = false;

    MYMPD_LOG_DEBUG("Parse setting %.*s: %.*s", key->len, key->ptr, val->len, val->ptr);
    if (strncmp(key->ptr, "coverimageNames", key->len) == 0) {
        if (validate_string(settingvalue) && sdslen(settingvalue) > 0) {
            mympd_state->coverimage_names = sdsreplacelen(mympd_state->coverimage_names, settingvalue, sdslen(settingvalue));
            settingname = sdscat(settingname, "coverimage_names");
        }
        else {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
    }
    else if (strncmp(key->ptr, "bookletName", key->len) == 0) {
        mympd_state->booklet_name = sdsreplacelen(mympd_state->booklet_name, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "booklet_name");
    }
    else if (strncmp(key->ptr, "lastPlayedCount", key->len) == 0) {
        int last_played_count = strtoimax(settingvalue, &crap, 10);
        if (last_played_count <= 0) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->last_played_count = last_played_count;
        settingname = sdscat(settingname, "last_played_count");
    }
    else if (strncmp(key->ptr, "volumeMin", key->len) == 0) {
        int volume_min = strtoimax(settingvalue, &crap, 10);
        if (volume_min < 0 || volume_min > 100) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->volume_min = volume_min;
        settingname = sdscat(settingname, "volume_min");
    }
    else if (strncmp(key->ptr, "volumeMax", key->len) == 0) {
        int volume_max = strtoimax(settingvalue, &crap, 10);
        if (volume_max < 0 || volume_max > 100) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->volume_max = volume_max;
        settingname = sdscat(settingname, "volume_max");
    }
    else if (strncmp(key->ptr, "volumeStep", key->len) == 0) {
        int volume_step = strtoimax(settingvalue, &crap, 10);
        if (volume_step < 0 || volume_step > 100) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->volume_step = volume_step;
        settingname = sdscat(settingname, "volume_step");
    }
    else if (strncmp(key->ptr, "taglist", key->len) == 0) {
        mympd_state->mpd_state->taglist = sdsreplacelen(mympd_state->mpd_state->taglist, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "taglist");
    }
    else if (strncmp(key->ptr, "searchtaglist", key->len) == 0) {
        mympd_state->searchtaglist = sdsreplacelen(mympd_state->searchtaglist, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "searchtaglist");
    }
    else if (strncmp(key->ptr, "browsetaglist", key->len) == 0) {
        mympd_state->browsetaglist = sdsreplacelen(mympd_state->browsetaglist, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "browsetaglist");
    }
    else if (strncmp(key->ptr, "smartpls", key->len) == 0) {
        mympd_state->smartpls = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(settingname, "smartpls");
    }
    else if (strncmp(key->ptr, "smartplsSort", key->len) == 0) {
        mympd_state->smartpls_sort = sdsreplacelen(mympd_state->smartpls_sort, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "smartpls_sort");
    }
    else if (strncmp(key->ptr, "smartplsPrefix", key->len) == 0) {
        mympd_state->smartpls_prefix = sdsreplacelen(mympd_state->smartpls_prefix, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "smartpls_prefix");
    }
    else if (strncmp(key->ptr, "smartplsInterval", key->len) == 0) {
        time_t interval = strtoumax(settingvalue, &crap, 10);
        if (interval != mympd_state->smartpls_interval) {
            mympd_state->smartpls_interval = interval;
            replace_timer(&mympd_state->timer_list, interval, interval, timer_handler_smartpls_update, 2, NULL, NULL);
        }
        settingname = sdscat(settingname, "smartpls_interval");
    }
    else if (strncmp(key->ptr, "generatePlsTags", key->len) == 0) {
        mympd_state->generate_pls_tags = sdsreplacelen(mympd_state->generate_pls_tags, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "generate_pls_tags");
    }
    else if (strncmp(key->ptr, "advanced", key->len) == 0) {
        mympd_state->advanced = sdsreplacelen(mympd_state->advanced, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "advanced");
    }
    else {
        MYMPD_LOG_WARN("Unknown setting \"%s\": \"%s\"", settingname, settingvalue);
        sdsfree(settingname);
        sdsfree(settingvalue);
        return true;
    }
    rc = state_file_write(mympd_state->config, "state", settingname, settingvalue);
    sdsfree(settingname);
    sdsfree(settingvalue);
    return rc;
}

bool mpdclient_api_options_set(struct t_mympd_state *mympd_state, struct json_token *key, struct json_token *val,
                            bool *jukebox_changed, bool *check_mpd_error)
{
    sds settingname = sdsempty();
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    *check_mpd_error = false;
    *jukebox_changed = false;
    char *crap;
    bool rc = false;

    MYMPD_LOG_DEBUG("Parse setting %.*s: %.*s", key->len, key->ptr, val->len, val->ptr);
    if (strncmp(key->ptr, "autoPlay", key->len) == 0) {
        mympd_state->auto_play = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(settingname, "auto_play");
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
        settingname = sdscat(settingname, "jukebox_mode");
    }
    else if (strncmp(key->ptr, "jukeboxPlaylist", key->len) == 0) {
        if (strcmp(mympd_state->jukebox_playlist, settingvalue) != 0) {
            mympd_state->jukebox_playlist = sdsreplacelen(mympd_state->jukebox_playlist, settingvalue, sdslen(settingvalue));
            *jukebox_changed = true;
        }
        settingname = sdscat(settingname, "jukebox_playlist");
    }
    else if (strncmp(key->ptr, "jukeboxQueueLength", key->len) == 0) {
        int jukebox_queue_length = strtoimax(settingvalue, &crap, 10);
        if (jukebox_queue_length <= 0 || jukebox_queue_length > 999) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->jukebox_queue_length = jukebox_queue_length;
        settingname = sdscat(settingname, "jukebox_queue_length");
    }
    else if (strncmp(key->ptr, "jukeboxUniqueTag", key->len) == 0) {
        settingname = sdscat(settingname, "jukebox_unique_tag");
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
        int jukebox_last_played = strtoimax(settingvalue, &crap, 10);
        if (jukebox_last_played != mympd_state->jukebox_last_played) {
            mympd_state->jukebox_last_played = jukebox_last_played;
            *jukebox_changed = true;
        }
        settingname = sdscat(settingname, "jukebox_last_played");
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
    }
    else {
        MYMPD_LOG_WARN("Unknown setting \"%s\": \"%s\"", settingname, settingvalue);
        sdsfree(settingname);
        sdsfree(settingvalue);
        return false;
    }
    if (*check_mpd_error == false) {
        rc = state_file_write(mympd_state->config, "state", settingname, settingvalue);
    }
    sdsfree(settingname);
    sdsfree(settingvalue);
    return rc;
}

void mympd_api_settings_reset(struct t_mympd_state *mympd_state) {
    mympd_api_settings_delete(mympd_state->config);
    free_mympd_state_sds(mympd_state);
    default_mympd_state(mympd_state);
}

void mympd_api_read_statefiles(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_NOTICE("Reading states");
    mympd_state->mpd_state->mpd_host = state_file_rw_string_sds(mympd_state->config, "state", "mpd_host", mympd_state->mpd_state->mpd_host, false);
    mympd_state->mpd_state->mpd_port = state_file_rw_int(mympd_state->config, "state", "mpd_port", mympd_state->mpd_state->mpd_port, false);
    mympd_state->mpd_state->mpd_pass = state_file_rw_string_sds(mympd_state->config, "state", "mpd_pass", mympd_state->mpd_state->mpd_pass, false);
    mympd_state->mpd_state->mpd_binarylimit = state_file_rw_int(mympd_state->config, "state", "mpd_binarylimit", mympd_state->mpd_state->mpd_binarylimit, false);
    mympd_state->mpd_state->taglist = state_file_rw_string_sds(mympd_state->config, "state", "taglist", mympd_state->mpd_state->taglist, false);

    mympd_state->searchtaglist = state_file_rw_string_sds(mympd_state->config, "state", "searchtaglist", mympd_state->searchtaglist, false);
    mympd_state->browsetaglist = state_file_rw_string_sds(mympd_state->config, "state", "browsetaglist", mympd_state->browsetaglist, false);
    mympd_state->smartpls = state_file_rw_bool(mympd_state->config, "state", "smartpls", mympd_state->smartpls, false);
    mympd_state->smartpls_sort = state_file_rw_string_sds(mympd_state->config, "state", "smartpls_sort", mympd_state->smartpls_sort, false);
    mympd_state->smartpls_prefix = state_file_rw_string_sds(mympd_state->config, "state", "smartpls_prefix", mympd_state->smartpls_prefix, false);
    mympd_state->smartpls_interval = state_file_rw_int(mympd_state->config, "state", "smartpls_interval", mympd_state->smartpls_interval, false);
    mympd_state->generate_pls_tags = state_file_rw_string_sds(mympd_state->config, "state", "generate_pls_tags", mympd_state->generate_pls_tags, false);
    mympd_state->last_played_count = state_file_rw_int(mympd_state->config, "state", "last_played_count", (int)mympd_state->last_played_count, false);
    mympd_state->auto_play = state_file_rw_bool(mympd_state->config, "state", "auto_play", mympd_state->auto_play, false);
    mympd_state->jukebox_mode = state_file_rw_int(mympd_state->config, "state", "jukebox_mode", mympd_state->jukebox_mode, false);
    mympd_state->jukebox_playlist = state_file_rw_string_sds(mympd_state->config, "state", "jukebox_playlist", mympd_state->jukebox_playlist, false);
    mympd_state->jukebox_queue_length = state_file_rw_int(mympd_state->config, "state", "jukebox_queue_length",(int)mympd_state->jukebox_queue_length, false);
    mympd_state->jukebox_last_played = state_file_rw_int(mympd_state->config, "state", "jukebox_last_played", mympd_state->jukebox_last_played, false);
    mympd_state->jukebox_unique_tag.tags[0] = state_file_rw_int(mympd_state->config, "state", "jukebox_unique_tag", mympd_state->jukebox_unique_tag.tags[0], false);
    mympd_state->cols_queue_current = state_file_rw_string_sds(mympd_state->config, "state", "cols_queue_current", mympd_state->cols_queue_current, false);
    mympd_state->cols_search = state_file_rw_string_sds(mympd_state->config, "state", "cols_search", mympd_state->cols_search, false);
    mympd_state->cols_browse_database = state_file_rw_string_sds(mympd_state->config, "state", "cols_browse_database", mympd_state->cols_browse_database, false);
    mympd_state->cols_browse_playlists_detail = state_file_rw_string_sds(mympd_state->config, "state", "cols_browse_playlists_detail", mympd_state->cols_browse_playlists_detail, false);
    mympd_state->cols_browse_filesystem = state_file_rw_string_sds(mympd_state->config, "state", "cols_browse_filesystem", mympd_state->cols_browse_filesystem, false);
    mympd_state->cols_playback = state_file_rw_string_sds(mympd_state->config, "state", "cols_playback", mympd_state->cols_playback, false);
    mympd_state->cols_queue_last_played = state_file_rw_string_sds(mympd_state->config, "state", "cols_queue_last_played", mympd_state->cols_queue_last_played, false);
    mympd_state->cols_queue_jukebox = state_file_rw_string_sds(mympd_state->config, "state", "cols_queue_jukebox", mympd_state->cols_queue_jukebox, false);
    mympd_state->coverimage_names = state_file_rw_string_sds(mympd_state->config, "state", "coverimage_names", mympd_state->coverimage_names, false);
    mympd_state->music_directory = state_file_rw_string_sds(mympd_state->config, "state", "music_directory", mympd_state->music_directory, false);
    mympd_state->playlist_directory = state_file_rw_string_sds(mympd_state->config, "state", "playlist_directory", mympd_state->playlist_directory, false);
    mympd_state->booklet_name = state_file_rw_string_sds(mympd_state->config, "state", "booklet_name", mympd_state->booklet_name, false);
    mympd_state->volume_min = state_file_rw_int(mympd_state->config, "state", "volume_min", (int)mympd_state->volume_min, false);
    mympd_state->volume_max = state_file_rw_int(mympd_state->config, "state", "volume_max", (int)mympd_state->volume_max, false);
    mympd_state->volume_step = state_file_rw_int(mympd_state->config, "state", "volume_step", (int)mympd_state->volume_step, false);
    mympd_state->advanced = state_file_rw_string_sds(mympd_state->config, "state", "advanced", mympd_state->advanced, false);
    mympd_state->mpd_stream_port = state_file_rw_int(mympd_state->config, "state", "mpd_stream_port", mympd_state->mpd_stream_port, false);
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
    buffer = tojson_long(buffer, "mpdBinarylimit", mympd_state->mpd_state->mpd_binarylimit, true);
#ifdef ENABLE_SSL
    buffer = tojson_bool(buffer, "featCacert", (mympd_state->config->custom_cert == false && mympd_state->config->ssl == true ? true : false), true);
#else
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
    buffer = sdscatfmt(buffer, "\"colsQueueCurrent\":%s,", mympd_state->cols_queue_current);
    buffer = sdscatfmt(buffer, "\"colsSearch\":%s,", mympd_state->cols_search);
    buffer = sdscatfmt(buffer, "\"colsBrowseDatabaseDetail\":%s,", mympd_state->cols_browse_database);
    buffer = sdscatfmt(buffer, "\"colsBrowsePlaylistsDetail\":%s,", mympd_state->cols_browse_playlists_detail);
    buffer = sdscatfmt(buffer, "\"colsBrowseFilesystem\":%s,", mympd_state->cols_browse_filesystem);
    buffer = sdscatfmt(buffer, "\"colsPlayback\":%s,", mympd_state->cols_playback);
    buffer = sdscatfmt(buffer, "\"colsQueueLastPlayed\":%s,", mympd_state->cols_queue_last_played);
    buffer = sdscatfmt(buffer, "\"colsQueueJukebox\":%s,", mympd_state->cols_queue_jukebox);
    buffer = sdscatfmt(buffer, "\"navbarIcons\":%s,", mympd_state->navbar_icons);
    buffer = sdscatfmt(buffer, "\"advanced\":%s,", mympd_state->advanced);
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
        mpd_status_free(status);

        buffer = print_tags_array(buffer, "tags", mympd_state->mpd_state->mympd_tag_types);
        buffer = sdscat(buffer, ",");
        buffer = print_tags_array(buffer, "searchtags", mympd_state->search_tag_types);
        buffer = sdscat(buffer, ",");
        buffer = print_tags_array(buffer, "browsetags", mympd_state->browse_tag_types);
        buffer = sdscat(buffer, ",");
        buffer = print_tags_array(buffer, "allmpdtags", mympd_state->mpd_state->mpd_tag_types);
        buffer = sdscat(buffer, ",");
        buffer = print_tags_array(buffer, "generatePlsTags", mympd_state->generate_pls_tag_types);
        
        buffer = sdscat(buffer, ",\"triggers\":{");
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
    DIR *pic_dir = opendir(pic_dirname);
    if (pic_dir == NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true,
            "general", "error", "Can not open directory pics");
        MYMPD_LOG_ERROR("Can not open directory \"%s\": %s", pic_dirname, strerror(errno));
        sdsfree(pic_dirname);
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int returned_entities = 0;
    struct dirent *next_file;
    while ((next_file = readdir(pic_dir)) != NULL ) {
        if (next_file->d_type == DT_REG) {
            if (returned_entities++) {
                buffer = sdscat(buffer, ",");
            }
            buffer = sdscatjson(buffer, next_file->d_name, strlen(next_file->d_name));
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
static sds default_navbar_icons(struct t_config *config, sds buffer) {
    MYMPD_LOG_NOTICE("Writing default navbar_icons");
    sds file_name = sdscatfmt(sdsempty(), "%s/state/navbar_icons", config->workdir);
    sdsclear(buffer);
    buffer = sdscat(buffer, NAVBAR_ICONS);
    FILE *fp = fopen(file_name, "w");
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write: %s", file_name, strerror(errno));
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
    FILE *fp = fopen(file_name, "r");
    if (fp == NULL) {
        if (errno != ENOENT) {
            MYMPD_LOG_ERROR("Can not open file \"%s\": %s", file_name, strerror(errno));
        }
        buffer = default_navbar_icons(config, buffer);
        sdsfree(file_name);
        return buffer;
    }
    sdsfree(file_name);
    char *line = NULL;
    char *crap = NULL;
    size_t n = 0;
    while (getline(&line, &n, fp) > 0) {
        strtok_r(line, "\n", &crap);
        buffer = sdscat(buffer, line);
    }
    FREE_PTR(line);
    fclose(fp);
    if (sdslen(buffer) == 0) {
        buffer = default_navbar_icons(config, buffer);
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
