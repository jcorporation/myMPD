/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/mympd_state.h"

#include "src/lib/album_cache.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/sticker_cache.h"
#include "src/lib/utility.h"
#include "src/mpd_client/presets.h"
#include "src/mympd_api/home.h"
#include "src/mympd_api/last_played.h"
#include "src/mympd_api/timer.h"
#include "src/mympd_api/trigger.h"

#include <string.h>

/**
 * Saves in-memory states to disc. This is done on shutdown and on SIGHUP.
 * @param mympd_state pointer to central myMPD state
 * @param free_data true=free the struct, else not
 */
void mympd_state_save(struct t_mympd_state *mympd_state, bool free_data) {
    mympd_api_home_file_save(&mympd_state->home_list, mympd_state->config->workdir);
    mympd_api_timer_file_save(&mympd_state->timer_list, mympd_state->config->workdir);
    mympd_api_trigger_file_save(&mympd_state->trigger_list, mympd_state->config->workdir);
    sticker_cache_write(&mympd_state->mpd_state->sticker_cache, mympd_state->config->workdir, free_data);

    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        mympd_api_last_played_file_save(partition_state);
        presets_save(partition_state);
        partition_state = partition_state->next;
    }
    if (free_data == true) {
        mympd_state_free(mympd_state);
    }
}

/**
 * Sets mympd_state defaults.
 * @param mympd_state pointer to central myMPD state
 * @param config pointer to static config
 */
void mympd_state_default(struct t_mympd_state *mympd_state, struct t_config *config) {
    //pointer to static config
    mympd_state->config = config;
    //configured mpd music_directory, used value is in mympd_state->mpd_state->music_directory_value
    mympd_state->music_directory = sdsnew(MYMPD_MUSIC_DIRECTORY);
    //configured mpd playlist directory
    mympd_state->playlist_directory = sdsnew(MYMPD_PLAYLIST_DIRECTORY);
    //comma separated list of albumart names - normal size
    mympd_state->coverimage_names = sdsnew(MYMPD_COVERIMAGE_NAMES);
    //comma separated list of albumart names - thumbnails
    mympd_state->thumbnail_names = sdsnew(MYMPD_THUMBNAIL_NAMES);
    mympd_state->tag_list_search = sdsnew(MYMPD_TAG_LIST_SEARCH);
    mympd_state->tag_list_browse = sdsnew(MYMPD_TAG_LIST_BROWSE);
    mympd_state->smartpls_generate_tag_list = sdsnew(MYMPD_SMARTPLS_GENERATE_TAG_LIST);
    mympd_state->smartpls = MYMPD_SMARTPLS;
    mympd_state->smartpls_sort = sdsnew(MYMPD_SMARTPLS_SORT);
    mympd_state->smartpls_prefix = sdsnew(MYMPD_SMARTPLS_PREFIX);
    mympd_state->smartpls_interval = MYMPD_SMARTPLS_INTERVAL;
    mympd_state->cols_queue_current = sdsnew(MYMPD_COLS_QUEUE_CURRENT);
    mympd_state->cols_search = sdsnew(MYMPD_COLS_SEARCH);
    mympd_state->cols_browse_database_album_detail_info = sdsnew(MYMPD_COLS_BROWSE_DATABASE_ALBUM_DETAIL_INFO);
    mympd_state->cols_browse_database_album_detail = sdsnew(MYMPD_COLS_BROWSE_DATABASE_ALBUM_DETAIL);
    mympd_state->cols_browse_database_album_list = sdsnew(MYMPD_COLS_BROWSE_DATABASE_ALBUM_LIST);
    mympd_state->cols_browse_playlist_detail = sdsnew(MYMPD_COLS_BROWSE_PLAYLIST_DETAIL);
    mympd_state->cols_browse_filesystem = sdsnew(MYMPD_COLS_BROWSE_FILESYSTEM);
    mympd_state->cols_playback = sdsnew(MYMPD_COLS_PLAYBACK);
    mympd_state->cols_queue_last_played = sdsnew(MYMPD_COLS_QUEUE_LAST_PLAYED);
    mympd_state->cols_queue_jukebox_song = sdsnew(MYMPD_COLS_QUEUE_JUKEBOX_SONG);
    mympd_state->cols_queue_jukebox_album = sdsnew(MYMPD_COLS_QUEUE_JUKEBOX_ALBUM);
    mympd_state->cols_browse_radio_webradiodb = sdsnew(MYMPD_COLS_BROWSE_RADIO_WEBRADIODB);
    mympd_state->cols_browse_radio_radiobrowser = sdsnew(MYMPD_COLS_BROWSE_RADIO_RADIOBROWSER);
    mympd_state->volume_min = MYMPD_VOLUME_MIN;
    mympd_state->volume_max = MYMPD_VOLUME_MAX;
    mympd_state->volume_step = MYMPD_VOLUME_STEP;
    mympd_state->webui_settings = sdsnew(MYMPD_WEBUI_SETTINGS);
    mympd_state->lyrics.uslt_ext = sdsnew(MYMPD_LYRICS_USLT_EXT);
    mympd_state->lyrics.sylt_ext = sdsnew(MYMPD_LYRICS_SYLT_EXT);
    mympd_state->lyrics.vorbis_uslt = sdsnew(MYMPD_LYRICS_VORBIS_USLT);
    mympd_state->lyrics.vorbis_sylt = sdsnew(MYMPD_LYRICS_VORBIS_SYLT);
    mympd_state->listenbrainz_token = sdsempty();
    mympd_state->navbar_icons = sdsnew(MYMPD_NAVBAR_ICONS);
    reset_t_tags(&mympd_state->smartpls_generate_tag_types);
    //mpd shared state
    mympd_state->mpd_state = malloc_assert(sizeof(struct t_mpd_state));
    mpd_state_default(mympd_state->mpd_state, mympd_state);
    //mpd partition state
    mympd_state->partition_state = malloc_assert(sizeof(struct t_partition_state));
    partition_state_default(mympd_state->partition_state, MPD_PARTITION_DEFAULT, mympd_state);
    //triggers;
    list_init(&mympd_state->trigger_list);
    //home icons
    list_init(&mympd_state->home_list);
    //timer
    mympd_api_timer_timerlist_init(&mympd_state->timer_list);
}

