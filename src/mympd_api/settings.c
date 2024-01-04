/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/settings.h"

#include "dist/mjson/mjson.h"
#include "src/lib/album_cache.h"
#include "src/lib/api.h"
#include "src/lib/convert.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/list.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/state_files.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/jukebox.h"
#include "src/mpd_client/presets.h"
#include "src/mpd_client/shortcuts.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/jukebox.h"
#include "src/mympd_api/timer.h"
#include "src/mympd_api/timer_handlers.h"
#include "src/mympd_api/trigger.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/**
 * Private declarations
 */

static void set_invalid_value(struct t_jsonrpc_parse_error *error, const char *path, sds key, sds value, const char *message);
static void set_invalid_field(struct t_jsonrpc_parse_error *error, const char *path, sds key);
static void enable_set_conn_options(struct t_mympd_state *mympd_state);

/**
 * Pushes some settings to the webserver thread
 * @param mympd_state pointer to central myMPD state
 * @return true on success, else false
 */
bool settings_to_webserver(struct t_mympd_state *mympd_state) {
    struct set_mg_user_data_request *extra = malloc_assert(sizeof(struct set_mg_user_data_request));
    extra->music_directory = sdsdup(mympd_state->mpd_state->music_directory_value);
    extra->playlist_directory = sdsdup(mympd_state->mpd_state->playlist_directory_value);
    extra->coverimage_names = sdsdup(mympd_state->coverimage_names);
    extra->thumbnail_names = sdsdup(mympd_state->thumbnail_names);
    extra->feat_albumart = mympd_state->mpd_state->feat.albumart;
    extra->mpd_host = sdsdup(mympd_state->mpd_state->mpd_host);
    list_init(&extra->partitions);
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        if (sdslen(partition_state->stream_uri) == 0) {
            //ignore custom stream uris
            list_push(&extra->partitions, partition_state->name, (int64_t)partition_state->mpd_stream_port, NULL, NULL);
        }
        partition_state = partition_state->next;
    }
    struct t_work_response *web_server_response = create_response_new(RESPONSE_TYPE_PUSH_CONFIG, 0, 0, INTERNAL_API_WEBSERVER_SETTINGS, MPD_PARTITION_DEFAULT);
    web_server_response->extra = extra;
    return mympd_queue_push(web_server_queue, web_server_response, 0);
}

/**
 * Saves connection specific settings
 * @param path jsonrpc path
 * @param key setting key
 * @param value setting value
 * @param vtype value type
 * @param vcb validation callback (unused)
 * @param userdata pointer to the t_mympd_state struct
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success, else false
 */
bool mympd_api_settings_connection_save(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error) {
    (void) vcb;
    struct t_mympd_state *mympd_state = (struct t_mympd_state *)userdata;

    MYMPD_LOG_DEBUG(NULL, "Parse setting \"%s\": \"%s\" (%s)", key, value, get_mjson_toktype_name(vtype));

    if ((strcmp(key, "mpdHost") == 0 || strcmp(key, "stickerdbMpdHost") == 0) && 
        vtype == MJSON_TOK_STRING)
    {
        if (vcb_isfilepath(value) == false) {
            set_invalid_value(error, path, key, value, "Must be a socket, ip-address or dns-name");
            return false;
        }
        if (strcmp(key, "mpdHost") == 0) {
            mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, value);
        }
        else {
            // stickerdb connection
            mympd_state->stickerdb->mpd_state->mpd_host = sds_replace(mympd_state->stickerdb->mpd_state->mpd_host, value);
        }
    }
    else if ((strcmp(key, "mpdPort") == 0 || strcmp(key, "stickerdbMpdPort") == 0) &&
        vtype == MJSON_TOK_NUMBER)
    {
        unsigned mpd_port;
        enum str2int_errno rc = str2uint(&mpd_port, value);
        if (rc != STR2INT_SUCCESS || mpd_port < MPD_PORT_MIN || mpd_port > MPD_PORT_MAX) {
            set_invalid_value(error, path, key, value, "Allowed port range is between 1024 and 65535");
            return false;
        }
        if (strcmp(key, "mpdPort") == 0) {
            mympd_state->mpd_state->mpd_port = mpd_port;
        }
        else {
            // stickerdb connection
            mympd_state->stickerdb->mpd_state->mpd_port = mpd_port;
        }
    }
    else if ((strcmp(key, "mpdPass") == 0 || strcmp(key, "stickerdbMpdPass") == 0) &&
        vtype == MJSON_TOK_STRING)
    {
        if (strcmp(value, "dontsetpassword") != 0) {
            if (vcb_isname(value) == false) {
                set_invalid_value(error, path, key, value, "Invalid characters in MPD password");
                return false;
            }
            if (strcmp(key, "mpdPass") == 0) {
                mympd_state->mpd_state->mpd_pass = sds_replace(mympd_state->mpd_state->mpd_pass, value);
            }
            else {
                // stickerdb connection
                mympd_state->stickerdb->mpd_state->mpd_pass = sds_replace(mympd_state->stickerdb->mpd_state->mpd_pass, value);
            }
        }
        else {
            //keep old password
            return true;
        }
    }
    else if ((strcmp(key, "mpdTimeout") == 0 || strcmp(key, "stickerdbMpdTimeout") == 0) &&
        vtype == MJSON_TOK_NUMBER)
    {
        unsigned mpd_timeout;
        enum str2int_errno rc = str2uint(&mpd_timeout, value);
        if (rc != STR2INT_SUCCESS || mpd_timeout < MPD_TIMEOUT_MIN || mpd_timeout > MPD_TIMEOUT_MAX) {
            set_invalid_value(error, path, key, value, "Must be a number between 10 and 1000");
            return false;
        }
        if (strcmp(key, "mpdTimeout") == 0) {
            if (mpd_timeout != mympd_state->mpd_state->mpd_timeout) {
                mympd_state->mpd_state->mpd_timeout = mpd_timeout;
                enable_set_conn_options(mympd_state);
            }
        }
        else {
            // stickerdb connection
            mympd_state->stickerdb->mpd_state->mpd_timeout = mpd_timeout;
            // this disconnects the stickerdb connection on next stickerdb call
            mympd_state->stickerdb->conn_state = MPD_FAILURE;
        }
    }
    else if (strcmp(key, "mpdKeepalive") == 0 || strcmp(key, "stickerdbMpdKeepalive") == 0) {
        if (vtype != MJSON_TOK_TRUE && vtype != MJSON_TOK_FALSE) {
            set_invalid_value(error, path, key, value, "Must be a boolean value");
            return false;
        }
        bool keepalive = vtype == MJSON_TOK_TRUE
            ? true
            : false;
        if (strcmp(key, "mpdKeepalive") == 0) {
            if (keepalive != mympd_state->mpd_state->mpd_keepalive) {
                mympd_state->mpd_state->mpd_keepalive = keepalive;
                enable_set_conn_options(mympd_state);
            }
        }
        else {
            // stickerdb connection
            mympd_state->stickerdb->mpd_state->mpd_keepalive = keepalive;
            // this disconnects the stickerdb connection on next stickerdb call
            mympd_state->stickerdb->conn_state = MPD_FAILURE;
        }
    }
    else if (strcmp(key, "mpdBinarylimit") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned binarylimit;
        enum str2int_errno rc = str2uint(&binarylimit, value);
        if (rc != STR2INT_SUCCESS || binarylimit < MPD_BINARY_CHUNK_SIZE_MIN || binarylimit > MPD_BINARY_CHUNK_SIZE_MAX) {
            set_invalid_value(error, path, key, value, "Number is out of valid range");
            return false;
        }
        if (binarylimit != mympd_state->mpd_state->mpd_binarylimit) {
            mympd_state->mpd_state->mpd_binarylimit = binarylimit;
            enable_set_conn_options(mympd_state);
        }
    }
    else if (strcmp(key, "musicDirectory") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilepath(value) == false) {
            set_invalid_value(error, path, key, value, "Must be none, auto or an absolute path");
            return false;
        }
        mympd_state->music_directory = sds_replace(mympd_state->music_directory, value);
        strip_slash(mympd_state->music_directory);
    }
    else if (strcmp(key, "playlistDirectory") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilepath(value) == false) {
            set_invalid_value(error, path, key, value, "Must be none, auto or an absolute path");
            return false;
        }
        mympd_state->playlist_directory = sds_replace(mympd_state->playlist_directory, value);
        strip_slash(mympd_state->playlist_directory);
    }
    else {
        set_invalid_field(error, path, key);
        return false;
    }

    sds state_filename = camel_to_snake(key);
    bool rc = state_file_write(mympd_state->config->workdir, DIR_WORK_STATE, state_filename, value);
    FREE_SDS(state_filename);

    return rc;
}

