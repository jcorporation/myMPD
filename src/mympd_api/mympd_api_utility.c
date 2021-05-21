/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../dist/src/rax/rax.h"
#include "../../dist/src/frozen/frozen.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "mympd_config_defs.h"
#include "../mympd_state.h"
#include "../api.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "../utility.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared/mpd_shared_sticker.h"
#include "../mpd_shared.h"
#include "mympd_api_timer.h"
#include "mympd_api_utility.h"

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
    sdsfree(mympd_state->tag_list_search);
    sdsfree(mympd_state->tag_list_browse);
    sdsfree(mympd_state->smartpls_generate_tag_list);
    sdsfree(mympd_state->jukebox_playlist);
    sdsfree(mympd_state->cols_queue_current);
    sdsfree(mympd_state->cols_search);
    sdsfree(mympd_state->cols_browse_database_detail);
    sdsfree(mympd_state->cols_browse_playlists_detail);
    sdsfree(mympd_state->cols_browse_filesystem);
    sdsfree(mympd_state->cols_playback);
    sdsfree(mympd_state->cols_queue_last_played);
    sdsfree(mympd_state->cols_queue_jukebox);
    sdsfree(mympd_state->coverimage_names);
    sdsfree(mympd_state->music_directory);
    sdsfree(mympd_state->music_directory_value);
    sdsfree(mympd_state->smartpls_sort);
    sdsfree(mympd_state->smartpls_prefix);
    sdsfree(mympd_state->booklet_name);
    sdsfree(mympd_state->navbar_icons);
    sdsfree(mympd_state->webui_settings);
    sdsfree(mympd_state->playlist_directory);
    sdsfree(mympd_state->lyrics_sylt_ext);
    sdsfree(mympd_state->lyrics_uslt_ext);
    sdsfree(mympd_state->lyrics_vorbis_uslt);
    sdsfree(mympd_state->lyrics_vorbis_sylt);
}

static const char *mympd_cols[]={"Pos", "Duration", "Type", "LastPlayed", "Filename", "Filetype", "Fileformat", "LastModified", 
    "Lyrics", "stickerPlayCount", "stickerSkipCount", "stickerLastPlayed", "stickerLastSkipped", "stickerLike", 0};

static bool is_mympd_col(sds token) {
    const char** ptr = mympd_cols;
    while (*ptr != 0) {
        if (strncmp(token, *ptr, sdslen(token)) == 0) {
            return true;
        }
        ++ptr;
    }
    return false;
}

sds json_to_cols(sds cols, char *str, size_t len, bool *error) {
    struct json_token t;
    int j = 0;
    *error = false;
    for (int i = 0; json_scanf_array_elem(str, len, ".params.cols", i, &t) > 0; i++) {
        sds token = sdscatlen(sdsempty(), t.ptr, t.len);
        if (mpd_tag_name_iparse(token) != MPD_TAG_UNKNOWN || is_mympd_col(token) == true) {
            if (j > 0) {
                cols = sdscatlen(cols, ",", 1);
            }
            cols = sdscatjson(cols, t.ptr, t.len);
            j++;
        }
        else {
            MYMPD_LOG_WARN("Unknown column: %s", token);
            *error = true;
        }
        sdsfree(token);
    }
    return cols;
}
