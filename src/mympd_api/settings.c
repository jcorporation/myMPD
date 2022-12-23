/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/settings.h"

#include "dist/mjson/mjson.h"
#include "src/lib/api.h"
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
#include "src/mpd_client/tags.h"
#include "src/mympd_api/timer.h"
#include "src/mympd_api/timer_handlers.h"
#include "src/mympd_api/trigger.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/**
 * Private declarations
 */

static sds set_invalid_value(sds buffer, sds key, sds value);
static void enable_set_conn_options(struct t_mympd_state *mympd_state);

/**
 * Pushes needed settings to the webserver thread
 * @param mympd_state pointer to central myMPD state
 * @return true on success, else false
 */
bool settings_to_webserver(struct t_mympd_state *mympd_state) {
    //push settings to web_server_queue
    struct set_mg_user_data_request *extra = malloc_assert(sizeof(struct set_mg_user_data_request));
    extra->music_directory = sdsdup(mympd_state->mpd_state->music_directory_value);
    extra->playlist_directory = sdsdup(mympd_state->mpd_state->playlist_directory_value);
    extra->coverimage_names = sdsdup(mympd_state->coverimage_names);
    extra->thumbnail_names = sdsdup(mympd_state->thumbnail_names);
    extra->feat_albumart = mympd_state->mpd_state->feat_albumart;
    extra->mpd_host = sdsdup(mympd_state->mpd_state->mpd_host);
    list_init(&extra->partitions);
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        if (sdslen(partition_state->stream_uri) == 0) {
            //ignore custom stream uris
            list_push(&extra->partitions, partition_state->name, (long long)partition_state->mpd_stream_port, NULL, NULL);
        }
        partition_state = partition_state->next;
    }

    struct t_work_response *web_server_response = create_response_new(-1, 0, INTERNAL_API_WEBSERVER_SETTINGS, MPD_PARTITION_DEFAULT);
    web_server_response->extra = extra;
    return mympd_queue_push(web_server_queue, web_server_response, 0);
}

