/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/mympd_state.h"

#include "src/lib/album_cache.h"
#include "src/lib/event.h"
#include "src/lib/last_played.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/timer.h"
#include "src/lib/utility.h"
#include "src/mpd_client/presets.h"
#include "src/mympd_api/home.h"
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

    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        last_played_file_save(partition_state);
        preset_list_save(partition_state);
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
    mympd_state->view_queue_current = sdsnew(MYMPD_VIEW_QUEUE_CURRENT);
    mympd_state->view_search = sdsnew(MYMPD_VIEW_SEARCH);
    mympd_state->view_browse_database_album_detail_info = sdsnew(MYMPD_VIEW_BROWSE_DATABASE_ALBUM_DETAIL_INFO);
    mympd_state->view_browse_database_album_detail = sdsnew(MYMPD_VIEW_BROWSE_DATABASE_ALBUM_DETAIL);
    mympd_state->view_browse_database_album_list = sdsnew(MYMPD_VIEW_BROWSE_DATABASE_ALBUM_LIST);
    mympd_state->view_browse_database_tag_list = sdsnew(MYMPD_VIEW_BROWSE_DATABASE_TAG_LIST);
    mympd_state->view_browse_playlist_list = sdsnew(MYMPD_VIEW_BROWSE_PLAYLIST_LIST);
    mympd_state->view_browse_playlist_detail = sdsnew(MYMPD_VIEW_BROWSE_PLAYLIST_DETAIL);
    mympd_state->view_browse_filesystem = sdsnew(MYMPD_VIEW_BROWSE_FILESYSTEM);
    mympd_state->view_playback = sdsnew(MYMPD_VIEW_PLAYBACK);
    mympd_state->view_queue_last_played = sdsnew(MYMPD_VIEW_QUEUE_LAST_PLAYED);
    mympd_state->view_queue_jukebox_song = sdsnew(MYMPD_VIEW_QUEUE_JUKEBOX_SONG);
    mympd_state->view_queue_jukebox_album = sdsnew(MYMPD_VIEW_QUEUE_JUKEBOX_ALBUM);
    mympd_state->view_browse_radio_webradiodb = sdsnew(MYMPD_VIEW_BROWSE_RADIO_WEBRADIODB);
    mympd_state->view_browse_radio_radiobrowser = sdsnew(MYMPD_VIEW_BROWSE_RADIO_RADIOBROWSER);
    mympd_state->view_browse_radio_favorites = sdsnew(MYMPD_VIEW_BROWSE_RADIO_FAVORITES);
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
    tags_reset(&mympd_state->smartpls_generate_tag_types);
    mympd_state->tag_disc_empty_is_first = MYMPD_TAG_DISC_EMPTY_IS_FIRST;
    mympd_state->booklet_name = sdsnew(MYMPD_BOOKLET_NAME);
    mympd_state->info_txt_name = sdsnew(MYMPD_INFO_TXT_NAME);
    //mpd shared state
    mympd_state->mpd_state = malloc_assert(sizeof(struct t_mpd_state));
    mpd_state_default(mympd_state->mpd_state, config);
    //mpd partition state
    mympd_state->partition_state = malloc_assert(sizeof(struct t_partition_state));
    partition_state_default(mympd_state->partition_state, MPD_PARTITION_DEFAULT, mympd_state->mpd_state, config);
    // stickerdb
    // use the partition struct to store the mpd connection for the stickerdb
    mympd_state->stickerdb = malloc_assert(sizeof(struct t_stickerdb_state));
    stickerdb_state_default(mympd_state->stickerdb, config);
    // do not use the shared mpd_state - we can connect to another mpd server for stickers
    mympd_state->stickerdb->mpd_state = malloc_assert(sizeof(struct t_mpd_state));
    mpd_state_default(mympd_state->stickerdb->mpd_state, config);
    //triggers;
    list_init(&mympd_state->trigger_list);
    //home icons
    list_init(&mympd_state->home_list);
    //timer
    mympd_api_timer_timerlist_init(&mympd_state->timer_list);
    //album cache
    cache_init(&mympd_state->album_cache);
    //init last played songs list
    mympd_state->last_played_count = MYMPD_LAST_PLAYED_COUNT;
    //poll fds
    event_pfd_init(&mympd_state->pfds);
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
    //stickerdb
    mpd_state_free(mympd_state->stickerdb->mpd_state);
    stickerdb_state_free(mympd_state->stickerdb);
    //caches
    album_cache_free(&mympd_state->album_cache);
    cache_free(&mympd_state->album_cache);
    //sds
    FREE_SDS(mympd_state->tag_list_search);
    FREE_SDS(mympd_state->tag_list_browse);
    FREE_SDS(mympd_state->smartpls_generate_tag_list);
    FREE_SDS(mympd_state->view_queue_current);
    FREE_SDS(mympd_state->view_search);
    FREE_SDS(mympd_state->view_browse_database_album_detail_info);
    FREE_SDS(mympd_state->view_browse_database_album_detail);
    FREE_SDS(mympd_state->view_browse_database_album_list);
    FREE_SDS(mympd_state->view_browse_database_tag_list);
    FREE_SDS(mympd_state->view_browse_playlist_list);
    FREE_SDS(mympd_state->view_browse_playlist_detail);
    FREE_SDS(mympd_state->view_browse_filesystem);
    FREE_SDS(mympd_state->view_playback);
    FREE_SDS(mympd_state->view_queue_last_played);
    FREE_SDS(mympd_state->view_queue_jukebox_song);
    FREE_SDS(mympd_state->view_queue_jukebox_album);
    FREE_SDS(mympd_state->view_browse_radio_webradiodb);
    FREE_SDS(mympd_state->view_browse_radio_radiobrowser);
    FREE_SDS(mympd_state->view_browse_radio_favorites);
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
    FREE_SDS(mympd_state->booklet_name);
    FREE_SDS(mympd_state->info_txt_name);
    //struct itself
    FREE_PTR(mympd_state);
}

