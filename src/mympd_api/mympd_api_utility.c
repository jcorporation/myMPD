/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_utility.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_sticker.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "mympd_api_timer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void default_mympd_state(struct t_mympd_state *mympd_state) {
    mympd_state->music_directory = sdsnew("auto");
    mympd_state->music_directory_value = sdsempty();
    mympd_state->playlist_directory = sdsnew("/var/lib/mpd/playlists");
    mympd_state->jukebox_mode = JUKEBOX_OFF;
    mympd_state->jukebox_playlist = sdsnew("Database");
    mympd_state->jukebox_unique_tag.len = 1;
    mympd_state->jukebox_unique_tag.tags[0] = MPD_TAG_ARTIST;
    mympd_state->jukebox_last_played = 24;
    mympd_state->jukebox_queue_length = 1;
    mympd_state->jukebox_enforce_unique = true;
    mympd_state->coverimage_names = sdsnew("folder,cover");
    mympd_state->tag_list_search = sdsnew("Artist,Album,AlbumArtist,Title,Genre");
    mympd_state->tag_list_browse = sdsnew("Artist,Album,AlbumArtist,Genre");
    mympd_state->smartpls_generate_tag_list = sdsnew("Genre");
    mympd_state->last_played_count = 200;
    mympd_state->smartpls = true;
    mympd_state->smartpls_sort = sdsempty();
    mympd_state->smartpls_prefix = sdsnew("myMPDsmart");
    mympd_state->smartpls_interval = 14400;
    mympd_state->booklet_name = sdsnew("booklet.pdf");
    mympd_state->auto_play = false;
    mympd_state->cols_queue_current = sdsnew("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    mympd_state->cols_search = sdsnew("[\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    mympd_state->cols_browse_database_detail = sdsnew("[\"Track\",\"Title\",\"Duration\"]");
    mympd_state->cols_browse_playlists_detail = sdsnew("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    mympd_state->cols_browse_filesystem = sdsnew("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"Duration\"]");
    mympd_state->cols_playback = sdsnew("[\"Artist\",\"Album\"]");
    mympd_state->cols_queue_last_played = sdsnew("[\"Pos\",\"Title\",\"Artist\",\"Album\",\"LastPlayed\"]");
    mympd_state->cols_queue_jukebox = sdsnew("[\"Pos\",\"Title\",\"Artist\",\"Album\"]");
    mympd_state->volume_min = 0;
    mympd_state->volume_max = 100;
    mympd_state->volume_step = 5;
    mympd_state->mpd_stream_port = 8080;
    mympd_state->webui_settings = sdsnew("{}");
    mympd_state->lyrics_uslt_ext = sdsnew("txt");
    mympd_state->lyrics_sylt_ext = sdsnew("lrc");
    mympd_state->lyrics_vorbis_uslt = sdsnew("LYRICS");
    mympd_state->lyrics_vorbis_sylt = sdsnew("SYNCEDLYRICS");
    mympd_state->covercache_keep_days = 7;
    reset_t_tags(&mympd_state->tag_types_search);
    reset_t_tags(&mympd_state->tag_types_browse);
    reset_t_tags(&mympd_state->smartpls_generate_tag_types);
    //init last played songs list
    list_init(&mympd_state->last_played);
    //init sticker queue
    list_init(&mympd_state->sticker_queue);
    //sticker cache
    mympd_state->sticker_cache_building = false;
    mympd_state->sticker_cache = NULL;
    //album cache
    mympd_state->album_cache_building = false;
    mympd_state->album_cache = NULL;
    //jukebox queue
    list_init(&mympd_state->jukebox_queue);
    list_init(&mympd_state->jukebox_queue_tmp);
    //mpd state
    mympd_state->mpd_state = (struct t_mpd_state *)malloc(sizeof(struct t_mpd_state));
    assert(mympd_state->mpd_state);
    mpd_shared_default_mpd_state(mympd_state->mpd_state);
    //triggers;
    list_init(&mympd_state->triggers);
    //home icons
    list_init(&mympd_state->home_list);
    //timer
    init_timerlist(&mympd_state->timer_list);
}

void free_mympd_state(struct t_mympd_state *mympd_state) {
    list_free(&mympd_state->jukebox_queue);
    list_free(&mympd_state->jukebox_queue_tmp);
    list_free(&mympd_state->sticker_queue);
    list_free(&mympd_state->triggers);
    list_free(&mympd_state->last_played);
    list_free(&mympd_state->home_list);
    free_timerlist(&mympd_state->timer_list);
    //mpd state
    mpd_shared_free_mpd_state(mympd_state->mpd_state);
    //caches
    sticker_cache_free(&mympd_state->sticker_cache);
    album_cache_free(&mympd_state->album_cache);
    //struct itself
    free_mympd_state_sds(mympd_state);
    FREE_PTR(mympd_state);
}

void free_mympd_state_sds(struct t_mympd_state *mympd_state) {
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
    FREE_SDS(mympd_state->coverimage_names);
    FREE_SDS(mympd_state->music_directory);
    FREE_SDS(mympd_state->music_directory_value);
    FREE_SDS(mympd_state->smartpls_sort);
    FREE_SDS(mympd_state->smartpls_prefix);
    FREE_SDS(mympd_state->booklet_name);
    FREE_SDS(mympd_state->navbar_icons);
    FREE_SDS(mympd_state->webui_settings);
    FREE_SDS(mympd_state->playlist_directory);
    FREE_SDS(mympd_state->lyrics_sylt_ext);
    FREE_SDS(mympd_state->lyrics_uslt_ext);
    FREE_SDS(mympd_state->lyrics_vorbis_uslt);
    FREE_SDS(mympd_state->lyrics_vorbis_sylt);
}

sds json_to_cols(sds cols, sds s, bool *error) {
    struct list col_list;
    list_init(&col_list);
    if (json_get_array_string(s, "$.params.cols", &col_list, vcb_iscolumn, 20, NULL) == true) {
        cols = list_to_json_array(cols, &col_list);
        *error = false;
    }
    else {
        *error = true;
    }
    list_free(&col_list);
    return cols;
}