/**
 * Saves connection specific settings
 * @param key setting key
 * @param value setting value
 * @param vtype value type
 * @param vcb validation callback
 * @param userdata pointer to the t_mympd_state struct
 * @param error pointer to a sds string to populate the error message
 * @return true on success, else false
 */
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
    else if (strcmp(key, "musicDirectory") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilepath(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->music_directory = sds_replace(mympd_state->music_directory, value);
        strip_slash(mympd_state->music_directory);
    }
    else if (strcmp(key, "playlistDirectory") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilepath(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->playlist_directory = sds_replace(mympd_state->playlist_directory, value);
        strip_slash(mympd_state->playlist_directory);
    }
    else if (strcmp(key, "mpdBinarylimit") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned binarylimit = (unsigned)strtoumax(value, NULL, 10);
        if (binarylimit < MPD_BINARY_SIZE_MIN || binarylimit > MPD_BINARY_SIZE_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (binarylimit != mympd_state->mpd_state->mpd_binarylimit) {
            mympd_state->mpd_state->mpd_binarylimit = binarylimit;
            enable_set_conn_options(mympd_state);
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
            enable_set_conn_options(mympd_state);
        }
    }
    else if (strcmp(key, "mpdKeepalive") == 0) {
        if (vtype != MJSON_TOK_TRUE && vtype != MJSON_TOK_FALSE) {
            *error = sdscatfmt(*error, "Invalid value for \"%S\": \"%S\"", key, value);
            MYMPD_LOG_WARN("%s", *error);
            return false;
        }
        bool keepalive = vtype == MJSON_TOK_TRUE ? true : false;
        if (keepalive != mympd_state->mpd_state->mpd_keepalive) {
            mympd_state->mpd_state->mpd_keepalive = keepalive;
            enable_set_conn_options(mympd_state);
        }
    }
    else {
        sdsclear(*error);
        *error = sdscatfmt(*error, "Unknown setting \"%S\": \"%S\"", key, value);
        MYMPD_LOG_WARN("%s", *error);
        return true;
    }

    bool rc = true;
    if (check_for_mpd_error == true) {
        rc = mympd_check_error_and_recover_plain(mympd_state->partition_state, error);
    }

    if (rc == true) {
        sds state_filename = camel_to_snake(key);
        rc = state_file_write(mympd_state->config->workdir, DIR_WORK_STATE, state_filename, value);
        FREE_SDS(state_filename);
    }
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
    bool rc = state_file_write(mympd_state->config->workdir, DIR_WORK_STATE, tablename, cols);
    FREE_SDS(tablename);
    return rc;
}

/**
 * Saves settings
 * @param key setting key
 * @param value setting value
 * @param vtype value type
 * @param vcb validation callback
 * @param userdata pointer to central myMPD state
 * @param error pointer to a sds string to populate the error message
 * @return true on success, else false
 */
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
            mympd_state->mpd_state->booklet_name = sds_replace(mympd_state->mpd_state->booklet_name, value);
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
        mympd_state->mpd_state->last_played_count = last_played_count;
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
        //set enabled tags for all connections
        enable_set_conn_options(mympd_state);
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
        if (sdslen(value) > 0 &&
            vcb_isfilename(value) == false)
        {
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
            mympd_api_timer_replace(&mympd_state->timer_list, interval, (int)interval, timer_handler_by_id, TIMER_ID_SMARTPLS_UPDATE, NULL);
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
        mympd_state->lyrics.uslt_ext = sds_replacelen(mympd_state->lyrics.uslt_ext, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsSyltExt") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->lyrics.sylt_ext = sds_replacelen(mympd_state->lyrics.sylt_ext, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsVorbisUslt") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->lyrics.vorbis_uslt = sds_replacelen(mympd_state->lyrics.vorbis_uslt, value, sdslen(value));
    }
    else if (strcmp(key, "lyricsVorbisSylt") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->lyrics.vorbis_sylt = sds_replacelen(mympd_state->lyrics.vorbis_sylt, value, sdslen(value));
    }
    else if (strcmp(key, "listenbrainzToken") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isalnum(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        mympd_state->listenbrainz_token = sds_replacelen(mympd_state->listenbrainz_token, value, sdslen(value));
    }
    else {
        sdsclear(*error);
        *error = sdscatfmt(*error, "Unknown setting \"%S\": \"%S\"", key, value);
        MYMPD_LOG_WARN("%s", *error);
        return true;
    }
    sds state_filename = camel_to_snake(key);
    bool rc = state_file_write(mympd_state->config->workdir, DIR_WORK_STATE, state_filename, value);
    FREE_SDS(state_filename);
    return rc;
}

/**
 * Saves partition settings
 * @param key setting key
 * @param value setting value
 * @param vtype value type
 * @param vcb validation callback
 * @param userdata pointer to partition state
 * @param error pointer to a sds string to populate the error message
 * @return true on success, else false
 */
bool mympd_api_settings_partition_set(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error) {
    (void) vcb;
    struct t_partition_state *partition_state = (struct t_partition_state *)userdata;

    MYMPD_LOG_DEBUG("Parse setting \"%s\": \"%s\" (%s)", key, value, get_mjson_toktype_name(vtype));

    if (strcmp(key, "highlightColor") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_ishexcolor(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        partition_state->highlight_color = sds_replace(partition_state->highlight_color, value);
    }
    else if (strcmp(key, "mpdStreamPort") == 0 && vtype == MJSON_TOK_NUMBER) {
        unsigned mpd_stream_port = (unsigned)strtoumax(value, NULL, 10);
        if (mpd_stream_port > MPD_PORT_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        partition_state->mpd_stream_port = mpd_stream_port;
    }
    else if (strcmp(key, "streamUri") == 0 && vtype == MJSON_TOK_STRING) {
        if (sdslen(value) > 0 &&
            vcb_isuri(value) == false)
        {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        partition_state->stream_uri = sds_replace(partition_state->stream_uri, value);
    }
    else {
        sdsclear(*error);
        *error = sdscatfmt(*error, "Unknown setting \"%S\": \"%S\"", key, value);
        MYMPD_LOG_WARN("%s", *error);
        return true;
    }
    sds state_filename = camel_to_snake(key);
    bool rc = state_file_write(partition_state->mympd_state->config->workdir, partition_state->state_dir, state_filename, value);
    FREE_SDS(state_filename);
    return rc;
}

/**
 * Sets mpd options and jukebox settings
 * @param key setting key
 * @param value setting value
 * @param vtype value type
 * @param vcb validation callback
 * @param userdata pointer to the t_partition_state struct
 * @param error pointer to a sds string to populate the error message
 * @return true on success, else false
 */
bool mympd_api_settings_mpd_options_set(sds key, sds value, int vtype, validate_callback vcb, void *userdata, sds *error) {
    (void) vcb;
    struct t_partition_state *partition_state = (struct t_partition_state *)userdata;

    bool rc = false;
    bool write_state_file = true;
    bool jukebox_changed = false;

    MYMPD_LOG_DEBUG("\"%s\": Parse setting \"%s\": \"%s\" (%s)", partition_state->name, key, value, get_mjson_toktype_name(vtype));
    if (strcmp(key, "autoPlay") == 0) {
        if (vtype == MJSON_TOK_TRUE) {
            partition_state->auto_play = true;
        }
        else if (vtype == MJSON_TOK_FALSE) {
            partition_state->auto_play = false;
        }
        else {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
    }
    else if (strcmp(key, "jukeboxMode") == 0 && vtype == MJSON_TOK_STRING) {
        enum jukebox_modes jukebox_mode = jukebox_mode_parse(value);

        if (jukebox_mode == JUKEBOX_UNKNOWN) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (partition_state->jukebox_mode != jukebox_mode) {
            partition_state->jukebox_mode = jukebox_mode;
            jukebox_changed = true;
        }
        sdsclear(value);
        value = sdscatfmt(value, "%i", jukebox_mode);
    }
    else if (strcmp(key, "jukeboxPlaylist") == 0 && vtype == MJSON_TOK_STRING) {
        if (vcb_isfilename(value) == false) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (strcmp(partition_state->jukebox_playlist, value) != 0) {
            partition_state->jukebox_playlist = sds_replace(partition_state->jukebox_playlist, value);
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxQueueLength") == 0 && vtype == MJSON_TOK_NUMBER) {
        long jukebox_queue_length = (long)strtoimax(value, NULL, 10);
        if (jukebox_queue_length <= JUKEBOX_QUEUE_MIN || jukebox_queue_length > JUKEBOX_QUEUE_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        partition_state->jukebox_queue_length = jukebox_queue_length;
    }
    else if (strcmp(key, "jukeboxUniqueTag") == 0 && vtype == MJSON_TOK_STRING) {
        enum mpd_tag_type unique_tag = mpd_tag_name_parse(value);
        if (unique_tag == MPD_TAG_UNKNOWN) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (partition_state->jukebox_unique_tag.tags[0] != unique_tag) {
            partition_state->jukebox_unique_tag.tags[0] = unique_tag;
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxLastPlayed") == 0 && vtype == MJSON_TOK_NUMBER) {
        long jukebox_last_played = (long)strtoimax(value, NULL, 10);
        if (jukebox_last_played < 0 || jukebox_last_played > JUKEBOX_LAST_PLAYED_MAX) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        if (jukebox_last_played != partition_state->jukebox_last_played) {
            partition_state->jukebox_last_played = jukebox_last_played;
            jukebox_changed = true;
        }
    }
    else if (strcmp(key, "jukeboxIgnoreHated") == 0) {
        if (vtype != MJSON_TOK_TRUE && vtype != MJSON_TOK_FALSE) {
            *error = set_invalid_value(*error, key, value);
            return false;
        }
        bool bool_buf = vtype == MJSON_TOK_TRUE ? true : false;
        if (bool_buf != partition_state->jukebox_ignore_hated) {
            partition_state->jukebox_ignore_hated = bool_buf;
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
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            bool bool_buf = vtype == MJSON_TOK_TRUE ? true : false;
            rc = mpd_run_random(partition_state->conn, bool_buf);
        }
        else if (strcmp(key, "repeat") == 0) {
            if (vtype != MJSON_TOK_TRUE && vtype != MJSON_TOK_FALSE) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            bool bool_buf = vtype == MJSON_TOK_TRUE ? true : false;
            rc = mpd_run_repeat(partition_state->conn, bool_buf);
        }
        else if (strcmp(key, "consume") == 0) {
            enum mpd_consume_state state = mpd_parse_consume_state(value);
            if (state == MPD_CONSUME_UNKNOWN) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            rc = mpd_run_consume_state(partition_state->conn, state);
        }
        else if (strcmp(key, "single") == 0) {
            enum mpd_single_state state = mpd_parse_single_state(value);
            if (state == MPD_SINGLE_UNKNOWN) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            rc = mpd_run_single_state(partition_state->conn, state);
        }
        else if (strcmp(key, "crossfade") == 0 && vtype == MJSON_TOK_NUMBER) {
            unsigned uint_buf = (unsigned)strtoumax(value, NULL, 10);
            if (uint_buf > MPD_CROSSFADE_MAX) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            rc = mpd_run_crossfade(partition_state->conn, uint_buf);
        }
        else if (strcmp(key, "replaygain") == 0 && vtype == MJSON_TOK_STRING) {
            enum mpd_replay_gain_mode mode = mpd_parse_replay_gain_name(value);
            if (mode == MPD_REPLAY_UNKNOWN) {
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            rc = mpd_run_replay_gain_mode(partition_state->conn, mode);
        }
        else if (strcmp(key, "mixrampDb") == 0 && vtype == MJSON_TOK_NUMBER) {
            float db = strtof(value, NULL);
            if (db < -100 || db > 0) {
                //mixrampdb should be a negative value
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            rc = mpd_run_mixrampdb(partition_state->conn, db);
        }
        else if (strcmp(key, "mixrampDelay") == 0 && vtype == MJSON_TOK_NUMBER) {
            float delay = strtof(value, NULL);
            if (delay < -1.0 || delay > 100) {
                //mixrampdb should be a positive value
                //0 disables mixramp
                *error = set_invalid_value(*error, key, value);
                return false;
            }
            rc = mpd_run_mixrampdelay(partition_state->conn, delay);
        }
        sds message = sdsempty();
        rc = mympd_check_rc_error_and_recover_notify(partition_state, &message, rc, key);
        if (rc == false) {
            ws_notify(message, partition_state->name);
        }
        FREE_SDS(message);
        write_state_file = false;
    }
    else {
        MYMPD_LOG_WARN("\"%s\": Unknown setting \"%s\": \"%s\"", partition_state->name, key, value);
        return false;
    }
    if (jukebox_changed == true && partition_state->jukebox_queue.length > 0) {
        MYMPD_LOG_INFO("\"%s\": Jukebox options changed, clearing jukebox queue", partition_state->name);
        jukebox_clear(&partition_state->jukebox_queue);
    }
    if (write_state_file == true) {
        sds state_filename = camel_to_snake(key);
        rc = state_file_write(partition_state->mympd_state->config->workdir, partition_state->state_dir, state_filename, value);
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
    MYMPD_LOG_NOTICE("Reading global states");
    sds workdir = mympd_state->config->workdir;
    mympd_state->mpd_state->mpd_host = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "mpd_host", mympd_state->mpd_state->mpd_host, vcb_isname, true);
    mympd_state->mpd_state->mpd_port = state_file_rw_uint(workdir, DIR_WORK_STATE, "mpd_port", mympd_state->mpd_state->mpd_port, MPD_PORT_MIN, MPD_PORT_MAX, true);
    mympd_state->mpd_state->mpd_pass = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "mpd_pass", mympd_state->mpd_state->mpd_pass, vcb_isname, true);
    mympd_state->mpd_state->mpd_binarylimit = state_file_rw_uint(workdir, DIR_WORK_STATE, "mpd_binarylimit", mympd_state->mpd_state->mpd_binarylimit, MPD_BINARY_SIZE_MIN, MPD_BINARY_SIZE_MAX, true);
    mympd_state->mpd_state->mpd_timeout = state_file_rw_uint(workdir, DIR_WORK_STATE, "mpd_timeout", mympd_state->mpd_state->mpd_timeout, MPD_TIMEOUT_MIN, MPD_TIMEOUT_MAX, true);
    mympd_state->mpd_state->mpd_keepalive = state_file_rw_bool(workdir, DIR_WORK_STATE, "mpd_keepalive", mympd_state->mpd_state->mpd_keepalive, true);
    mympd_state->mpd_state->tag_list = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "tag_list", mympd_state->mpd_state->tag_list, vcb_istaglist, true);
    mympd_state->tag_list_search = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "tag_list_search", mympd_state->tag_list_search, vcb_istaglist, true);
    mympd_state->tag_list_browse = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "tag_list_browse", mympd_state->tag_list_browse, vcb_istaglist, true);
    mympd_state->smartpls = state_file_rw_bool(workdir, DIR_WORK_STATE, "smartpls", mympd_state->smartpls, true);
    mympd_state->smartpls_sort = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "smartpls_sort", mympd_state->smartpls_sort, vcb_ismpdsort, true);
    mympd_state->smartpls_prefix = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "smartpls_prefix", mympd_state->smartpls_prefix, vcb_isname, true);
    mympd_state->smartpls_interval = state_file_rw_int(workdir, DIR_WORK_STATE, "smartpls_interval", (int)mympd_state->smartpls_interval, TIMER_INTERVAL_MIN, TIMER_INTERVAL_MAX, true);
    mympd_state->smartpls_generate_tag_list = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "smartpls_generate_tag_list", mympd_state->smartpls_generate_tag_list, vcb_istaglist, true);
    mympd_state->mpd_state->last_played_count = state_file_rw_long(workdir, DIR_WORK_STATE, "last_played_count", mympd_state->mpd_state->last_played_count, 0, MPD_PLAYLIST_LENGTH_MAX, true);
    mympd_state->cols_queue_current = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_queue_current", mympd_state->cols_queue_current, vcb_isname, true);
    mympd_state->cols_search = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_search", mympd_state->cols_search, vcb_isname, true);
    mympd_state->cols_browse_database_album_detail_info = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_database_album_detail_info", mympd_state->cols_browse_database_album_detail_info, vcb_isname, true);
    mympd_state->cols_browse_database_album_detail = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_database_album_detail", mympd_state->cols_browse_database_album_detail, vcb_isname, true);
    mympd_state->cols_browse_database_album_list = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_database_album_list", mympd_state->cols_browse_database_album_list, vcb_isname, true);
    mympd_state->cols_browse_playlist_detail = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_playlist_detail", mympd_state->cols_browse_playlist_detail, vcb_isname, true);
    mympd_state->cols_browse_filesystem = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_filesystem", mympd_state->cols_browse_filesystem, vcb_isname, true);
    mympd_state->cols_playback = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_playback", mympd_state->cols_playback, vcb_isname, true);
    mympd_state->cols_queue_last_played = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_queue_last_played", mympd_state->cols_queue_last_played, vcb_isname, true);
    mympd_state->cols_queue_jukebox = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_queue_jukebox", mympd_state->cols_queue_jukebox, vcb_isname, true);
    mympd_state->cols_browse_radio_webradiodb = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_radio_webradiodb", mympd_state->cols_browse_radio_webradiodb, vcb_isname, true);
    mympd_state->cols_browse_radio_radiobrowser = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "cols_browse_radio_radiobrowser", mympd_state->cols_browse_radio_radiobrowser, vcb_isname, true);
    mympd_state->coverimage_names = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "coverimage_names", mympd_state->coverimage_names, vcb_isfilename, true);
    mympd_state->thumbnail_names = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "thumbnail_names", mympd_state->thumbnail_names, vcb_isfilename, true);
    mympd_state->music_directory = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "music_directory", mympd_state->music_directory, vcb_isfilepath, true);
    mympd_state->playlist_directory = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "playlist_directory", mympd_state->playlist_directory, vcb_isfilepath, true);
    mympd_state->mpd_state->booklet_name = state_file_rw_string_sds(workdir, DIR_WORK_STATE, "booklet_name", mympd_state->mpd_state->booklet_name, vcb_isfilename, true);
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

    strip_slash(mympd_state->music_directory);
    strip_slash(mympd_state->playlist_directory);
}