/**
 * Sets mpd_state defaults.
 * @param mpd_state pointer to mpd_state
 * @param config pointer to static config
 */
void mpd_state_default(struct t_mpd_state *mpd_state, struct t_config *config) {
    mpd_state->config = config;
    mpd_state->mpd_keepalive = MYMPD_MPD_KEEPALIVE;
    mpd_state->mpd_timeout = MYMPD_MPD_TIMEOUT;
    mpd_state->mpd_host = sdsnew(MYMPD_MPD_HOST);
    mpd_state->mpd_port = MYMPD_MPD_PORT;
    mpd_state->mpd_pass = sdsnew(MYMPD_MPD_PASS);
    mpd_state->mpd_binarylimit = MYMPD_MPD_BINARYLIMIT;
    mpd_state->music_directory_value = sdsempty();
    mpd_state->playlist_directory_value = sdsempty();
    mpd_state->tag_list = sdsnew(MYMPD_MPD_TAG_LIST);
    tags_reset(&mpd_state->tags_mympd);
    tags_reset(&mpd_state->tags_mpd);
    tags_reset(&mpd_state->tags_search);
    tags_reset(&mpd_state->tags_browse);
    tags_reset(&mpd_state->tags_album);
    mpd_state->tag_albumartist = MPD_TAG_ALBUM_ARTIST;
    //features
    mpd_state_features_default(&mpd_state->feat);
}

/**
 * Copy mpd state
 * @param src source
 * @param dst destination
 */
