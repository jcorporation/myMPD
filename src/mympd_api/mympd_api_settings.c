/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

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

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../dist/src/frozen/frozen.h"
#include "../utility.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "mympd_api_utility.h"
#include "mympd_api_settings.h"

void mympd_api_settings_delete(t_config *config) {
    if (config->readonly == true) {
        return;
    }
    const char* state_files[]={"auto_play", "bg_color", "bg_cover", "bg_css_filter", "browsetaglist", "cols_browse_database",
        "cols_browse_filesystem", "cols_browse_playlists_detail", "cols_playback", "cols_queue_current", "cols_queue_last_played",
        "cols_search", "coverimage", "coverimage_name", "coverimage_size", "jukebox_mode", "jukebox_playlist", "jukebox_queue_length",
        "last_played", "last_played_count", "locale", "localplayer", "localplayer_autoplay", "love", "love_channel", "love_message",
        "max_elements_per_page",  "mpd_host", "mpd_pass", "mpd_port", "notification_page", "notification_web", "searchtaglist",
        "smartpls", "stickers", "stream_port", "stream_url", "taglist", "music_directory", "bookmarks", "covergrid_size", 
        "theme", 0};
    const char** ptr = state_files;
    while (*ptr != 0) {
        sds filename = sdscatfmt(sdsempty(), "%s/state/%s", config->varlibdir, *ptr);
        unlink(filename);
        sdsfree(filename);
        ++ptr;
    }
}

bool mympd_api_connection_save(t_config *config, t_mympd_state *mympd_state, struct json_token *key, struct json_token *val) {
    char *crap;
    sds settingname = sdsempty();
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    if (strncmp(key->ptr, "mpdPass", key->len) == 0) {
        if (strncmp(val->ptr, "dontsetpassword", val->len) != 0) {
            mympd_state->mpd_pass = sdsreplacelen(mympd_state->mpd_pass, settingvalue, sdslen(settingvalue));
            settingname = sdscat(settingname, "mpd_pass");
        }
        else {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return true;
        }
    }
    else if (strncmp(key->ptr, "mpdHost", key->len) == 0) {
        mympd_state->mpd_host = sdsreplacelen(mympd_state->mpd_host, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "mpd_host");
    }
    else if (strncmp(key->ptr, "mpdPort", key->len) == 0) {
        mympd_state->mpd_port = strtoimax(settingvalue, &crap, 10);
        settingname = sdscat(settingname, "mpd_port");
    }
    else if (strncmp(key->ptr, "musicDirectory", key->len) == 0) {
        mympd_state->music_directory = sdsreplacelen(mympd_state->music_directory, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "music_directory");
    }
    else {
        sdsfree(settingname);
        sdsfree(settingvalue);
        return true;
    }

    bool rc = state_file_write(config, settingname, settingvalue);
    sdsfree(settingname);
    sdsfree(settingvalue);
    return rc;
}