/**
 * Frees the myMPD state, frees also the referenced structs.
 * @param mympd_state pointer to central myMPD state
 */
void mympd_state_free(struct t_mympd_state *mympd_state) {
    //trigger
    mympd_api_trigger_list_clear(&mympd_state->trigger_list);
    //home icons
    list_clear(&mympd_state->home_list);
    //timer
    mympd_api_timer_timerlist_clear(&mympd_state->timer_list);
    //mpd shared state
    mpd_state_free(mympd_state->mpd_state);
    //partition state
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        struct t_partition_state *next = partition_state->next;
        partition_state_free(partition_state);
        partition_state = next;
    }
    //sds
    FREE_SDS(mympd_state->tag_list_search);
    FREE_SDS(mympd_state->tag_list_browse);
    FREE_SDS(mympd_state->smartpls_generate_tag_list);
    FREE_SDS(mympd_state->cols_queue_current);
    FREE_SDS(mympd_state->cols_search);
    FREE_SDS(mympd_state->cols_browse_database_album_detail_info);
    FREE_SDS(mympd_state->cols_browse_database_album_detail);
    FREE_SDS(mympd_state->cols_browse_database_album_list);
    FREE_SDS(mympd_state->cols_browse_playlist_detail);
    FREE_SDS(mympd_state->cols_browse_filesystem);
    FREE_SDS(mympd_state->cols_playback);
    FREE_SDS(mympd_state->cols_queue_last_played);
    FREE_SDS(mympd_state->cols_queue_jukebox_song);
    FREE_SDS(mympd_state->cols_queue_jukebox_album);
    FREE_SDS(mympd_state->cols_browse_radio_webradiodb);
    FREE_SDS(mympd_state->cols_browse_radio_radiobrowser);
    FREE_SDS(mympd_state->coverimage_names);
    FREE_SDS(mympd_state->thumbnail_names);
    FREE_SDS(mympd_state->music_directory);
    FREE_SDS(mympd_state->smartpls_sort);
    FREE_SDS(mympd_state->smartpls_prefix);
    FREE_SDS(mympd_state->navbar_icons);
    FREE_SDS(mympd_state->webui_settings);
    FREE_SDS(mympd_state->playlist_directory);
    FREE_SDS(mympd_state->lyrics.sylt_ext);
    FREE_SDS(mympd_state->lyrics.uslt_ext);
    FREE_SDS(mympd_state->lyrics.vorbis_uslt);
    FREE_SDS(mympd_state->lyrics.vorbis_sylt);
    FREE_SDS(mympd_state->listenbrainz_token);
    //struct itself
    FREE_PTR(mympd_state);
}

/**
 * Sets mpd_state defaults.
 * @param mpd_state pointer to mpd_state
 * @param mympd_state pointer to central myMPD state
 */