/**
 * Saves columns
 * @param mympd_state pointer to the t_mympd_state struct
 * @param table save columns for this table
 * @param cols json object with the columns
 * @return true on success, else false
 */
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
    else if (strcmp(table, "colsBrowseDatabaseAlbumDetailInfo") == 0) {
        mympd_state->cols_browse_database_album_detail_info = sds_replace(mympd_state->cols_browse_database_album_detail_info, cols);
    }
    else if (strcmp(table, "colsBrowseDatabaseAlbumDetail") == 0) {
        mympd_state->cols_browse_database_album_detail = sds_replace(mympd_state->cols_browse_database_album_detail, cols);
    }
    else if (strcmp(table, "colsBrowseDatabaseAlbumList") == 0) {
        mympd_state->cols_browse_database_album_list = sds_replace(mympd_state->cols_browse_database_album_list, cols);
    }
    else if (strcmp(table, "colsBrowsePlaylistDetail") == 0) {
        mympd_state->cols_browse_playlist_detail = sds_replace(mympd_state->cols_browse_playlist_detail, cols);
    }
    else if (strcmp(table, "colsBrowseFilesystem") == 0) {
        mympd_state->cols_browse_filesystem = sds_replace(mympd_state->cols_browse_filesystem, cols);
    }
    else if (strcmp(table, "colsPlayback") == 0) {
        mympd_state->cols_playback = sds_replace(mympd_state->cols_playback, cols);
    }
    else if (strcmp(table, "colsQueueJukeboxSong") == 0) {
        mympd_state->cols_queue_jukebox_song = sds_replace(mympd_state->cols_queue_jukebox_song, cols);
    }
    else if (strcmp(table, "colsQueueJukeboxAlbum") == 0) {
        mympd_state->cols_queue_jukebox_album = sds_replace(mympd_state->cols_queue_jukebox_album, cols);
    }
    else if (strcmp(table, "colsBrowseRadioWebradiodb") == 0) {
        mympd_state->cols_browse_radio_webradiodb = sds_replace(mympd_state->cols_browse_radio_webradiodb, cols);
    }
    else if (strcmp(table, "colsBrowseRadioRadiobrowser") == 0) {
        mympd_state->cols_browse_radio_radiobrowser = sds_replace(mympd_state->cols_browse_radio_radiobrowser, cols);
    }
    else {
        MYMPD_LOG_ERROR(NULL, "MYMPD_API_COLS_SAVE: Unknown table %s", table);
        return false;
    }
    sds tablename = camel_to_snake(table);
    bool rc = state_file_write(mympd_state->config->workdir, DIR_WORK_STATE, tablename, cols);
    FREE_SDS(tablename);
    return rc;
}

/**
 * Saves settings
 * @param path jsonrpc path
 * @param key setting key
 * @param value setting value
 * @param vtype value type
 * @param vcb validation callback (unused)
 * @param userdata pointer to central myMPD state
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success, else false
 */