bool mympd_api_cols_save(t_config *config, t_mympd_state *mympd_state, const char *table, const char *cols) {
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
    else if (strcmp(table, "colsBrowseDatabase") == 0) {
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
    else {
        sdsfree(tablename);
        return false;
    }
    
    if (!state_file_write(config, tablename, cols)) {
        sdsfree(tablename);
        return false;
    }
    sdsfree(tablename);
    return true;
}

bool mympd_api_settings_set(t_config *config, t_mympd_state *mympd_state, struct json_token *key, struct json_token *val) {
    sds settingname = sdsempty();
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    char *crap;

    LOG_DEBUG("Parse setting %.*s: %.*s", key->len, key->ptr, val->len, val->ptr);    
    if (strncmp(key->ptr, "notificationWeb", key->len) == 0) {
        mympd_state->notification_web = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(settingname, "notification_web");
    }
    else if (strncmp(key->ptr, "notificationPage", key->len) == 0) {
        mympd_state->notification_page = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(settingname, "notification_page");
    }
    else if (strncmp(key->ptr, "autoPlay", key->len) == 0) {
        mympd_state->auto_play = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(settingname, "auto_play");
    }
    else if (strncmp(key->ptr, "localplayerAutoplay", key->len) == 0) {
        mympd_state->localplayer_autoplay = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(settingname, "localplayer_autoplay");
    }
    else if (strncmp(key->ptr, "coverimage", key->len) == 0) {
        mympd_state->coverimage = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(settingname, "coverimage");
    }
    else if (strncmp(key->ptr, "coverimageName", key->len) == 0) {
        if (validate_string(settingvalue) && sdslen(settingvalue) > 0) {
            mympd_state->coverimage_name = sdsreplacelen(mympd_state->coverimage_name, settingvalue, sdslen(settingvalue));
            settingname = sdscat(settingname, "coverimage_name");
        }
        else {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
    }
    else if (strncmp(key->ptr, "coverimageSize", key->len) == 0) {
        mympd_state->coverimage_size = strtoimax(settingvalue, &crap, 10);
        settingname = sdscat(settingname, "coverimage_size");
    }
    else if (strncmp(key->ptr, "covergridSize", key->len) == 0) {
        mympd_state->covergrid_size = strtoimax(settingvalue, &crap, 10);
        settingname = sdscat(settingname, "covergrid_size");
    }
    else if (strncmp(key->ptr, "featLocalplayer", key->len) == 0) {
        mympd_state->localplayer = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(settingname, "localplayer");
    }
    else if (strncmp(key->ptr, "streamPort", key->len) == 0) {
        mympd_state->stream_port = strtoimax(settingvalue, &crap, 10);
        settingname = sdscat(settingname, "stream_port");
    }
    else if (strncmp(key->ptr, "streamUrl", key->len) == 0) {
        mympd_state->stream_url = sdsreplacelen(mympd_state->stream_url, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "stream_url");
    }
    else if (strncmp(key->ptr, "locale", key->len) == 0) {
        mympd_state->locale = sdsreplacelen(mympd_state->locale, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "locale");
    }
    else if (strncmp(key->ptr, "bgCover", key->len) == 0) {
        mympd_state->bg_cover = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(settingname, "bg_cover");
    }
    else if (strncmp(key->ptr, "bgColor", key->len) == 0) {
        mympd_state->bg_color = sdsreplacelen(mympd_state->bg_color, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "bg_color");
    }
    else if (strncmp(key->ptr, "bgCssFilter", key->len) == 0) {
        mympd_state->bg_css_filter = sdsreplacelen(mympd_state->bg_css_filter, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "bg_css_filter");
    }
    else if (strncmp(key->ptr, "jukeboxMode", key->len) == 0) {
        int jukebox_mode = strtoimax(settingvalue, &crap, 10);
        if (jukebox_mode < 0 || jukebox_mode > 2) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->jukebox_mode = jukebox_mode;
        settingname = sdscat(settingname, "jukebox_mode");
    }
    else if (strncmp(key->ptr, "jukeboxPlaylist", key->len) == 0) {
        mympd_state->jukebox_playlist = sdsreplacelen(mympd_state->jukebox_playlist, settingvalue, sdslen(settingvalue));
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
    else if (strncmp(key->ptr, "stickers", key->len) == 0) {
        mympd_state->stickers = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(settingname, "stickers");
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
    else if (strncmp(key->ptr, "taglist", key->len) == 0) {
        mympd_state->taglist = sdsreplacelen(mympd_state->taglist, settingvalue, sdslen(settingvalue));
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
        if (config->readonly == false) {
            mympd_state->smartpls = val->type == JSON_TYPE_TRUE ? true : false;
        }
        else {
            mympd_state->smartpls = false;
        }
        settingname = sdscat(settingname, "smartpls");
    }
    else if (strncmp(key->ptr, "maxElementsPerPage", key->len) == 0) {
        int max_elements_per_page = strtoimax(settingvalue, &crap, 10);
        if (max_elements_per_page <= 0 || max_elements_per_page > 999) {
            sdsfree(settingname);
            sdsfree(settingvalue);
            return false;
        }
        mympd_state->max_elements_per_page = max_elements_per_page;
        settingname = sdscat(settingname, "max_elements_per_page");
    }
    else if (strncmp(key->ptr, "love", key->len) == 0) {
        mympd_state->love = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(settingname, "love");
    }
    else if (strncmp(key->ptr, "loveChannel", key->len) == 0) {
        mympd_state->love_channel = sdsreplacelen(mympd_state->love_channel, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "love_channel");
    }
    else if (strncmp(key->ptr, "loveMessage", key->len) == 0) {
        mympd_state->love_message = sdsreplacelen(mympd_state->love_message, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "love_message");
    }
    else if (strncmp(key->ptr, "bookmarks", key->len) == 0) {
        if (config->readonly == false) {
            mympd_state->bookmarks = val->type == JSON_TYPE_TRUE ? true : false;
        }
        else {
            mympd_state->bookmarks = false;
        }
        settingname = sdscat(settingname, "bookmarks");
    }
    else if (strncmp(key->ptr, "theme", key->len) == 0) {
        mympd_state->love_message = sdsreplacelen(mympd_state->theme, settingvalue, sdslen(settingvalue));
        settingname = sdscat(settingname, "theme");
    }
    else {
        sdsfree(settingname);
        sdsfree(settingvalue);
        return true;
    }
    bool rc = state_file_write(config, settingname, settingvalue);
    sdsfree(settingname);
    sdsfree(settingvalue);
    return rc;
}

void mympd_api_settings_reset(t_config *config, t_mympd_state *mympd_state) {
    mympd_api_settings_delete(config);
    mympd_api_read_statefiles(config, mympd_state);
    mympd_api_push_to_mpd_client(mympd_state);
}

void mympd_api_read_statefiles(t_config *config, t_mympd_state *mympd_state) {
    LOG_INFO("Reading states");
    mympd_state->mpd_host = state_file_rw_string(config, "mpd_host", config->mpd_host, false);
    mympd_state->mpd_port = state_file_rw_int(config, "mpd_port", config->mpd_port, false);
    mympd_state->mpd_pass = state_file_rw_string(config, "mpd_pass", config->mpd_pass, false);
    mympd_state->stickers = state_file_rw_bool(config, "stickers", config->stickers, false);
    mympd_state->taglist = state_file_rw_string(config, "taglist", config->taglist, false);
    mympd_state->searchtaglist = state_file_rw_string(config, "searchtaglist", config->searchtaglist, false);
    mympd_state->browsetaglist = state_file_rw_string(config, "browsetaglist", config->browsetaglist, false);
    mympd_state->smartpls = state_file_rw_bool(config, "smartpls", config->smartpls, false);
    mympd_state->max_elements_per_page = state_file_rw_int(config, "max_elements_per_page", config->max_elements_per_page, false);
    mympd_state->last_played_count = state_file_rw_int(config, "last_played_count", config->last_played_count, false);
    mympd_state->love = state_file_rw_bool(config, "love", config->love, false);
    mympd_state->love_channel = state_file_rw_string(config, "love_channel", config->love_channel, false);
    mympd_state->love_message = state_file_rw_string(config, "love_message", config->love_message, false);
    mympd_state->notification_web = state_file_rw_bool(config, "notification_web", config->notification_web, false);
    mympd_state->notification_page = state_file_rw_bool(config, "notification_page", config->notification_page, false);
    mympd_state->auto_play = state_file_rw_bool(config, "auto_play", config->auto_play, false);
    mympd_state->jukebox_mode = state_file_rw_int(config, "jukebox_mode", config->jukebox_mode, false);
    mympd_state->jukebox_playlist = state_file_rw_string(config, "jukebox_playlist", config->jukebox_playlist, false);
    mympd_state->jukebox_queue_length = state_file_rw_int(config, "jukebox_queue_length", config->jukebox_queue_length, false);
    mympd_state->cols_queue_current = state_file_rw_string(config, "cols_queue_current", config->cols_queue_current, false);
    mympd_state->cols_search = state_file_rw_string(config, "cols_search", config->cols_search, false);
    mympd_state->cols_browse_database = state_file_rw_string(config, "cols_browse_database", config->cols_browse_database, false);
    mympd_state->cols_browse_playlists_detail = state_file_rw_string(config, "cols_browse_playlists_detail", config->cols_browse_playlists_detail, false);
    mympd_state->cols_browse_filesystem = state_file_rw_string(config, "cols_browse_filesystem", config->cols_browse_filesystem, false);
    mympd_state->cols_playback = state_file_rw_string(config, "cols_playback", config->cols_playback, false);
    mympd_state->cols_queue_last_played = state_file_rw_string(config, "cols_queue_last_played", config->cols_queue_last_played, false);
    mympd_state->localplayer = state_file_rw_bool(config, "localplayer", config->localplayer, false);
    mympd_state->localplayer_autoplay = state_file_rw_bool(config, "localplayer_autoplay", config->localplayer_autoplay, false);
    mympd_state->stream_port = state_file_rw_int(config, "stream_port", config->stream_port, false);
    mympd_state->stream_url = state_file_rw_string(config, "stream_url", config->stream_url, false);
    mympd_state->bg_cover = state_file_rw_bool(config, "bg_cover", config->bg_cover, false);
    mympd_state->bg_color = state_file_rw_string(config, "bg_color", config->bg_color, false);
    mympd_state->bg_css_filter = state_file_rw_string(config, "bg_css_filter", config->bg_css_filter, false);
    mympd_state->coverimage = state_file_rw_bool(config, "coverimage", config->coverimage, false);
    mympd_state->coverimage_name = state_file_rw_string(config, "coverimage_name", config->coverimage_name, false);
    mympd_state->coverimage_size = state_file_rw_int(config, "coverimage_size", config->coverimage_size, false);
    mympd_state->covergrid_size = state_file_rw_int(config, "covergrid_size", config->covergrid_size, false);
    mympd_state->locale = state_file_rw_string(config, "locale", config->locale, false);
    mympd_state->music_directory = state_file_rw_string(config, "music_directory", config->music_directory, false);
    mympd_state->bookmarks = state_file_rw_bool(config, "bookmarks", config->bookmarks, false);
    mympd_state->theme = state_file_rw_string(config, "theme", config->theme, false);
    if (config->readonly == true) {
        mympd_state->bookmarks = false;
        mympd_state->smartpls = false;
    }
}

sds state_file_rw_string(t_config *config, const char *name, const char *def_value, bool warn) {
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    
    sds result = sdsempty();
    
    if (!validate_string(name)) {
        return result;
    }
    
    sds cfg_file = sdscatfmt(sdsempty(), "%s/state/%s", config->varlibdir, name);
    FILE *fp = fopen(cfg_file, "r");
    sdsfree(cfg_file);
    if (fp == NULL) {
        if (warn == true) {
            LOG_WARN("Can't open %s", cfg_file);
        }
        state_file_write(config, name, def_value);
        result = sdscat(result, def_value);
        return result;
    }
    read = getline(&line, &n, fp);
    if (read > 0) {
        LOG_DEBUG("State %s: %s", name, line);
    }
    fclose(fp);
    if (read > 0) {
        result = sdscat(result, line);
        FREE_PTR(line);
        return result;
    }
    else {
        FREE_PTR(line);
        result = sdscat(result, def_value);
        return result;
    }
}

bool state_file_rw_bool(t_config *config, const char *name, const bool def_value, bool warn) {
    bool value = def_value;
    sds line = state_file_rw_string(config, name, def_value == true ? "true" : "false", warn);
    if (sdslen(line) > 0) {
        value = strcmp(line, "true") == 0 ? true : false;
        sdsfree(line);
    }
    return value;
}

int state_file_rw_int(t_config *config, const char *name, const int def_value, bool warn) {
    char *crap = NULL;
    int value = def_value;
    sds def_value_str = sdsfromlonglong(def_value);
    sds line = state_file_rw_string(config, name, def_value_str, warn);
    sdsfree(def_value_str);
    if (sdslen(line) > 0) {
        value = strtoimax(line, &crap, 10);
        sdsfree(line);
    }
    return value;
}

bool state_file_write(t_config *config, const char *name, const char *value) {
    if (config->readonly == true) {
        return true;
    }
    else if (!validate_string(name)) {
        return false;
    }
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/%s.XXXXXX", config->varlibdir, name);
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    fputs(value, fp);
    fclose(fp);
    sds cfg_file = sdscatfmt(sdsempty(), "%s/state/%s", config->varlibdir, name);
    if (rename(tmp_file, cfg_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, cfg_file);
        sdsfree(tmp_file);
        sdsfree(cfg_file);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(cfg_file);
    return true;
}

sds mympd_api_settings_put(t_config *config, t_mympd_state *mympd_state, sds buffer, sds method, int request_id) {
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",");
    buffer = tojson_char(buffer, "mpdHost", mympd_state->mpd_host, true);
    buffer = tojson_long(buffer, "mpdPort", mympd_state->mpd_port, true);
    buffer = tojson_char(buffer, "mpdPass", "dontsetpassword", true);
    buffer = tojson_bool(buffer, "featSyscmds", config->syscmds, true);
#ifdef ENABLE_SSL
    buffer = tojson_bool(buffer, "featCacert", (config->custom_cert == false && config->ssl == true ? true : false), true);
#else
    buffer = tojson_bool(buffer, "featCacert", false, true);
#endif
    buffer = tojson_bool(buffer, "featLocalplayer", mympd_state->localplayer, true);
    buffer = tojson_long(buffer, "streamPort", mympd_state->stream_port, true);
    buffer = tojson_char(buffer, "streamUrl", mympd_state->stream_url, true);
    buffer = tojson_bool(buffer, "coverimage", mympd_state->coverimage, true);
    buffer = tojson_char(buffer, "coverimageName", mympd_state->coverimage_name, true);
    buffer = tojson_long(buffer, "coverimageSize", mympd_state->coverimage_size, true);
    buffer = tojson_long(buffer, "covergridSize", mympd_state->covergrid_size, true);
    buffer = tojson_bool(buffer, "featMixramp", config->mixramp, true);
    buffer = tojson_long(buffer, "maxElementsPerPage", mympd_state->max_elements_per_page, true);
    buffer = tojson_bool(buffer, "notificationWeb", mympd_state->notification_web, true);
    buffer = tojson_bool(buffer, "notificationPage", mympd_state->notification_page, true);
    buffer = tojson_long(buffer, "jukeboxMode", mympd_state->jukebox_mode, true);
    buffer = tojson_char(buffer, "jukeboxPlaylist", mympd_state->jukebox_playlist, true);
    buffer = tojson_long(buffer, "jukeboxQueueLength", mympd_state->jukebox_queue_length, true);
    buffer = tojson_bool(buffer, "autoPlay", mympd_state->auto_play, true);
    buffer = tojson_char(buffer, "bgColor", mympd_state->bg_color, true);
    buffer = tojson_bool(buffer, "bgCover", mympd_state->bg_cover, true);
    buffer = tojson_char(buffer, "bgCssFilter", mympd_state->bg_css_filter, true);
    buffer = tojson_long(buffer, "loglevel", loglevel, true);
    buffer = tojson_char(buffer, "locale", mympd_state->locale, true);
    buffer = tojson_bool(buffer, "localplayerAutoplay", mympd_state->localplayer_autoplay, true);
    buffer = tojson_bool(buffer, "stickers", mympd_state->stickers, true);
    buffer = tojson_bool(buffer, "smartpls", mympd_state->smartpls, true);
    buffer = tojson_long(buffer, "lastPlayedCount", mympd_state->last_played_count, true);
    buffer = tojson_bool(buffer, "love", mympd_state->love, true);
    buffer = tojson_char(buffer, "loveChannel", mympd_state->love_channel, true);
    buffer = tojson_char(buffer, "loveMessage", mympd_state->love_message, true);
    buffer = tojson_char(buffer, "musicDirectory", mympd_state->music_directory, true);
    buffer = tojson_bool(buffer, "readonly", config->readonly, true);
    buffer = tojson_bool(buffer, "featBookmarks", mympd_state->bookmarks, true);
    buffer = tojson_long(buffer, "volumeStep", config->volume_step, true);
    buffer = tojson_bool(buffer, "publishLibrary", config->publish_library, true);
    buffer = tojson_char(buffer, "theme", mympd_state->theme, true);
    buffer = sdscatfmt(buffer, "\"colsQueueCurrent\":%s,", mympd_state->cols_queue_current);
    buffer = sdscatfmt(buffer, "\"colsSearch\":%s,", mympd_state->cols_search);
    buffer = sdscatfmt(buffer, "\"colsBrowseDatabase\":%s,", mympd_state->cols_browse_database);
    buffer = sdscatfmt(buffer, "\"colsBrowsePlaylistsDetail\":%s,", mympd_state->cols_browse_playlists_detail);
    buffer = sdscatfmt(buffer, "\"colsBrowseFilesystem\":%s,", mympd_state->cols_browse_filesystem);
    buffer = sdscatfmt(buffer, "\"colsPlayback\":%s,", mympd_state->cols_playback);
    buffer = sdscatfmt(buffer, "\"colsQueueLastPlayed\":%s", mympd_state->cols_queue_last_played);

    if (config->syscmds == true) {
        buffer = sdscat(buffer, ",\"syscmdList\":[");
        int nr = 0;
        struct node *current = config->syscmd_list.list;
        while (current != NULL) {
            if (nr++) {
                buffer = sdscat(buffer, ",");
            }
            buffer = sdscatjson(buffer, current->data, sdslen(current->data));
            current = current->next;
        }
        buffer = sdscat(buffer, "]");
    }

    buffer = jsonrpc_end_result(buffer);
    return buffer;
}
