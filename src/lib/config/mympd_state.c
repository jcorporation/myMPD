/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD state for the mympd_api thread
 */

#include "compile_time.h"
#include "src/lib/config/mympd_state.h"

#include "src/lib/cache/cache_rax_album.h"
#include "src/lib/event.h"
#include "src/lib/last_played.h"
#include "src/lib/mem.h"
#include "src/lib/sds/sds_extras.h"
#include "src/lib/webradio.h"
#include "src/mympd_api/home.h"
#include "src/mympd_api/timer.h"
#include "src/mympd_api/trigger.h"
#include "src/mympd_client/presets.h"

#include <string.h>

/**
 * Saves in-memory states to disc. This is done on shutdown and on SIGHUP.
 * @param mympd_state pointer to central myMPD state
 * @param free_data true=free the struct, else not
 */
void mympd_state_save(struct t_mympd_state *mympd_state, bool free_data) {
    // write album cache to disc
    // only for simple mode to save the cached uris
    if (mympd_state->config->albums.mode == ALBUM_MODE_SIMPLE) {
        album_cache_write(&mympd_state->album_cache, mympd_state->config->workdir,
            &mympd_state->mpd_state->tags_album, &mympd_state->config->albums, true);
    }
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        last_played_file_save(partition_state);
        preset_list_save(partition_state);
        jukebox_file_save(partition_state);
        partition_state = partition_state->next;
    }
    mympd_api_home_file_save(&mympd_state->home_list, mympd_state->config->workdir);
    mympd_api_timer_file_save(&mympd_state->timer_list, mympd_state->config->workdir);
    mympd_api_trigger_file_save(&mympd_state->trigger_list, mympd_state->config->workdir);
    webradios_save_to_disk(mympd_state->config, mympd_state->webradio_favorites, FILENAME_WEBRADIO_FAVORITES);
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
    //comma separated list of image names
    mympd_state->image_names_sm = sdsnew(MYMPD_IMAGE_NAMES_SM);
    mympd_state->image_names_md = sdsnew(MYMPD_IMAGE_NAMES_MD);
    mympd_state->image_names_lg = sdsnew(MYMPD_IMAGE_NAMES_LG);
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
    mympd_state->view_browse_radio_favorites = sdsnew(MYMPD_VIEW_BROWSE_RADIO_FAVORITES);
    mympd_state->volume_min = MYMPD_VOLUME_MIN;
    mympd_state->volume_max = MYMPD_VOLUME_MAX;
    mympd_state->volume_step = MYMPD_VOLUME_STEP;
    mympd_state->webui_settings = sdsnew(MYMPD_WEBUI_SETTINGS);
    mympd_state->lyrics.uslt_ext = sdsnew(MYMPD_LYRICS_USLT_EXT);
    mympd_state->lyrics.sylt_ext = sdsnew(MYMPD_LYRICS_SYLT_EXT);
    mympd_state->lyrics.vorbis_uslt = sdsnew(MYMPD_LYRICS_VORBIS_USLT);
    mympd_state->lyrics.vorbis_sylt = sdsnew(MYMPD_LYRICS_VORBIS_SYLT);
    mympd_state->navbar_icons = sdsnew(MYMPD_NAVBAR_ICONS);
    mympd_mpd_tags_reset(&mympd_state->smartpls_generate_tag_types);
    mympd_state->tag_disc_empty_is_first = MYMPD_TAG_DISC_EMPTY_IS_FIRST;
    mympd_state->booklet_name = sdsnew(MYMPD_BOOKLET_NAME);
    mympd_state->info_txt_name = sdsnew(MYMPD_INFO_TXT_NAME);
    //mpd shared state
    mympd_state->mpd_state = malloc_assert(sizeof(struct t_mpd_state));
    mympd_mpd_state_default(mympd_state->mpd_state, config);
    //mpd partition state
    mympd_state->partition_state = malloc_assert(sizeof(struct t_partition_state));
    partition_state_default(mympd_state->partition_state, MPD_PARTITION_DEFAULT, mympd_state->mpd_state, config);
    mympd_state->partition_state->repopulate_pfds = &mympd_state->pfds.repopulate;
    // stickerdb
    // use the partition struct to store the mpd connection for the stickerdb
    mympd_state->stickerdb = malloc_assert(sizeof(struct t_stickerdb_state));
    stickerdb_state_default(mympd_state->stickerdb, config);
    mympd_state->stickerdb->repopulate_pfds = &mympd_state->pfds.repopulate;
    // do not use the shared mpd_state - we can connect to another mpd server for stickers
    mympd_state->stickerdb->mpd_state = malloc_assert(sizeof(struct t_mpd_state));
    mympd_mpd_state_default(mympd_state->stickerdb->mpd_state, config);
    //triggers;
    list_init(&mympd_state->trigger_list);
    //home icons
    list_init(&mympd_state->home_list);
    //timer
    mympd_api_timer_timerlist_init(&mympd_state->timer_list);
    mympd_state->timer_list.repopulate_pfds = &mympd_state->pfds.repopulate;
    //album cache
    cache_init(&mympd_state->album_cache);
    //init last played songs list
    mympd_state->last_played_count = MYMPD_LAST_PLAYED_COUNT;
    //poll fds
    event_pfd_init(&mympd_state->pfds);
    //webradios
    mympd_state->webradiodb = webradios_new();
    mympd_state->webradio_favorites = webradios_new();
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
    mympd_mpd_state_free(mympd_state->mpd_state);
    //partition state
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        struct t_partition_state *next = partition_state->next;
        partition_state_free(partition_state);
        partition_state = next;
    }
    //stickerdb
    mympd_mpd_state_free(mympd_state->stickerdb->mpd_state);
    stickerdb_state_free(mympd_state->stickerdb);
    //caches
    album_cache_free(&mympd_state->album_cache);
    cache_free(&mympd_state->album_cache);
    //webradioDB
    webradios_free(mympd_state->webradiodb);
    webradios_free(mympd_state->webradio_favorites);
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
    FREE_SDS(mympd_state->view_browse_radio_favorites);
    FREE_SDS(mympd_state->image_names_sm);
    FREE_SDS(mympd_state->image_names_md);
    FREE_SDS(mympd_state->image_names_lg);
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
    FREE_SDS(mympd_state->booklet_name);
    FREE_SDS(mympd_state->info_txt_name);
    //struct itself
    FREE_PTR(mympd_state);
}