bool mympd_api_settings_set(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error) {
    (void) vcb;
    struct t_mympd_state *mympd_state = (struct t_mympd_state *)userdata;

    MYMPD_LOG_DEBUG(NULL, "Parse setting \"%s\": \"%s\" (%s)", key, value, get_mjson_toktype_name(vtype));

    if (strcmp(key, "coverimageNames") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilename(value) == true) {
            mympd_state->coverimage_names = sds_replace(mympd_state->coverimage_names, value);
        }
        else {
            set_invalid_value(error, path, key, value, "Only filenames allowed");
            return false;
        }
    }
    else if (strcmp(key, "thumbnailNames") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilename(value) == true) {
            mympd_state->thumbnail_names = sds_replace(mympd_state->thumbnail_names, value);
        }
        else {
            set_invalid_value(error, path, key, value, "Only filenames allowed");
            return false;
        }
    }
    else if (strcmp(key, "bookletName") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilename(value) == true) {
            mympd_state->booklet_name = sds_replace(mympd_state->booklet_name, value);
        }
        else {
            set_invalid_value(error, path, key, value, "Must be a valid filename");
            return false;
        }
    }
    else if (strcmp(key, "infoTxtName") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilename(value) == true) {
            mympd_state->info_txt_name = sds_replace(mympd_state->info_txt_name, value);
        }
        else {
            set_invalid_value(error, path, key, value, "Must be a valid filename");
            return false;
        }
    }
    else if (strcmp(key, "lastPlayedCount") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned last_played_count;
        enum str2int_errno rc = str2uint(&last_played_count, value);
        if (rc != STR2INT_SUCCESS || last_played_count > MPD_PLAYLIST_LENGTH_MAX) {
            set_invalid_value(error, path, key, value, "Must be zero or a positive number");
            return false;
        }
        mympd_state->last_played_count = last_played_count;
    }
    else if (strcmp(key, "volumeMin") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned volume_min;
        enum str2int_errno rc = str2uint(&volume_min, value);
        if (rc != STR2INT_SUCCESS || volume_min > VOLUME_MAX) {
            set_invalid_value(error, path, key, value, "Must be a number between 0 and 100");
            return false;
        }
        mympd_state->volume_min = volume_min;
    }
    else if (strcmp(key, "volumeMax") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned volume_max;
        enum str2int_errno rc = str2uint(&volume_max, value);
        if (rc != STR2INT_SUCCESS || volume_max > VOLUME_MAX) {
            set_invalid_value(error, path, key, value, "Must be a number between 0 and 100");
            return false;
        }
        mympd_state->volume_max = volume_max;
    }
    else if (strcmp(key, "volumeStep") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned volume_step;
        enum str2int_errno rc = str2uint(&volume_step, value);
        if (rc != STR2INT_SUCCESS || volume_step < VOLUME_STEP_MIN || volume_step > VOLUME_STEP_MAX) {
            set_invalid_value(error, path, key, value, "Must be a number between 1 and 25");
            return false;
        }
        mympd_state->volume_step = volume_step;
    }
    else if (strcmp(key, "tagList") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_istaglist(value) == false) {
            set_invalid_value(error, path, key, value, "Must be a list of MPD tags");
            return false;
        }
        mympd_state->mpd_state->tag_list = sds_replace(mympd_state->mpd_state->tag_list, value);
        //set enabled tags for all connections
        enable_set_conn_options(mympd_state);
    }
    else if (strcmp(key, "tagListSearch") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_istaglist(value) == false) {
            set_invalid_value(error, path, key, value, "Must be a list of MPD tags");
            return false;
        }
        mympd_state->tag_list_search = sds_replace(mympd_state->tag_list_search, value);
    }
    else if (strcmp(key, "tagListBrowse") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_istaglist(value) == false) {
            set_invalid_value(error, path, key, value, "Must be a list of MPD tags");
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
            set_invalid_value(error, path, key, value, "Must be a boolean value");
            return false;
        }
    }
    else if (strcmp(key, "smartplsSort") == 0 && vtype == MJSON_TOK_STRING) {
        if (sdslen(value) > 0 && vcb_ismpdsort(value) == false) {
            set_invalid_value(error, path, key, value, "Only sort tags allowed");
            return false;
        }
        mympd_state->smartpls_sort = sds_replace(mympd_state->smartpls_sort, value);
    }
    else if (strcmp(key, "smartplsPrefix") == 0 && vtype == MJSON_TOK_STRING) {
        if (sdslen(value) > 0 &&
            vcb_isfilename(value) == false)
        {
            set_invalid_value(error, path, key, value, "Must be a valid filename");
            return false;
        }
        mympd_state->smartpls_prefix = sds_replacelen(mympd_state->smartpls_prefix, value, sdslen(value));
    }
    else if (strcmp(key, "smartplsInterval") == 0 && vtype == MJSON_TOK_NUMBER) {
        int interval;
        enum str2int_errno rc = str2int(&interval, value);
        if (rc != STR2INT_SUCCESS || interval < TIMER_INTERVAL_MIN || interval > TIMER_INTERVAL_MAX) {
            set_invalid_value(error, path, key, value, "Must be zero or a positive number");
            return false;
        }
        if (interval != mympd_state->smartpls_interval) {
            mympd_state->smartpls_interval = interval;
            if (interval > 0) {
                mympd_api_timer_replace(&mympd_state->timer_list, interval, interval, timer_handler_by_id, TIMER_ID_SMARTPLS_UPDATE, NULL);
            }
            else {
                mympd_api_timer_remove(&mympd_state->timer_list, TIMER_ID_SMARTPLS_UPDATE);
            }
        }
    }
    else if (strcmp(key, "smartplsGenerateTagList") == 0 && vtype == MJSON_TOK_STRING) {
        if (sdslen(value) > 0 && vcb_istaglist(value) == false) {
            set_invalid_value(error, path, key, value, "Must be a list of MPD tags");
            return false;
        }
        mympd_state->smartpls_generate_tag_list = sds_replacelen(mympd_state->smartpls_generate_tag_list, value, sdslen(value));
    }
    else if (strcmp(key, "webuiSettings") == 0 && vtype == MJSON_TOK_OBJECT) {
        // we save the webui settings without validation, validation is done on client-side
        mympd_state->webui_settings = sds_replacelen(mympd_state->webui_settings, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsUsltExt") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            set_invalid_value(error, path, key, value, "Must be a valid file extension");
            return false;
        }
        mympd_state->lyrics.uslt_ext = sds_replacelen(mympd_state->lyrics.uslt_ext, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsSyltExt") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            set_invalid_value(error, path, key, value, "Must be a valid file extension");
            return false;
        }
        mympd_state->lyrics.sylt_ext = sds_replacelen(mympd_state->lyrics.sylt_ext, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsVorbisUslt") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            set_invalid_value(error, path, key, value, "Must be a valid tag name");
            return false;
        }
        mympd_state->lyrics.vorbis_uslt = sds_replacelen(mympd_state->lyrics.vorbis_uslt, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsVorbisSylt") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            set_invalid_value(error, path, key, value, "Must be a valid tag name");
            return false;
        }
        mympd_state->lyrics.vorbis_sylt = sds_replacelen(mympd_state->lyrics.vorbis_sylt, value, sdslen(value));
    }
    else if (strcmp(key, "listenbrainzToken") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            set_invalid_value(error, path, key, value, "Must be an alphanumeric value");
            return false;
        }
        mympd_state->listenbrainz_token = sds_replacelen(mympd_state->listenbrainz_token, value, sdslen(value));
    }
    else if (strcmp(key, "tagDiscEmptyIsFirst") == 0) {
        if (vtype == MJSON_TOK_TRUE) {
            mympd_state->tag_disc_empty_is_first = true;
        }
        else if (vtype == MJSON_TOK_FALSE) {
            mympd_state->tag_disc_empty_is_first = false;
        }
        else {
            set_invalid_value(error, path, key, value, "Must be a boolean value");
            return false;
        }
    }
    else {
        set_invalid_field(error, path, key);
        return false;
    }
    sds state_filename = camel_to_snake(key);
    bool rc = state_file_write(mympd_state->config->workdir, DIR_WORK_STATE, state_filename, value);
    FREE_SDS(state_filename);
    return rc;
}

/**
 * Saves partition settings
 * @param path jsonrpc path
 * @param key setting key
 * @param value setting value
 * @param vtype value type
 * @param vcb validation callback (unused)
 * @param userdata pointer to partition state
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success, else false
 */
bool mympd_api_settings_partition_set(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error) {
    (void) vcb;
    struct t_partition_state *partition_state = (struct t_partition_state *)userdata;

    MYMPD_LOG_DEBUG(partition_state->name, "Parse setting \"%s\": \"%s\" (%s)", key, value, get_mjson_toktype_name(vtype));

    if (strcmp(key, "highlightColor") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_ishexcolor(value) == false) {
            set_invalid_value(error, path, key, value, "Must be a hex color value");
            return false;
        }
        partition_state->highlight_color = sds_replace(partition_state->highlight_color, value);
    }
    else if (strcmp(key, "highlightColorContrast") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_ishexcolor(value) == false) {
            set_invalid_value(error, path, key, value, "Must be a hex color value");
            return false;
        }
        partition_state->highlight_color_contrast = sds_replace(partition_state->highlight_color_contrast, value);
    }
    else if (strcmp(key, "mpdStreamPort") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned mpd_stream_port;
        enum str2int_errno rc = str2uint(&mpd_stream_port, value);
        if (rc != STR2INT_SUCCESS || mpd_stream_port < MPD_PORT_MIN || mpd_stream_port > MPD_PORT_MAX) {
            set_invalid_value(error, path, key, value, "Allowed port range is between 1024 and 65535");
            return false;
        }
        partition_state->mpd_stream_port = mpd_stream_port;
    }
    else if (strcmp(key, "streamUri") == 0 && vtype == MJSON_TOK_STRING) {
        if (sdslen(value) > 0 &&
            vcb_isstreamuri(value) == false)
        {
            set_invalid_value(error, path, key, value, "Must be a valid uri");
            return false;
        }
        partition_state->stream_uri = sds_replace(partition_state->stream_uri, value);
    }
    else {
        set_invalid_field(error, path, key);
        return false;
    }
    sds state_filename = camel_to_snake(key);
    bool rc = state_file_write(partition_state->config->workdir, partition_state->state_dir, state_filename, value);
    FREE_SDS(state_filename);
    return rc;
}

/**
 * Sets mpd options and jukebox settings
 * @param path jsonrpc path
 * @param key setting key
 * @param value setting value
 * @param vtype value type
 * @param vcb validation callback (unused)
 * @param userdata pointer to the t_partition_state struct
 * @param error pointer to t_jsonrpc_parse_error
 * @return true on success, else false
 */