void mpd_state_copy(struct t_mpd_state *src, struct t_mpd_state *dst) {
    dst->config = src->config;
    dst->mpd_keepalive = src->mpd_keepalive;
    dst->mpd_timeout = src->mpd_timeout;
    dst->mpd_host = sdsdup(src->mpd_host);
    dst->mpd_port = src->mpd_port;
    dst->mpd_pass = sdsdup(src->mpd_pass);
    dst->mpd_binarylimit = src->mpd_binarylimit;
    dst->music_directory_value = sdsdup(src->music_directory_value);
    dst->playlist_directory_value = sdsdup(src->playlist_directory_value);
    dst->tag_list = sdsdup( src->tag_list);
    tags_clone(&src->tags_mympd, &dst->tags_mympd);
    tags_clone(&src->tags_mpd, &dst->tags_mpd);
    tags_clone(&src->tags_search, &dst->tags_search);
    tags_clone(&src->tags_browse, &dst->tags_browse);
    tags_clone(&src->tags_album, &dst->tags_album);
    dst->tag_albumartist = src->tag_albumartist;
    mpd_state_features_copy(&src->feat, &dst->feat);
}

/**
 * Sets all feature states to default
 * @param feat pointer to mpd feature struct
 */
void mpd_state_features_default(struct t_mpd_features *feat) {
    feat->stickers = false;
    feat->playlists = false;
    feat->tags = false;
    feat->fingerprint = false;
    feat->albumart = false;
    feat->readpicture = false;
    feat->mount = false;
    feat->neighbor = false;
    feat->partitions = false;
    feat->binarylimit = false;
    feat->playlist_rm_range = false;
    feat->whence = false;
    feat->advqueue = false;
    feat->consume_oneshot = false;
    feat->playlist_dir_auto = false;
    feat->starts_with = false;
    feat->pcre = true;
    feat->db_added = false;
    feat->sticker_sort_window = false;
    feat->sticker_int = false;
    feat->search_add_sort_window = false;
    feat->listplaylist_range = false;
}

/**
 * Copy mpd state feature flags
 * @param src source
 * @param dst destination
 */
void mpd_state_features_copy(struct t_mpd_features *src, struct t_mpd_features *dst) {
    memcpy((void *)dst, (void *)src, sizeof(struct t_mpd_features));
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
    //struct itself
    FREE_PTR(mpd_state);
}

/**
 * Sets per partition state defaults
 * @param partition_state pointer to t_partition_state struct
 * @param name partition name
 * @param mpd_state pointer to shared mpd state
 * @param config pointer to static config
 */
void partition_state_default(struct t_partition_state *partition_state, const char *name,
        struct t_mpd_state *mpd_state, struct t_config *config)
{
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
    partition_state->song_id = -1;
    partition_state->song_uri = sdsempty();
    partition_state->next_song_id = -1;
    partition_state->last_song_id = -1;
    partition_state->last_song_uri = sdsempty();
    partition_state->queue_version = 0;
    partition_state->queue_length = 0;
    partition_state->song_start_time = 0;
    partition_state->song_end_time = 0;
    partition_state->last_song_start_time = 0;
    partition_state->last_song_end_time = 0;
    partition_state->last_skipped_id = 0;
    partition_state->crossfade = 0;
    partition_state->auto_play = MYMPD_AUTO_PLAY;
    partition_state->next = NULL;
    partition_state->player_error = false;
    //jukebox
    jukebox_state_default(&partition_state->jukebox);
    //add pointer to other states
    partition_state->config = config;
    partition_state->mpd_state = mpd_state;
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
    partition_state->stream_uri = sdsnew(PARTITION_MPD_STREAM_URI);
    //lists
    list_init(&partition_state->last_played);
    list_init(&partition_state->preset_list);
    preset_list_load(partition_state);
    //timers
    partition_state->timer_fd_jukebox = mympd_timer_create(CLOCK_MONOTONIC, 0, 0);
    partition_state->timer_fd_scrobble = mympd_timer_create(CLOCK_MONOTONIC, 0, 0);
    partition_state->timer_fd_mpd_connect = mympd_timer_create(CLOCK_MONOTONIC, 0, 0);
    //events
    partition_state->waiting_events = 0;
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
    jukebox_state_free(&partition_state->jukebox);
    //lists
    list_clear(&partition_state->last_played);
    list_clear(&partition_state->preset_list);
    //local playback
    FREE_SDS(partition_state->stream_uri);
    //timers
    mympd_timer_close(partition_state->timer_fd_jukebox);
    mympd_timer_close (partition_state->timer_fd_scrobble);
    mympd_timer_close(partition_state->timer_fd_mpd_connect);
    //struct itself
    FREE_PTR(partition_state);
}

