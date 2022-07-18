/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_state.h"

#include "../lib/album_cache.h"
#include "../lib/sticker_cache.h"
#include "../mpd_client/mpd_client_jukebox.h"
#include "../mpd_client/mpd_client_tags.h"
#include "../mympd_api/mympd_api_timer.h"
#include "log.h"
#include "mem.h"
#include "sds_extras.h"

#include <stdlib.h>
#include <string.h>

//mympd state
void mympd_state_default(struct t_mympd_state *mympd_state) {
    mympd_state->music_directory = sdsnew(MYMPD_MUSIC_DIRECTORY);
    mympd_state->music_directory_value = sdsempty();
    mympd_state->playlist_directory = sdsnew(MYMPD_PLAYLIST_DIRECTORY);
    mympd_state->jukebox_mode = JUKEBOX_OFF;
    mympd_state->jukebox_playlist = sdsnew(MYMPD_JUKEBOX_PLAYLIST);
    mympd_state->jukebox_unique_tag.len = 1;
    mympd_state->jukebox_unique_tag.tags[0] = MYMPD_JUKEBOX_UNIQUE_TAG;
    mympd_state->jukebox_last_played = MYMPD_JUKEBOX_LAST_PLAYED;
    mympd_state->jukebox_queue_length = MYMPD_JUKEBOX_QUEUE_LENGTH;
    mympd_state->jukebox_enforce_unique = MYMPD_JUKEBOX_ENFORCE_UNIQUE;
    mympd_state->coverimage_names = sdsnew(MYMPD_COVERIMAGE_NAMES);
    mympd_state->thumbnail_names = sdsnew(MYMPD_THUMBNAIL_NAMES);
    mympd_state->tag_list_search = sdsnew(MYMPD_TAG_LIST_SEARCH);
    mympd_state->tag_list_browse = sdsnew(MYMPD_TAG_LIST_BROWSE);
    mympd_state->smartpls_generate_tag_list = sdsnew(MYMPD_SMARTPLS_GENERATE_TAG_LIST);
    mympd_state->last_played_count = MYMPD_LAST_PLAYED_COUNT;
    mympd_state->smartpls = MYMPD_SMARTPLS;
    mympd_state->smartpls_sort = sdsnew(MYMPD_SMARTPLS_SORT);
    mympd_state->smartpls_prefix = sdsnew(MYMPD_SMARTPLS_PREFIX);
    mympd_state->smartpls_interval = MYMPD_SMARTPLS_INTERVAL;
    mympd_state->booklet_name = sdsnew(MYMPD_BOOKLET_NAME);
    mympd_state->auto_play = MYMPD_AUTO_PLAY;
    mympd_state->cols_queue_current = sdsnew(MYMPD_COLS_QUEUE_CURRENT);
    mympd_state->cols_search = sdsnew(MYMPD_COLS_SEARCH);
    mympd_state->cols_browse_database_detail = sdsnew(MYMPD_COLS_BROWSE_DATABASE_DETAIL);
    mympd_state->cols_browse_playlists_detail = sdsnew(MYMPD_COLS_BROWSE_PLAYLISTS_DETAIL);
    mympd_state->cols_browse_filesystem = sdsnew(MYMPD_COLS_BROWSE_FILESYSTEM);
    mympd_state->cols_playback = sdsnew(MYMPD_COLS_PLAYBACK);
    mympd_state->cols_queue_last_played = sdsnew(MYMPD_COLS_QUEUE_LAST_PLAYED);
    mympd_state->cols_queue_jukebox = sdsnew(MYMPD_COLS_QUEUE_JUKEBOX);
    mympd_state->cols_browse_radio_webradiodb = sdsnew(MYMPD_COLS_BROWSE_RADIO_WEBRADIODB);
    mympd_state->cols_browse_radio_radiobrowser = sdsnew(MYMPD_COLS_BROWSE_RADIO_RADIOBROWSER);
    mympd_state->volume_min = MYMPD_VOLUME_MIN;
    mympd_state->volume_max = MYMPD_VOLUME_MAX;
    mympd_state->volume_step = MYMPD_VOLUME_STEP;
    mympd_state->mpd_stream_port = MYMPD_MPD_STREAM_PORT;
    mympd_state->webui_settings = sdsnew(MYMPD_WEBUI_SETTINGS);
    mympd_state->lyrics.uslt_ext = sdsnew(MYMPD_LYRICS_USLT_EXT);
    mympd_state->lyrics.sylt_ext = sdsnew(MYMPD_LYRICS_SYLT_EXT);
    mympd_state->lyrics.vorbis_uslt = sdsnew(MYMPD_LYRICS_VORBIS_USLT);
    mympd_state->lyrics.vorbis_sylt = sdsnew(MYMPD_LYRICS_VORBIS_SYLT);
    mympd_state->covercache_keep_days = MYMPD_COVERCACHE_KEEP_DAYS;
    mympd_state->listenbrainz_token = sdsempty();
    mympd_state->navbar_icons = sdsnew(MYMPD_NAVBAR_ICONS);
    reset_t_tags(&mympd_state->tag_types_search);
    reset_t_tags(&mympd_state->tag_types_browse);
    reset_t_tags(&mympd_state->smartpls_generate_tag_types);
    //init last played songs list
    list_init(&mympd_state->last_played);
    //init sticker queue
    list_init(&mympd_state->sticker_queue);
    //sticker cache
    mympd_state->sticker_cache.building = false;
    mympd_state->sticker_cache.cache = NULL;
    //album cache
    mympd_state->album_cache.building = false;
    mympd_state->album_cache.cache = NULL;
    //jukebox queue
    list_init(&mympd_state->jukebox_queue);
    list_init(&mympd_state->jukebox_queue_tmp);
    //mpd state
    mympd_state->mpd_state = malloc_assert(sizeof(struct t_mpd_state));
    mympd_state_default_mpd_state(mympd_state->mpd_state);
    //triggers;
    list_init(&mympd_state->trigger_list);
    //home icons
    list_init(&mympd_state->home_list);
    //timer
    mympd_api_timer_timerlist_init(&mympd_state->timer_list);
}