/**
 * Reads the partition specific settings from the state files.
 * If the state file does not exist, it is populated with the default value and created.
 * @param partition_state pointer to the t_partition_state struct
 */
void mympd_api_settings_statefiles_partition_read(struct t_partition_state *partition_state) {
    sds workdir = partition_state->mympd_state->config->workdir;
    MYMPD_LOG_NOTICE("\"%s\": Reading partition states from directory \"%s/%s\"", partition_state->name, workdir, partition_state->state_dir);
    partition_state->auto_play = state_file_rw_bool(workdir, partition_state->state_dir, "auto_play", partition_state->auto_play, true);
    partition_state->jukebox_mode = state_file_rw_uint(workdir, partition_state->state_dir, "jukebox_mode", partition_state->jukebox_mode, JUKEBOX_MODE_MIN, JUKEBOX_MODE_MAX, true);
    partition_state->jukebox_playlist = state_file_rw_string_sds(workdir, partition_state->state_dir, "jukebox_playlist", partition_state->jukebox_playlist, vcb_isfilename, true);
    partition_state->jukebox_queue_length = state_file_rw_long(workdir, partition_state->state_dir, "jukebox_queue_length", partition_state->jukebox_queue_length, JUKEBOX_QUEUE_MIN, JUKEBOX_QUEUE_MAX, true);
    partition_state->jukebox_last_played = state_file_rw_long(workdir, partition_state->state_dir, "jukebox_last_played", partition_state->jukebox_last_played, JUKEBOX_LAST_PLAYED_MIN, JUKEBOX_LAST_PLAYED_MAX, true);
    partition_state->jukebox_unique_tag.tags[0] = state_file_rw_int(workdir, partition_state->state_dir, "jukebox_unique_tag", partition_state->jukebox_unique_tag.tags[0], 0, MPD_TAG_COUNT, true);
    partition_state->jukebox_ignore_hated = state_file_rw_bool(workdir, partition_state->state_dir, "jukebox_ignore_hated", MYMPD_JUKEBOX_IGNORE_HATED, true);
    partition_state->highlight_color = state_file_rw_string_sds(workdir, partition_state->state_dir, "highlight_color", partition_state->highlight_color, vcb_ishexcolor, true);
    partition_state->mpd_stream_port = state_file_rw_uint(workdir, partition_state->state_dir, "mpd_stream_port", partition_state->mpd_stream_port, MPD_PORT_MIN, MPD_PORT_MAX, true);
    partition_state->stream_uri = state_file_rw_string_sds(workdir, partition_state->state_dir, "stream_uri", partition_state->stream_uri, vcb_isuri, true);
}