bool mympd_api_settings_mpd_options_set(const char *path, sds key, sds value, int vtype, validate_callback vcb, void *userdata, struct t_jsonrpc_parse_error *error) {
    (void) vcb;
    struct t_partition_state *partition_state = (struct t_partition_state *)userdata;

    bool rc = false;
    bool write_state_file = true;
    bool jukebox_changed = false;

    MYMPD_LOG_DEBUG(partition_state->name, "Parse setting \"%s\": \"%s\" (%s)", key, value, get_mjson_toktype_name(vtype));
    if (strcmp(key, "autoPlay") == 0) {
        if (vtype == MJSON_TOK_TRUE) {
            partition_state->auto_play = true;
        }
        else if (vtype == MJSON_TOK_FALSE) {
            partition_state->auto_play = false;
        }
        else {
            set_invalid_value(error, path, key, value, "Must be a boolean value");
            return false;
        }
    }
    else if (strcmp(key, "jukeboxMode") == 0 && vtype == MJSON_TOK_STRING) {
        enum jukebox_modes jukebox_mode = jukebox_mode_parse(value);

        if (jukebox_mode == JUKEBOX_UNKNOWN) {
            set_invalid_value(error, path, key, value, "Invalid jukebox mode");
            return false;
        }
        if (partition_state->jukebox.mode != jukebox_mode) {
            partition_state->jukebox.mode = jukebox_mode;
            jukebox_changed = true;
        }
        sdsclear(value);
        value = sdscatfmt(value, "%i", jukebox_mode);
    }
    else if (strcmp(key, "jukeboxPlaylist") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilename(value) == false) {
            set_invalid_value(error, path, key, value, "Must be valid MPD playlist");
            return false;
        }
        if (strcmp(partition_state->jukebox.playlist, value) != 0) {
            partition_state->jukebox.playlist = sds_replace(partition_state->jukebox.playlist, value);
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxQueueLength") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned jukebox_queue_length;
        enum str2int_errno crc = str2uint(&jukebox_queue_length, value);
        if (crc != STR2INT_SUCCESS || jukebox_queue_length <= JUKEBOX_QUEUE_MIN || jukebox_queue_length > JUKEBOX_QUEUE_MAX) {
            set_invalid_value(error, path, key, value, "Must be a number between 1 and 999");
            return false;
        }
        partition_state->jukebox.queue_length = jukebox_queue_length;
    }
    else if (strcmp(key, "jukeboxUniqTag") == 0 && vtype == MJSON_TOK_STRING) {
        enum mpd_tag_type uniq_tag = mpd_tag_name_parse(value);
        if (uniq_tag == MPD_TAG_UNKNOWN) {
            set_invalid_value(error, path, key, value, "Must be a valid tag name");
            return false;
        }
        if (partition_state->jukebox.uniq_tag.tags[0] != uniq_tag) {
            partition_state->jukebox.uniq_tag.tags[0] = uniq_tag;
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxLastPlayed") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned jukebox_last_played;
        enum str2int_errno crc = str2uint(&jukebox_last_played, value);
        if (crc != STR2INT_SUCCESS || jukebox_last_played > JUKEBOX_LAST_PLAYED_MAX) {
            set_invalid_value(error, path, key, value, "Must be a number between 0 and 5000");
            return false;
        }
        if (jukebox_last_played != partition_state->jukebox.last_played) {
            partition_state->jukebox.last_played = jukebox_last_played;
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxIgnoreHated") == 0) {
        if (vtype != MJSON_TOK_TRUE && vtype != MJSON_TOK_FALSE) {
            set_invalid_value(error, path, key, value, "Must be a boolean value");
            return false;
        }
        bool bool_buf = vtype == MJSON_TOK_TRUE ? true : false;
        if (bool_buf != partition_state->jukebox.ignore_hated) {
            partition_state->jukebox.ignore_hated = bool_buf;
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxFilterInclude") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_issearchexpression(value) == false) {
            set_invalid_value(error, path, key, value, "Invalid MPD search expression");
            return false;
        }
        if (strcmp(partition_state->jukebox.filter_include, value) != 0) {
            partition_state->jukebox.filter_include = sds_replace(partition_state->jukebox.filter_include, value);
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxFilterExclude") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_issearchexpression(value) == false) {
            set_invalid_value(error, path, key, value, "Invalid MPD search expression");
            return false;
        }
        if (strcmp(partition_state->jukebox.filter_exclude, value) != 0) {
            partition_state->jukebox.filter_exclude = sds_replace(partition_state->jukebox.filter_exclude, value);
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxMinSongDuration") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned min_song_duration;
        enum str2int_errno crc = str2uint(&min_song_duration, value);
        if (crc != STR2INT_SUCCESS || min_song_duration > JUKEBOX_MIN_SONG_DURATION_MAX) {
            set_invalid_value(error, path, key, value, "Invalid value");
            return false;
        }
        if (min_song_duration != partition_state->jukebox.min_song_duration) {
            partition_state->jukebox.min_song_duration = min_song_duration;
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxMaxSongDuration") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned max_song_duration;
        enum str2int_errno crc = str2uint(&max_song_duration, value);
        if (crc != STR2INT_SUCCESS || max_song_duration > JUKEBOX_MAX_SONG_DURATION_MAX) {
            set_invalid_value(error, path, key, value, "Invalid value");
            return false;
        }
        if (max_song_duration != partition_state->jukebox.max_song_duration) {
            partition_state->jukebox.max_song_duration = max_song_duration;
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "name") == 0) {
        //ignore
        rc = true;
    }
    else if (partition_state->conn_state == MPD_CONNECTED) {
        if (strcmp(key, "random") == 0) {
            if (vtype != MJSON_TOK_TRUE && vtype != MJSON_TOK_FALSE) {
                set_invalid_value(error, path, key, value, "Must be a boolean value");
                return false;
            }
            bool bool_buf = vtype == MJSON_TOK_TRUE ? true : false;
            mpd_run_random(partition_state->conn, bool_buf);
        }
        else if (strcmp(key, "repeat") == 0) {
            if (vtype != MJSON_TOK_TRUE && vtype != MJSON_TOK_FALSE) {
                set_invalid_value(error, path, key, value, "Must be a boolean value");
                return false;
            }
            bool bool_buf = vtype == MJSON_TOK_TRUE ? true : false;
             mpd_run_repeat(partition_state->conn, bool_buf);
        }
        else if (strcmp(key, "consume") == 0) {
            enum mpd_consume_state state = mpd_parse_consume_state(value);
            if (state == MPD_CONSUME_UNKNOWN) {
                set_invalid_value(error, path, key, value, "Must be on, off or oneshot");
                return false;
            }
            mpd_run_consume_state(partition_state->conn, state);
        }
        else if (strcmp(key, "single") == 0) {
            enum mpd_single_state state = mpd_parse_single_state(value);
            if (state == MPD_SINGLE_UNKNOWN) {
                set_invalid_value(error, path, key, value, "Must be on, off or oneshot");
                return false;
            }
            mpd_run_single_state(partition_state->conn, state);
        }
        else if (strcmp(key, "crossfade") == 0 && vtype == MJSON_TOK_NUMBER) {
            unsigned uint_buf;
            enum str2int_errno crc = str2uint(&uint_buf, value);
            if (crc != STR2INT_SUCCESS || uint_buf > MPD_CROSSFADE_MAX) {
                set_invalid_value(error, path, key, value, "Must be a number between 0 and 100");
                return false;
            }
            mpd_run_crossfade(partition_state->conn, uint_buf);
        }
        else if (strcmp(key, "replaygain") == 0 && vtype == MJSON_TOK_STRING) {
            enum mpd_replay_gain_mode mode = mpd_parse_replay_gain_name(value);
            if (mode == MPD_REPLAY_UNKNOWN) {
                set_invalid_value(error, path, key, value, "Must be off, auto, track or album");
                return false;
            }
            mpd_run_replay_gain_mode(partition_state->conn, mode);
        }
        else if (strcmp(key, "mixrampDb") == 0 && vtype == MJSON_TOK_NUMBER) {
            float db;
            enum str2int_errno crc = str2float(&db, value);
            if (crc != STR2INT_SUCCESS || db < -100 || db > 0) {
                // mixramp db should be a negative value
                // 0 means mixrampdb is disabled.
                set_invalid_value(error, path, key, value, "Must be a number between -100 and 0");
                return false;
            }
            mpd_run_mixrampdb(partition_state->conn, db);
        }
        else if (strcmp(key, "mixrampDelay") == 0 && vtype == MJSON_TOK_NUMBER) {
            float delay;
            enum str2int_errno crc = str2float(&delay, value);
            if (crc != STR2INT_SUCCESS || delay < -1.0 || delay > 100) {
                // mixramp delay should be a positive value
                // 0 disables mixramp
                // Negative means mixrampdelay is disabled
                set_invalid_value(error, path, key, value, "Must be a number between -1 and 100");
                return false;
            }
            mpd_run_mixrampdelay(partition_state->conn, delay);
        }
        sds message = sdsempty();
        rc = mympd_check_error_and_recover_notify(partition_state, &message, key);
        if (rc == false) {
            ws_notify(message, partition_state->name);
        }
        FREE_SDS(message);
        write_state_file = false;
    }
    else {
        MYMPD_LOG_WARN(partition_state->name, "Unknown setting \"%s\": \"%s\"", key, value);
        return false;
    }
    if (jukebox_changed == true && partition_state->jukebox.queue->length > 0) {
        MYMPD_LOG_INFO(partition_state->name, "Jukebox options changed, clearing jukebox queue");
        mympd_api_jukebox_clear(partition_state->jukebox.queue, partition_state->name);
    }
    if (write_state_file == true) {
        sds state_filename = camel_to_snake(key);
        rc = state_file_write(partition_state->config->workdir, partition_state->state_dir, state_filename, value);
        FREE_SDS(state_filename);
    }
    return rc;
}

/**
 * Reads the settings from the state files.
 * If the state file does not exist, it is populated with the default value and created.
 * @param mympd_state pointer to the t_mympd_state struct
 */
void mympd_api_settings_statefiles_global_read(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_NOTICE(NULL, "Reading global states");
    sds workdir = mympd_state->config->workdir;
    // mpd connection
    mympd_state->mpd_state->mpd_host = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "mpd_host", mympd_state->mpd_state->mpd_host, vcb_isname, true);
    mympd_state->mpd_state->mpd_port = state_file_rw_uint(workdir, DIR_WORK_STATE, "mpd_port", mympd_state->mpd_state->mpd_port, MPD_PORT_MIN, MPD_PORT_MAX, true);
    mympd_state->mpd_state->mpd_pass = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "mpd_pass", mympd_state->mpd_state->mpd_pass, vcb_isname, true);
    mympd_state->mpd_state->mpd_binarylimit = state_file_rw_uint(workdir, DIR_WORK_STATE, "mpd_binarylimit", mympd_state->mpd_state->mpd_binarylimit, MPD_BINARY_CHUNK_SIZE_MIN, MPD_BINARY_CHUNK_SIZE_MAX, true);
    mympd_state->mpd_state->mpd_timeout = state_file_rw_uint(workdir, DIR_WORK_STATE, "mpd_timeout", mympd_state->mpd_state->mpd_timeout, MPD_TIMEOUT_MIN, MPD_TIMEOUT_MAX, true);
    mympd_state->mpd_state->mpd_keepalive = state_file_rw_bool(workdir, DIR_WORK_STATE, "mpd_keepalive", mympd_state->mpd_state->mpd_keepalive, true);
    // stickerdb connection, use mpd connection settings as default
    mympd_state->stickerdb->mpd_state->mpd_host = sds_replace(mympd_state->stickerdb->mpd_state->mpd_host, mympd_state->mpd_state->mpd_host);
    mympd_state->stickerdb->mpd_state->mpd_pass = sds_replace(mympd_state->stickerdb->mpd_state->mpd_pass, mympd_state->mpd_state->mpd_pass);

    mympd_state->stickerdb->mpd_state->mpd_host = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "stickerdb_mpd_host", mympd_state->stickerdb->mpd_state->mpd_host, vcb_isname, true);
    mympd_state->stickerdb->mpd_state->mpd_port = state_file_rw_uint(workdir, DIR_WORK_STATE, "stickerdb_mpd_port", mympd_state->mpd_state->mpd_port, MPD_PORT_MIN, MPD_PORT_MAX, true);
    mympd_state->stickerdb->mpd_state->mpd_pass = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "stickerdb_mpd_pass", mympd_state->stickerdb->mpd_state->mpd_pass, vcb_isname, true);
    mympd_state->stickerdb->mpd_state->mpd_timeout = state_file_rw_uint(workdir, DIR_WORK_STATE, "stickerdb_mpd_timeout", mympd_state->mpd_state->mpd_timeout, MPD_TIMEOUT_MIN, MPD_TIMEOUT_MAX, true);
    mympd_state->stickerdb->mpd_state->mpd_keepalive = state_file_rw_bool(workdir, DIR_WORK_STATE, "stickerdb_mpd_keepalive", mympd_state->mpd_state->mpd_keepalive, true);
    // other settings
    mympd_state->mpd_state->tag_list = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "tag_list", mympd_state->mpd_state->tag_list, vcb_istaglist, true);
    mympd_state->last_played_count = state_file_rw_uint(workdir, DIR_WORK_STATE, "last_played_count", mympd_state->last_played_count, 0, MPD_PLAYLIST_LENGTH_MAX, true);
    mympd_state->booklet_name = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "booklet_name", mympd_state->booklet_name, vcb_isfilename, true);
    mympd_state->info_txt_name = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "info_txt_name", mympd_state->info_txt_name, vcb_isfilename, true);
    mympd_state->tag_list_search = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "tag_list_search", mympd_state->tag_list_search, vcb_istaglist, true);
    mympd_state->tag_list_browse = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "tag_list_browse", mympd_state->tag_list_browse, vcb_istaglist, true);
    mympd_state->smartpls = state_file_rw_bool(workdir, DIR_WORK_STATE, "smartpls", mympd_state->smartpls, true);
    mympd_state->smartpls_sort = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "smartpls_sort", mympd_state->smartpls_sort, vcb_ismpdsort, true);
    mympd_state->smartpls_prefix = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "smartpls_prefix", mympd_state->smartpls_prefix, vcb_isname, true);
    mympd_state->smartpls_interval = state_file_rw_int(workdir, DIR_WORK_STATE, "smartpls_interval", mympd_state->smartpls_interval, TIMER_INTERVAL_MIN, TIMER_INTERVAL_MAX, true);
    mympd_state->smartpls_generate_tag_list = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "smartpls_generate_tag_list", mympd_state->smartpls_generate_tag_list, vcb_istaglist, true);
    mympd_state->cols_queue_current = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_queue_current", mympd_state->cols_queue_current, vcb_isname, true);
    mympd_state->cols_search = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_search", mympd_state->cols_search, vcb_isname, true);
    mympd_state->cols_browse_database_album_detail_info = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_database_album_detail_info", mympd_state->cols_browse_database_album_detail_info, vcb_isname, true);
    mympd_state->cols_browse_database_album_detail = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_database_album_detail", mympd_state->cols_browse_database_album_detail, vcb_isname, true);
    mympd_state->cols_browse_database_album_list = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_database_album_list", mympd_state->cols_browse_database_album_list, vcb_isname, true);
    mympd_state->cols_browse_playlist_detail = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_playlist_detail", mympd_state->cols_browse_playlist_detail, vcb_isname, true);
    mympd_state->cols_browse_filesystem = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_filesystem", mympd_state->cols_browse_filesystem, vcb_isname, true);
    mympd_state->cols_playback = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_playback", mympd_state->cols_playback, vcb_isname, true);
    mympd_state->cols_queue_last_played = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_queue_last_played", mympd_state->cols_queue_last_played, vcb_isname, true);
    mympd_state->cols_queue_jukebox_song = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_queue_jukebox_song", mympd_state->cols_queue_jukebox_song, vcb_isname, true);
    mympd_state->cols_queue_jukebox_album = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_queue_jukebox_album", mympd_state->cols_queue_jukebox_album, vcb_isname, true);
    mympd_state->cols_browse_radio_webradiodb = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_radio_webradiodb", mympd_state->cols_browse_radio_webradiodb, vcb_isname, true);
    mympd_state->cols_browse_radio_radiobrowser = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_radio_radiobrowser", mympd_state->cols_browse_radio_radiobrowser, vcb_isname, true);
    mympd_state->coverimage_names = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "coverimage_names", mympd_state->coverimage_names, vcb_isfilename, true);
    mympd_state->thumbnail_names = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "thumbnail_names", mympd_state->thumbnail_names, vcb_isfilename, true);
    mympd_state->music_directory = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "music_directory", mympd_state->music_directory, vcb_isfilepath, true);
    mympd_state->playlist_directory = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "playlist_directory", mympd_state->playlist_directory, vcb_isfilepath, true);
    mympd_state->volume_min = state_file_rw_uint(workdir, DIR_WORK_STATE, "volume_min", mympd_state->volume_min, VOLUME_MIN, VOLUME_MAX, true);
    mympd_state->volume_max = state_file_rw_uint(workdir, DIR_WORK_STATE, "volume_max", mympd_state->volume_max, VOLUME_MIN, VOLUME_MAX, true);
    mympd_state->volume_step = state_file_rw_uint(workdir, DIR_WORK_STATE, "volume_step", mympd_state->volume_step, VOLUME_STEP_MIN, VOLUME_STEP_MAX, true);
    mympd_state->webui_settings = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "webui_settings", mympd_state->webui_settings, validate_json_object, true);
    mympd_state->lyrics.uslt_ext = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "lyrics_uslt_ext", mympd_state->lyrics.uslt_ext, vcb_isalnum, true);
    mympd_state->lyrics.sylt_ext = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "lyrics_sylt_ext", mympd_state->lyrics.sylt_ext, vcb_isalnum, true);
    mympd_state->lyrics.vorbis_uslt = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "lyrics_vorbis_uslt", mympd_state->lyrics.vorbis_uslt, vcb_isalnum, true);
    mympd_state->lyrics.vorbis_sylt = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "lyrics_vorbis_sylt", mympd_state->lyrics.vorbis_sylt, vcb_isalnum, true);
    mympd_state->listenbrainz_token = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "listenbrainz_token", mympd_state->listenbrainz_token, vcb_isalnum, true);
    mympd_state->navbar_icons = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "navbar_icons", mympd_state->navbar_icons, validate_json_array, true);
    mympd_state->tag_disc_empty_is_first = state_file_rw_bool(workdir, DIR_WORK_STATE, "tag_disc_empty_is_first", mympd_state->tag_disc_empty_is_first, true);

    strip_slash(mympd_state->music_directory);
    strip_slash(mympd_state->playlist_directory);
}