void *mympd_state_free(struct t_mympd_state *mympd_state) {
    mpd_client_clear_jukebox(&mympd_state->jukebox_queue);
    mpd_client_clear_jukebox(&mympd_state->jukebox_queue_tmp);
    list_clear(&mympd_state->sticker_queue);
    list_clear_user_data(&mympd_state->trigger_list, list_free_cb_t_list_user_data);
    list_clear(&mympd_state->last_played);
    list_clear(&mympd_state->home_list);
    mympd_api_timer_timerlist_clear(&mympd_state->timer_list);
    //mpd state
    mympd_state_free_mpd_state(mympd_state->mpd_state);
    //caches
    sticker_cache_free(&mympd_state->sticker_cache);
    album_cache_free(&mympd_state->album_cache);
    //sds
    FREE_SDS(mympd_state->tag_list_search);
    FREE_SDS(mympd_state->tag_list_browse);
    FREE_SDS(mympd_state->smartpls_generate_tag_list);
    FREE_SDS(mympd_state->jukebox_playlist);
    FREE_SDS(mympd_state->cols_queue_current);
    FREE_SDS(mympd_state->cols_search);
    FREE_SDS(mympd_state->cols_browse_database_detail);
    FREE_SDS(mympd_state->cols_browse_playlists_detail);
    FREE_SDS(mympd_state->cols_browse_filesystem);
    FREE_SDS(mympd_state->cols_playback);
    FREE_SDS(mympd_state->cols_queue_last_played);
    FREE_SDS(mympd_state->cols_queue_jukebox);
    FREE_SDS(mympd_state->cols_browse_radio_webradiodb);
    FREE_SDS(mympd_state->cols_browse_radio_radiobrowser);
    FREE_SDS(mympd_state->coverimage_names);
    FREE_SDS(mympd_state->thumbnail_names);
    FREE_SDS(mympd_state->music_directory);
    FREE_SDS(mympd_state->music_directory_value);
    FREE_SDS(mympd_state->smartpls_sort);
    FREE_SDS(mympd_state->smartpls_prefix);
    FREE_SDS(mympd_state->booklet_name);
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
    return NULL;
}

//mpd state
void mympd_state_default_mpd_state(struct t_mpd_state *mpd_state) {
    mpd_state->conn = NULL;
    mpd_state->conn_state = MPD_DISCONNECTED;
    mpd_state->reconnect_time = 0;
    mpd_state->reconnect_interval = 0;
    mpd_state->mpd_keepalive = MYMPD_MPD_KEEPALIVE;
    mpd_state->mpd_timeout = MYMPD_MPD_TIMEOUT;
    mpd_state->state = MPD_STATE_UNKNOWN;
    mpd_state->mpd_host = sdsnew(MYMPD_MPD_HOST);
    mpd_state->mpd_port = MYMPD_MPD_PORT;
    mpd_state->mpd_pass = sdsnew(MYMPD_MPD_PASS);
    mpd_state->mpd_binarylimit = MYMPD_MPD_BINARYLIMIT;
    mpd_state->song_id = -1;
    mpd_state->song_uri = sdsempty();
    mpd_state->next_song_id = -1;
    mpd_state->last_song_id = -1;
    mpd_state->last_song_uri = sdsempty();
    mpd_state->queue_version = 0;
    mpd_state->queue_length = 0;
    mpd_state->last_last_played_id = -1;
    mpd_state->song_end_time = 0;
    mpd_state->song_start_time = 0;
    mpd_state->last_song_end_time = 0;
    mpd_state->last_song_start_time = 0;
    mpd_state->last_skipped_id = 0;
    mpd_state->set_song_played_time = 0;
    mpd_state->last_song_set_song_played_time = 0;
    mpd_state->crossfade = 0;
    mpd_state->set_song_played_time = 0;
    mpd_state->tag_list = sdsnew(MYMPD_MPD_TAG_LIST);
    reset_t_tags(&mpd_state->tag_types_mympd);
    reset_t_tags(&mpd_state->tag_types_mpd);
    mpd_state->tag_albumartist = MPD_TAG_ALBUM_ARTIST;
}

void *mympd_state_free_mpd_state(struct t_mpd_state *mpd_state) {
    FREE_SDS(mpd_state->mpd_host);
    FREE_SDS(mpd_state->mpd_pass);
    FREE_SDS(mpd_state->song_uri);
    FREE_SDS(mpd_state->last_song_uri);
    FREE_SDS(mpd_state->tag_list);
    FREE_PTR(mpd_state);
    return NULL;
}

//tagtypes
void copy_tag_types(struct t_tags *src_tag_list, struct t_tags *dst_tag_list) {
    memcpy((void *)dst_tag_list, (void *)src_tag_list, sizeof(struct t_tags));
}

void reset_t_tags(struct t_tags *tags) {
    tags->len = 0;
    memset(tags->tags, 0, sizeof(tags->tags));
}