void mpd_state_default(struct t_mpd_state *mpd_state, struct t_mympd_state *mympd_state) {
    mpd_state->mympd_state = mympd_state;
    mpd_state->mpd_keepalive = MYMPD_MPD_KEEPALIVE;
    mpd_state->mpd_timeout = MYMPD_MPD_TIMEOUT;
    mpd_state->mpd_host = sdsnew(MYMPD_MPD_HOST);
    mpd_state->mpd_port = MYMPD_MPD_PORT;
    mpd_state->mpd_pass = sdsnew(MYMPD_MPD_PASS);
    mpd_state->mpd_binarylimit = MYMPD_MPD_BINARYLIMIT;
    mpd_state->music_directory_value = sdsempty();
    mpd_state->playlist_directory_value = sdsempty();
    mpd_state->tag_list = sdsnew(MYMPD_MPD_TAG_LIST);
    reset_t_tags(&mpd_state->tags_mympd);
    reset_t_tags(&mpd_state->tags_mpd);
    reset_t_tags(&mpd_state->tags_search);
    reset_t_tags(&mpd_state->tags_browse);
    reset_t_tags(&mpd_state->tags_album);
    mpd_state->tag_albumartist = MPD_TAG_ALBUM_ARTIST;
    //sticker cache
    mpd_state->sticker_cache.building = false;
    mpd_state->sticker_cache.cache = NULL;
    //album cache
    mpd_state->album_cache.building = false;
    mpd_state->album_cache.cache = NULL;
    //init last played songs list
    mpd_state->last_played_count = MYMPD_LAST_PLAYED_COUNT;
    //init sticker queue
    list_init(&mpd_state->sticker_queue);
    //booklet name
    mpd_state->booklet_name = sdsnew(MYMPD_BOOKLET_NAME);
    //features
    mpd_state_features_disable(mpd_state);
}

/**
 * Sets all feature states to disabled
 * @param mpd_state pointer to mpd_state
 */
void mpd_state_features_disable(struct t_mpd_state *mpd_state) {
    mpd_state->feat_stickers = false;
    mpd_state->feat_playlists = false;
    mpd_state->feat_tags = false;
    mpd_state->feat_fingerprint = false;
    mpd_state->feat_albumart = false;
    mpd_state->feat_readpicture = false;
    mpd_state->feat_mount = false;
    mpd_state->feat_neighbor = false;
    mpd_state->feat_partitions = false;
    mpd_state->feat_binarylimit = false;
    mpd_state->feat_playlist_rm_range = false;
    mpd_state->feat_whence = false;
    mpd_state->feat_advqueue = false;
    mpd_state->feat_consume_oneshot = false;
    mpd_state->feat_playlist_dir_auto = false;
    mpd_state->feat_starts_with = false;
    mpd_state->feat_pcre = true;
}

/**
 * Frees the t_mpd_state struct
 */
void mpd_state_free(struct t_mpd_state *mpd_state) {
    FREE_SDS(mpd_state->mpd_host);
    FREE_SDS(mpd_state->mpd_pass);
    FREE_SDS(mpd_state->tag_list);
    FREE_SDS(mpd_state->music_directory_value);
    FREE_SDS(mpd_state->playlist_directory_value);
    //lists
    list_clear(&mpd_state->sticker_queue);
    //caches
    sticker_cache_free(&mpd_state->sticker_cache);
    album_cache_free(&mpd_state->album_cache);

    FREE_SDS(mpd_state->booklet_name);
    //struct itself
    FREE_PTR(mpd_state);
}

/**
 * Sets per partition state defaults
 * @param partition_state pointer to t_partition_state struct
 * @param name partition name
 * @param mympd_state pointer to central myMPD state
 */