/**
 * Sets jukebox state defaults
 * @param jukebox_state pointer to t_jukebox_state struct
 */
void jukebox_state_default(struct t_jukebox_state *jukebox_state) {
    jukebox_state->queue = list_new();
    jukebox_state->mode = JUKEBOX_OFF;
    jukebox_state->playlist = sdsnew(MYMPD_JUKEBOX_PLAYLIST);
    jukebox_state->uniq_tag.len = 1;
    jukebox_state->uniq_tag.tags[0] = MYMPD_JUKEBOX_UNIQ_TAG;
    jukebox_state->last_played = MYMPD_JUKEBOX_LAST_PLAYED;
    jukebox_state->queue_length = MYMPD_JUKEBOX_QUEUE_LENGTH;
    jukebox_state->ignore_hated = MYMPD_JUKEBOX_IGNORE_HATED;
    jukebox_state->filter_include = sdsempty();
    jukebox_state->filter_exclude = sdsempty();
    jukebox_state->min_song_duration = MYMPD_JUKEBOX_MIN_SONG_DURATION;
    jukebox_state->max_song_duration = MYMPD_JUKEBOX_MAX_SONG_DURATION;
    jukebox_state->filling = false;
    jukebox_state->last_error = sdsempty();
}

/**
 * Frees the t_jukebox_state struct
 * @param jukebox_state pointer to t_jukebox_state struct
 */
void jukebox_state_free(struct t_jukebox_state *jukebox_state) {
    FREE_SDS(jukebox_state->playlist);
    FREE_SDS(jukebox_state->filter_include);
    FREE_SDS(jukebox_state->filter_exclude);
    FREE_SDS(jukebox_state->last_error);
    list_free(jukebox_state->queue);
}

/**
 * Copies the jukebox settings
 * @param src source
 * @param dst destination
 */
void jukebox_state_copy(struct t_jukebox_state *src, struct t_jukebox_state *dst) {
    dst->mode = src->mode;
    dst->playlist = sds_replace(dst->playlist, src->playlist);
    dst->filter_include = sds_replace(dst->filter_include, src->filter_include);
    dst->filter_exclude = sds_replace(dst->filter_exclude, src->filter_exclude);
    dst->uniq_tag.tags[0] = src->uniq_tag.tags[0];
    dst->last_played = src->last_played;
    dst->ignore_hated = src->ignore_hated;
    dst->min_song_duration = src->min_song_duration;
    dst->max_song_duration = src->max_song_duration;
    dst->filling = src->filling;
    struct t_list_node *current = src->queue->head;
    while (current != NULL) {
        list_push(dst->queue, current->key, current->value_i, current->value_p, current->user_data);
        current = current->next;
    }
}

/**
 * Sets stickerdb state defaults
 * @param stickerdb pointer to stickerdb state
 * @param config pointer to static config
 */
void stickerdb_state_default(struct t_stickerdb_state *stickerdb, struct t_config *config) {
    stickerdb->config = config;
    stickerdb->mpd_state = NULL;
    stickerdb->conn_state = MPD_DISCONNECTED;
    stickerdb->conn = NULL;
    stickerdb->name = sdsnew("stickerdb");
}

/**
 * Frees the t_stickerdb_state struct
 * @param stickerdb pointer to struct
 */
void stickerdb_state_free(struct t_stickerdb_state *stickerdb) {
    FREE_SDS(stickerdb->name);
    FREE_PTR(stickerdb);
}