/**
 * Prints all settings
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds mympd_api_settings_get(struct t_partition_state *partition_state, sds buffer, long request_id) {
    //shortcuts
    struct t_mympd_state *mympd_state = partition_state->mympd_state;

    enum mympd_cmd_ids cmd_id = MYMPD_API_SETTINGS_GET;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = tojson_char(buffer, "mympdVersion", MYMPD_VERSION, true);
    buffer = tojson_sds(buffer, "mpdHost", partition_state->mpd_state->mpd_host, true);
    buffer = tojson_uint(buffer, "mpdPort", partition_state->mpd_state->mpd_port, true);
    buffer = tojson_char(buffer, "mpdPass", "dontsetpassword", true);
    buffer = tojson_uint(buffer, "mpdTimeout", partition_state->mpd_state->mpd_timeout, true);
    buffer = tojson_bool(buffer, "mpdKeepalive", partition_state->mpd_state->mpd_keepalive, true);
    buffer = tojson_uint(buffer, "mpdBinarylimit", partition_state->mpd_state->mpd_binarylimit, true);
#ifdef MYMPD_ENABLE_SSL
    buffer = tojson_bool(buffer, "pin", (sdslen(mympd_state->config->pin_hash) == 0 ? false : true), true);
#else
    buffer = tojson_bool(buffer, "pin", false, true);
#endif
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
    buffer = tojson_time(buffer, "smartplsInterval", mympd_state->smartpls_interval, true);
    buffer = tojson_long(buffer, "lastPlayedCount", mympd_state->mpd_state->last_played_count, true);
    buffer = tojson_sds(buffer, "musicDirectory", mympd_state->music_directory, true);
    buffer = tojson_sds(buffer, "playlistDirectory", mympd_state->playlist_directory, true);
    buffer = tojson_sds(buffer, "bookletName", mympd_state->mpd_state->booklet_name, true);
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
    buffer = tojson_raw(buffer, "colsQueueJukebox", mympd_state->cols_queue_jukebox, true);
    buffer = tojson_raw(buffer, "colsBrowseRadioWebradiodb", mympd_state->cols_browse_radio_webradiodb, true);
    buffer = tojson_raw(buffer, "colsBrowseRadioRadiobrowser", mympd_state->cols_browse_radio_radiobrowser, true);
    buffer = tojson_raw(buffer, "navbarIcons", mympd_state->navbar_icons, true);
    buffer = tojson_sds(buffer, "listenbrainzToken", mympd_state->listenbrainz_token, true);
    buffer = tojson_raw(buffer, "webuiSettings", mympd_state->webui_settings, true);
    //partition specific settings
    buffer = sdscat(buffer, "\"partition\":{");
    const char *jukebox_mode_str = jukebox_mode_lookup(partition_state->jukebox_mode);
    buffer = tojson_char(buffer, "jukeboxMode", jukebox_mode_str, true);
    buffer = tojson_sds(buffer, "jukeboxPlaylist", partition_state->jukebox_playlist, true);
    buffer = tojson_long(buffer, "jukeboxQueueLength", partition_state->jukebox_queue_length, true);
    buffer = tojson_char(buffer, "jukeboxUniqueTag", mpd_tag_name(partition_state->jukebox_unique_tag.tags[0]), true);
    buffer = tojson_long(buffer, "jukeboxLastPlayed", partition_state->jukebox_last_played, true);
    buffer = tojson_bool(buffer, "jukeboxIgnoreHated", partition_state->jukebox_ignore_hated, true);
    buffer = tojson_bool(buffer, "autoPlay", partition_state->auto_play, true);
    buffer = tojson_char(buffer, "highlightColor", partition_state->highlight_color, true);
    buffer = tojson_uint(buffer, "mpdStreamPort", partition_state->mpd_stream_port, true);
    buffer = tojson_char(buffer, "streamUri", partition_state->stream_uri, true);
    buffer = sdscat(buffer, "\"presets\": [");
    buffer = presets_list(&partition_state->presets, buffer);
    buffer = sdscatlen(buffer, "],", 2);
    if (partition_state->conn_state == MPD_CONNECTED) {
        //mpd options
        buffer = tojson_bool(buffer, "mpdConnected", true, true);
        struct mpd_status *status = mpd_run_status(partition_state->conn);
        if (status == NULL) {
            mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id);
            return buffer;
        }
        enum mpd_single_state single_state = mpd_status_get_single_state(status);
        buffer = tojson_char(buffer, "single", mpd_lookup_single_state(single_state), true);
        buffer = tojson_uint(buffer, "crossfade", mpd_status_get_crossfade(status), true);
        buffer = tojson_double(buffer, "mixrampDb", mpd_status_get_mixrampdb(status), true);
        buffer = tojson_double(buffer, "mixrampDelay", mpd_status_get_mixrampdelay(status), true);
        buffer = tojson_bool(buffer, "repeat", mpd_status_get_repeat(status), true);
        buffer = tojson_bool(buffer, "random", mpd_status_get_random(status), true);
        enum mpd_consume_state consume_state = mpd_status_get_consume_state(status);
        buffer = tojson_char(buffer, "consume", mpd_lookup_consume_state(consume_state), true);
        mpd_status_free(status);

        enum mpd_replay_gain_mode replay_gain_mode = mpd_run_replay_gain_status(partition_state->conn);
        if (replay_gain_mode == MPD_REPLAY_UNKNOWN) {
            if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
                return buffer;
            }
        }
        const char *replaygain = mpd_lookup_replay_gain_mode(replay_gain_mode);
        buffer = tojson_char(buffer, "replaygain", replaygain == NULL ? "" : replaygain, false);
    }
    else {
        //not connected to mpd
        buffer = tojson_bool(buffer, "mpdConnected", false, false);
    }
    //features
    buffer = sdscat(buffer, "},\"features\":{");
    if (partition_state->conn_state == MPD_CONNECTED) {
        buffer = tojson_bool(buffer, "featPlaylists", partition_state->mpd_state->feat_playlists, true);
        buffer = tojson_bool(buffer, "featTags", partition_state->mpd_state->feat_tags, true);
        buffer = tojson_bool(buffer, "featLibrary", partition_state->mpd_state->feat_library, true);
        buffer = tojson_bool(buffer, "featStickers", partition_state->mpd_state->feat_stickers, true);
        buffer = tojson_bool(buffer, "featFingerprint", partition_state->mpd_state->feat_fingerprint, true);
        buffer = tojson_bool(buffer, "featPartitions", partition_state->mpd_state->feat_partitions, true);
        buffer = tojson_bool(buffer, "featMounts", partition_state->mpd_state->feat_mount, true);
        buffer = tojson_bool(buffer, "featNeighbors", partition_state->mpd_state->feat_neighbor, true);
        buffer = tojson_bool(buffer, "featBinarylimit", partition_state->mpd_state->feat_binarylimit, true);
        buffer = tojson_bool(buffer, "featPlaylistRmRange", partition_state->mpd_state->feat_playlist_rm_range, true);
        buffer = tojson_bool(buffer, "featWhence", partition_state->mpd_state->feat_whence, true);
        buffer = tojson_bool(buffer, "featAdvqueue", partition_state->mpd_state->feat_advqueue, true);
        buffer = tojson_bool(buffer, "featConsumeOneshot", partition_state->mpd_state->feat_consume_oneshot, true);
        buffer = tojson_bool(buffer, "featPlaylistDirAuto", partition_state->mpd_state->feat_playlist_dir_auto, true);
        buffer = tojson_bool(buffer, "featStartsWith", partition_state->mpd_state->feat_starts_with, true);
        buffer = tojson_bool(buffer, "featPcre", partition_state->mpd_state->feat_pcre, true);
    }
#ifdef MYMPD_ENABLE_SSL
    buffer = tojson_bool(buffer, "featCacert", (mympd_state->config->custom_cert == false && mympd_state->config->ssl == true ? true : false), true);
#else
    buffer = tojson_bool(buffer, "featCacert", false, true);
#endif
#ifdef MYMPD_ENABLE_LUA
    buffer = tojson_bool(buffer, "featScripting", true, false);
#else
    buffer = tojson_bool(buffer, "featScripting", false, false);
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
 * @param buffer already allocated sds string to append the response
 * @param key setting key
 * @param value setting value
 * @return pointer to buffer
 */
static sds set_invalid_value(sds buffer, sds key, sds value) {
    sdsclear(buffer);
    buffer = sdscatfmt(buffer, "Invalid value for \"%s\": \"%s\"", key, value);
    MYMPD_LOG_WARN("%s", buffer);
    return buffer;
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