/**
 * Reads the partition specific settings from the state files.
 * If the state file does not exist, it is populated with the default value and created.
 * @param partition_state pointer to the t_partition_state struct
 */
void mympd_api_settings_statefiles_partition_read(struct t_partition_state *partition_state) {
    sds workdir = partition_state->config->workdir;
    MYMPD_LOG_NOTICE(partition_state->name, "Reading partition states from directory \"%s/%s\"", workdir, partition_state->state_dir);
    partition_state->auto_play = state_file_rw_bool(workdir, partition_state->state_dir, "auto_play", partition_state->auto_play, true);
    partition_state->jukebox.mode = state_file_rw_uint(workdir, partition_state->state_dir, "jukebox_mode", partition_state->jukebox.mode, JUKEBOX_MODE_MIN, JUKEBOX_MODE_MAX, true);
    partition_state->jukebox.playlist = state_file_rw_string_sds(workdir, partition_state->state_dir, "jukebox_playlist", partition_state->jukebox.playlist, vcb_isfilename, true);
    partition_state->jukebox.queue_length = state_file_rw_uint(workdir, partition_state->state_dir, "jukebox_queue_length", partition_state->jukebox.queue_length, JUKEBOX_QUEUE_MIN, JUKEBOX_QUEUE_MAX, true);
    partition_state->jukebox.last_played = state_file_rw_uint(workdir, partition_state->state_dir, "jukebox_last_played", partition_state->jukebox.last_played, JUKEBOX_LAST_PLAYED_MIN, JUKEBOX_LAST_PLAYED_MAX, true);
    partition_state->jukebox.uniq_tag.tags[0] = state_file_rw_tag(workdir, partition_state->state_dir, "jukebox_uniq_tag", partition_state->jukebox.uniq_tag.tags[0], true);
    partition_state->jukebox.ignore_hated = state_file_rw_bool(workdir, partition_state->state_dir, "jukebox_ignore_hated", MYMPD_JUKEBOX_IGNORE_HATED, true);
    partition_state->jukebox.filter_include = state_file_rw_string_sds(workdir, partition_state->state_dir, "jukebox_filter_include", partition_state->jukebox.filter_include, vcb_issearchexpression, true);
    partition_state->jukebox.filter_exclude = state_file_rw_string_sds(workdir, partition_state->state_dir, "jukebox_filter_exclude", partition_state->jukebox.filter_exclude, vcb_issearchexpression, true);
    partition_state->jukebox.min_song_duration= state_file_rw_uint(workdir, partition_state->state_dir, "jukebox_min_song_duration", partition_state->jukebox.min_song_duration, 0, JUKEBOX_MIN_SONG_DURATION_MAX, true);
    partition_state->jukebox.max_song_duration= state_file_rw_uint(workdir, partition_state->state_dir, "jukebox_max_song_duration", partition_state->jukebox.max_song_duration, 0, JUKEBOX_MAX_SONG_DURATION_MAX, true);
    partition_state->highlight_color = state_file_rw_string_sds(workdir, partition_state->state_dir, "highlight_color", partition_state->highlight_color, vcb_ishexcolor, true);
    partition_state->highlight_color_contrast = state_file_rw_string_sds(workdir, partition_state->state_dir, "highlight_color_contrast", partition_state->highlight_color_contrast, vcb_ishexcolor, true);
    partition_state->mpd_stream_port = state_file_rw_uint(workdir, partition_state->state_dir, "mpd_stream_port", partition_state->mpd_stream_port, MPD_PORT_MIN, MPD_PORT_MAX, true);
    partition_state->stream_uri = state_file_rw_string_sds(workdir, partition_state->state_dir, "stream_uri", partition_state->stream_uri, vcb_isuri, true);
}