void partition_state_default(struct t_partition_state *partition_state, const char *name, struct t_mympd_state *mympd_state) {
    partition_state->name = sdsnew(name);
    partition_state->highlight_color = sdsnew(PARTITION_HIGHLIGHT_COLOR);
    partition_state->highlight_color_contrast = sdsnew(PARTITION_HIGHLIGHT_COLOR_CONTRAST);
    sds partition_dir = sdsnew(name);
    sanitize_filename(partition_dir);
    partition_state->state_dir = sdscatfmt(sdsempty(), "%s/%S", DIR_WORK_STATE, partition_dir);
    FREE_SDS(partition_dir);
    partition_state->conn = NULL;
    partition_state->conn_state = MPD_DISCONNECTED;
    partition_state->play_state = MPD_STATE_UNKNOWN;
    partition_state->reconnect_time = 0;
    partition_state->reconnect_interval = 0;
    partition_state->song_id = -1;
    partition_state->song_uri = sdsempty();
    partition_state->next_song_id = -1;
    partition_state->last_song_id = -1;
    partition_state->last_song_uri = sdsempty();
    partition_state->queue_version = 0;
    partition_state->queue_length = 0;
    partition_state->last_scrobbled_id = -1;
    partition_state->song_start_time = 0;
    partition_state->song_scrobble_time = 0;
    partition_state->song_end_time = 0;
    partition_state->last_song_start_time = 0;
    partition_state->last_song_scrobble_time = 0;
    partition_state->last_song_end_time = 0;
    partition_state->last_skipped_id = 0;
    partition_state->crossfade = 0;
    partition_state->auto_play = MYMPD_AUTO_PLAY;
    partition_state->next = NULL;
    partition_state->player_error = false;
    //jukebox
    list_init(&partition_state->jukebox_queue);
    list_init(&partition_state->jukebox_queue_tmp);
    partition_state->jukebox_mode = JUKEBOX_OFF;
    partition_state->jukebox_playlist = sdsnew(MYMPD_JUKEBOX_PLAYLIST);
    partition_state->jukebox_unique_tag.len = 1;
    partition_state->jukebox_unique_tag.tags[0] = MYMPD_JUKEBOX_UNIQUE_TAG;
    partition_state->jukebox_last_played = MYMPD_JUKEBOX_LAST_PLAYED;
    partition_state->jukebox_queue_length = MYMPD_JUKEBOX_QUEUE_LENGTH;
    partition_state->jukebox_enforce_unique = MYMPD_JUKEBOX_ENFORCE_UNIQUE;
    partition_state->jukebox_ignore_hated = MYMPD_JUKEBOX_IGNORE_HATED;
    //add pointer to other states
    partition_state->mympd_state = mympd_state;
    partition_state->mpd_state = mympd_state->mpd_state;
    //mpd idle mask
    if (strcmp(name, MPD_PARTITION_DEFAULT) == 0) {
        partition_state->is_default = true;
        //handle all
        partition_state->idle_mask = MPD_IDLE_QUEUE | MPD_IDLE_PLAYER | MPD_IDLE_MIXER | MPD_IDLE_OUTPUT | MPD_IDLE_OPTIONS |
            MPD_IDLE_UPDATE | MPD_IDLE_PARTITION | MPD_IDLE_DATABASE | MPD_IDLE_STORED_PLAYLIST;
    }
    else {
        partition_state->is_default = false;
        //handle only partition specific mpd idle events
        partition_state->idle_mask = MPD_IDLE_QUEUE | MPD_IDLE_PLAYER | MPD_IDLE_MIXER | MPD_IDLE_OUTPUT | MPD_IDLE_OPTIONS;
    }
    partition_state->set_conn_options = false;
    //local playback
    partition_state->mpd_stream_port = PARTITION_MPD_STREAM_PORT;
    partition_state->stream_uri = sdsempty();
    //lists
    list_init(&partition_state->last_played);
    list_init(&partition_state->presets);
    presets_load(partition_state);
}

/**
 * Frees the t_partition_state struct
 * @param partition_state pointer to t_partition_state struct
 */
void partition_state_free(struct t_partition_state *partition_state) {
    FREE_SDS(partition_state->name);
    FREE_SDS(partition_state->highlight_color);
    FREE_SDS(partition_state->highlight_color_contrast);
    FREE_SDS(partition_state->state_dir);
    FREE_SDS(partition_state->song_uri);
    FREE_SDS(partition_state->last_song_uri);
    //jukebox
    FREE_SDS(partition_state->jukebox_playlist);
    //do not use jukebox_clear wrapper to prevent obsolet notification
    list_clear(&partition_state->jukebox_queue);
    list_clear(&partition_state->jukebox_queue_tmp);
    //lists
    list_clear(&partition_state->last_played);
    list_clear(&partition_state->presets);
    //local playback
    FREE_SDS(partition_state->stream_uri);
    //struct itself
    FREE_PTR(partition_state);
}

/**
 * Copy a struct t_tags struct to another one
 * @param src_tag_list source
 * @param dst_tag_list destination
 */
void copy_tag_types(struct t_tags *src_tag_list, struct t_tags *dst_tag_list) {
    memcpy((void *)dst_tag_list, (void *)src_tag_list, sizeof(struct t_tags));
}

/**
 * (Re-)initializes a t_tags struct
 * @param tags pointer to t_tags struct
*/
void reset_t_tags(struct t_tags *tags) {
    tags->len = 0;
    memset(tags->tags, 0, sizeof(tags->tags));
}