/**
 * Prints all settings
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds mympd_api_settings_get(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state, sds buffer, unsigned request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SETTINGS_GET;

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = tojson_char(buffer, "mympdVersion", MYMPD_VERSION, true);
    // mpd connection
    buffer = tojson_sds(buffer, "mpdHost", partition_state->mpd_state->mpd_host, true);
    buffer = tojson_uint(buffer, "mpdPort", partition_state->mpd_state->mpd_port, true);
    buffer = tojson_char(buffer, "mpdPass", "dontsetpassword", true);
    buffer = tojson_uint(buffer, "mpdTimeout", partition_state->mpd_state->mpd_timeout, true);
    buffer = tojson_bool(buffer, "mpdKeepalive", partition_state->mpd_state->mpd_keepalive, true);
    buffer = tojson_uint(buffer, "mpdBinarylimit", partition_state->mpd_state->mpd_binarylimit, true);
    // stickerdb connection
    buffer = tojson_sds(buffer, "stickerdbMpdHost", mympd_state->stickerdb->mpd_state->mpd_host, true);
    buffer = tojson_uint(buffer, "stickerdbMpdPort", mympd_state->stickerdb->mpd_state->mpd_port, true);
    buffer = tojson_char(buffer, "stickerdbMpdPass", "dontsetpassword", true);
    buffer = tojson_uint(buffer, "stickerdbMpdTimeout", mympd_state->stickerdb->mpd_state->mpd_timeout, true);
    buffer = tojson_bool(buffer, "stickerdbMpdKeepalive", mympd_state->stickerdb->mpd_state->mpd_keepalive, true);
    // other settings
    buffer = tojson_bool(buffer, "pin", (sdslen(mympd_state->config->pin_hash) == 0 ? false : true), true);
    #ifdef MYMPD_DEBUG
        buffer = tojson_bool(buffer, "debugMode", true, true);
    #else
        buffer = tojson_bool(buffer, "debugMode", false, true);
    #endif
    buffer = tojson_sds(buffer, "coverimageNames", mympd_state->coverimage_names, true);
    buffer = tojson_sds(buffer, "thumbnailNames", mympd_state->thumbnail_names, true);
    buffer = tojson_int(buffer, "loglevel", loglevel, true);
    buffer = tojson_bool(buffer, "smartpls", mympd_state->smartpls, true);
    buffer = tojson_sds(buffer, "smartplsSort", mympd_state->smartpls_sort, true);
    buffer = tojson_sds(buffer, "smartplsPrefix", mympd_state->smartpls_prefix, true);
    buffer = tojson_int(buffer, "smartplsInterval", mympd_state->smartpls_interval, true);
    buffer = tojson_uint(buffer, "lastPlayedCount", mympd_state->last_played_count, true);
    buffer = tojson_sds(buffer, "musicDirectory", mympd_state->music_directory, true);
    buffer = tojson_sds(buffer, "playlistDirectory", mympd_state->playlist_directory, true);
    buffer = tojson_sds(buffer, "bookletName", mympd_state->booklet_name, true);
    buffer = tojson_sds(buffer, "infoTxtName", mympd_state->info_txt_name, true);
    buffer = tojson_uint(buffer, "volumeMin", mympd_state->volume_min, true);
    buffer = tojson_uint(buffer, "volumeMax", mympd_state->volume_max, true);
    buffer = tojson_uint(buffer, "volumeStep", mympd_state->volume_step, true);
    buffer = tojson_sds(buffer, "lyricsUsltExt", mympd_state->lyrics.uslt_ext, true);
    buffer = tojson_sds(buffer, "lyricsSyltExt", mympd_state->lyrics.sylt_ext, true);
    buffer = tojson_sds(buffer, "lyricsVorbisUslt", mympd_state->lyrics.vorbis_uslt, true);
    buffer = tojson_sds(buffer, "lyricsVorbisSylt", mympd_state->lyrics.vorbis_sylt, true);
    buffer = tojson_raw(buffer, "colsQueueCurrent", mympd_state->cols_queue_current, true);
    buffer = tojson_raw(buffer, "colsSearch", mympd_state->cols_search, true);
    buffer = tojson_raw(buffer, "colsBrowseDatabaseAlbumDetailInfo", mympd_state->cols_browse_database_album_detail_info, true);
    buffer = tojson_raw(buffer, "colsBrowseDatabaseAlbumDetail", mympd_state->cols_browse_database_album_detail, true);
    buffer = tojson_raw(buffer, "colsBrowseDatabaseAlbumList", mympd_state->cols_browse_database_album_list, true);
    buffer = tojson_raw(buffer, "colsBrowsePlaylistDetail", mympd_state->cols_browse_playlist_detail, true);
    buffer = tojson_raw(buffer, "colsBrowseFilesystem", mympd_state->cols_browse_filesystem, true);
    buffer = tojson_raw(buffer, "colsPlayback", mympd_state->cols_playback, true);
    buffer = tojson_raw(buffer, "colsQueueLastPlayed", mympd_state->cols_queue_last_played, true);
    buffer = tojson_raw(buffer, "colsQueueJukeboxSong", mympd_state->cols_queue_jukebox_song, true);
    buffer = tojson_raw(buffer, "colsQueueJukeboxAlbum", mympd_state->cols_queue_jukebox_album, true);
    buffer = tojson_raw(buffer, "colsBrowseRadioWebradiodb", mympd_state->cols_browse_radio_webradiodb, true);
    buffer = tojson_raw(buffer, "colsBrowseRadioRadiobrowser", mympd_state->cols_browse_radio_radiobrowser, true);
    buffer = tojson_raw(buffer, "navbarIcons", mympd_state->navbar_icons, true);
    buffer = tojson_sds(buffer, "listenbrainzToken", mympd_state->listenbrainz_token, true);
    buffer = tojson_bool(buffer, "tagDiscEmptyIsFirst", mympd_state->tag_disc_empty_is_first, true);
    buffer = tojson_char(buffer, "albumMode", lookup_album_mode(mympd_state->config->albums.mode), true);
    buffer = tojson_char(buffer, "albumGroupTag", mpd_tag_name(mympd_state->config->albums.group_tag), true);
    buffer = tojson_raw(buffer, "webuiSettings", mympd_state->webui_settings, true);
    //partition specific settings
    buffer = sdscat(buffer, "\"partition\":{");
    const char *jukebox_mode_str = jukebox_mode_lookup(partition_state->jukebox.mode);
    buffer = tojson_char(buffer, "jukeboxMode", jukebox_mode_str, true);
    buffer = tojson_sds(buffer, "jukeboxPlaylist", partition_state->jukebox.playlist, true);
    buffer = tojson_uint(buffer, "jukeboxQueueLength", partition_state->jukebox.queue_length, true);
    buffer = tojson_char(buffer, "jukeboxUniqTag", mpd_tag_name(partition_state->jukebox.uniq_tag.tags[0]), true);
    buffer = tojson_uint(buffer, "jukeboxLastPlayed", partition_state->jukebox.last_played, true);
    buffer = tojson_bool(buffer, "jukeboxIgnoreHated", partition_state->jukebox.ignore_hated, true);
    buffer = tojson_char(buffer, "jukeboxFilterInclude", partition_state->jukebox.filter_include, true);
    buffer = tojson_char(buffer, "jukeboxFilterExclude", partition_state->jukebox.filter_exclude, true);
    buffer = tojson_uint(buffer, "jukeboxMinSongDuration", partition_state->jukebox.min_song_duration, true);
    buffer = tojson_uint(buffer, "jukeboxMaxSongDuration", partition_state->jukebox.max_song_duration, true);
    buffer = tojson_bool(buffer, "autoPlay", partition_state->auto_play, true);
    buffer = tojson_char(buffer, "highlightColor", partition_state->highlight_color, true);
    buffer = tojson_char(buffer, "highlightColorContrast", partition_state->highlight_color_contrast, true);
    buffer = tojson_uint(buffer, "mpdStreamPort", partition_state->mpd_stream_port, true);
    buffer = tojson_char(buffer, "streamUri", partition_state->stream_uri, true);
    buffer = sdscat(buffer, "\"presets\": [");
    buffer = presets_list(&partition_state->preset_list, buffer);
    buffer = sdscatlen(buffer, "],", 2);
    if (partition_state->conn_state == MPD_CONNECTED) {
        //mpd options
        buffer = tojson_bool(buffer, "mpdConnected", true, true);
        if (mpd_command_list_begin(partition_state->conn, true)) {
            if (mpd_send_status(partition_state->conn) == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_status");
            }
            if (mpd_send_replay_gain_status(partition_state->conn) == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_replay_gain_status");
            }
            mpd_client_command_list_end_check(partition_state);
        }
        struct mpd_status *status = mpd_recv_status(partition_state->conn);
        enum mpd_replay_gain_mode replay_gain_mode = MPD_REPLAY_UNKNOWN;
        if (mpd_response_next(partition_state->conn)) {
            replay_gain_mode = mpd_recv_replay_gain_status(partition_state->conn);
        }
        if (status != NULL) {
            enum mpd_single_state single_state = mpd_status_get_single_state(status);
            buffer = tojson_char(buffer, "single", mpd_lookup_single_state(single_state), true);
            buffer = tojson_uint(buffer, "crossfade", mpd_status_get_crossfade(status), true);
            buffer = tojson_float(buffer, "mixrampDb", mpd_status_get_mixrampdb(status), true);
            buffer = tojson_float(buffer, "mixrampDelay", mpd_status_get_mixrampdelay(status), true);
            buffer = tojson_bool(buffer, "repeat", mpd_status_get_repeat(status), true);
            buffer = tojson_bool(buffer, "random", mpd_status_get_random(status), true);
            enum mpd_consume_state consume_state = mpd_status_get_consume_state(status);
            buffer = tojson_char(buffer, "consume", mpd_lookup_consume_state(consume_state), true);
            buffer = tojson_char(buffer, "replaygain", mpd_lookup_replay_gain_mode(replay_gain_mode), false);
            mpd_status_free(status);
        }
        mpd_response_finish(partition_state->conn);
        if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_run_status") == false) {
            return buffer;
        }
    }
    else {
        //not connected to mpd
        buffer = tojson_bool(buffer, "mpdConnected", false, false);
    }
    //features
    buffer = sdscat(buffer, "},\"features\":{");
    if (partition_state->conn_state == MPD_CONNECTED) {
        buffer = tojson_bool(buffer, "featPlaylists", partition_state->mpd_state->feat.playlists, true);
        buffer = tojson_bool(buffer, "featTags", partition_state->mpd_state->feat.tags, true);
        buffer = tojson_bool(buffer, "featLibrary", partition_state->mpd_state->feat.library, true);
        buffer = tojson_bool(buffer, "featStickers", partition_state->mpd_state->feat.stickers, true);
        buffer = tojson_bool(buffer, "featStickersEnabled", partition_state->config->stickers, true);
        buffer = tojson_bool(buffer, "featFingerprint", partition_state->mpd_state->feat.fingerprint, true);
        buffer = tojson_bool(buffer, "featPartitions", partition_state->mpd_state->feat.partitions, true);
        buffer = tojson_bool(buffer, "featMounts", partition_state->mpd_state->feat.mount, true);
        buffer = tojson_bool(buffer, "featNeighbors", partition_state->mpd_state->feat.neighbor, true);
        buffer = tojson_bool(buffer, "featBinarylimit", partition_state->mpd_state->feat.binarylimit, true);
        buffer = tojson_bool(buffer, "featPlaylistRmRange", partition_state->mpd_state->feat.playlist_rm_range, true);
        buffer = tojson_bool(buffer, "featWhence", partition_state->mpd_state->feat.whence, true);
        buffer = tojson_bool(buffer, "featAdvqueue", partition_state->mpd_state->feat.advqueue, true);
        buffer = tojson_bool(buffer, "featConsumeOneshot", partition_state->mpd_state->feat.consume_oneshot, true);
        buffer = tojson_bool(buffer, "featPlaylistDirAuto", partition_state->mpd_state->feat.playlist_dir_auto, true);
        buffer = tojson_bool(buffer, "featStartsWith", partition_state->mpd_state->feat.starts_with, true);
        buffer = tojson_bool(buffer, "featPcre", partition_state->mpd_state->feat.pcre, true);
        buffer = tojson_bool(buffer, "featDbAdded", partition_state->mpd_state->feat.db_added, true);
        buffer = tojson_bool(buffer, "featStickerSortWindow", partition_state->mpd_state->feat.sticker_sort_window, true);
        buffer = tojson_bool(buffer, "featStickerInt", partition_state->mpd_state->feat.sticker_int, true);
    }
    buffer = tojson_bool(buffer, "featCacert", (mympd_state->config->custom_cert == false && mympd_state->config->ssl == true ? true : false), true);
    #ifdef MYMPD_ENABLE_LUA
        buffer = tojson_bool(buffer, "featScripting", true, true);
    #else
        buffer = tojson_bool(buffer, "featScripting", false, true);
    #endif
    buffer = sdscatlen(buffer, "}", 1);
    if (partition_state->conn_state == MPD_CONNECTED) {
        buffer = sdscatlen(buffer, ",", 1);
        buffer = tojson_sds(buffer, "musicDirectoryValue", partition_state->mpd_state->music_directory_value, true);
        buffer = tojson_sds(buffer, "playlistDirectoryValue", partition_state->mpd_state->playlist_directory_value, true);
        //taglists
        buffer = print_tags_array(buffer, "tagList", &partition_state->mpd_state->tags_mympd);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = print_tags_array(buffer, "tagListSearch", &partition_state->mpd_state->tags_search);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = print_tags_array(buffer, "tagListBrowse", &partition_state->mpd_state->tags_browse);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = print_tags_array(buffer, "tagListMpd", &partition_state->mpd_state->tags_mpd);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = print_tags_array(buffer, "tagListAlbum", &partition_state->mpd_state->tags_album);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = print_tags_array(buffer, "smartplsGenerateTagList", &mympd_state->smartpls_generate_tag_types);
        //trigger events
        buffer = sdscat(buffer, ",\"triggerEvents\":{");
        buffer = mympd_api_trigger_print_event_list(buffer);
        buffer = sdscatlen(buffer, "}", 1);
    }
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Privat functions
 */

/**
 * Helper function to set an error message
 * @param error pointer to t_jsonrpc_parse_error
 * @param path jsonrpc path
 * @param key setting key
 * @param value setting value
 * @param message the message to print, leave empty for generic invalid message
 */
static void set_invalid_value(struct t_jsonrpc_parse_error *error, const char *path, sds key, sds value, const char *message) {
    error->message = message[0] == '\0'
        ? sdscatfmt(sdsempty(), "Invalid value for \"%S\"", key)
        : sdsnew(message);
    error->path = sdscatfmt(sdsempty(), "%s.%S", path, key);
    MYMPD_LOG_WARN(NULL, "Invalid value for \"%s\": \"%s\"", key, value);
}

/**
 * Helper function to set an error message
 * @param error pointer to t_jsonrpc_parse_error
 * @param path jsonrpc path
 * @param key setting key
 */
static void set_invalid_field(struct t_jsonrpc_parse_error *error, const char *path, sds key) {
    error->message = sdscatfmt(sdsempty(), "Invalid field: \"%s\"", key);
    error->path = sdscatfmt(sdsempty(), "%s.%S", path, key);
    MYMPD_LOG_WARN(NULL, "%s", error->message);
}

/**
 * Enables the set_conn_options flag for all partitions
 * @param mympd_state pointer to central myMPD state
 */
static void enable_set_conn_options(struct t_mympd_state *mympd_state) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        partition_state->set_conn_options = true;
        partition_state = partition_state->next;
    }
}
