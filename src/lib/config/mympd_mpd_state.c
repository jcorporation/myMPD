/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD state for the mympd_api thread
 */

#include "compile_time.h"
#include "src/lib/config/mympd_mpd_state.h"


#include "src/lib/mem.h"
#include "src/lib/sds/sds_extras.h"

#include <string.h>

/**
 * Sets mpd_state defaults.
 * @param mpd_state pointer to mpd_state
 * @param config pointer to static config
 */
void mympd_mpd_state_default(struct t_mpd_state *mpd_state, struct t_config *config) {
    mpd_state->config = config;
    mpd_state->mpd_keepalive = MYMPD_MPD_KEEPALIVE;
    mpd_state->mpd_timeout = MYMPD_MPD_TIMEOUT;
    mpd_state->mpd_host = sdsnew(MYMPD_MPD_HOST);
    mpd_state->mpd_port = MYMPD_MPD_PORT;
    mpd_state->mpd_pass = sdsnew(MYMPD_MPD_PASS);
    mpd_state->mpd_binarylimit = MYMPD_MPD_BINARYLIMIT;
    mpd_state->mpd_stringnormalization = MYMPD_MPD_STRINGNORMALIZATION;
    mpd_state->music_directory_value = sdsempty();
    mpd_state->playlist_directory_value = sdsempty();
    mpd_state->tag_list = sdsnew(MYMPD_MPD_TAG_LIST);
    mympd_mpd_tags_reset(&mpd_state->tags_mympd);
    mympd_mpd_tags_reset(&mpd_state->tags_mpd);
    mympd_mpd_tags_reset(&mpd_state->tags_search);
    mympd_mpd_tags_reset(&mpd_state->tags_browse);
    mympd_mpd_tags_reset(&mpd_state->tags_album);
    mpd_state->tag_albumartist = MPD_TAG_ALBUM_ARTIST;
    //features
    mympd_mpd_state_features_default(&mpd_state->feat);
    list_init(&mpd_state->sticker_types);
}

/**
 * Copy mpd state
 * @param src source
 * @param dst destination
 */
void mympd_mpd_state_copy(struct t_mpd_state *src, struct t_mpd_state *dst) {
    dst->config = src->config;
    dst->mpd_keepalive = src->mpd_keepalive;
    dst->mpd_timeout = src->mpd_timeout;
    dst->mpd_host = sdsdup(src->mpd_host);
    dst->mpd_port = src->mpd_port;
    dst->mpd_pass = sdsdup(src->mpd_pass);
    dst->mpd_binarylimit = src->mpd_binarylimit;
    dst->mpd_stringnormalization = src->mpd_stringnormalization;
    dst->music_directory_value = sdsdup(src->music_directory_value);
    dst->playlist_directory_value = sdsdup(src->playlist_directory_value);
    dst->tag_list = sdsdup( src->tag_list);
    mympd_mpd_tags_clone(&src->tags_mympd, &dst->tags_mympd);
    mympd_mpd_tags_clone(&src->tags_mpd, &dst->tags_mpd);
    mympd_mpd_tags_clone(&src->tags_search, &dst->tags_search);
    mympd_mpd_tags_clone(&src->tags_browse, &dst->tags_browse);
    mympd_mpd_tags_clone(&src->tags_album, &dst->tags_album);
    dst->tag_albumartist = src->tag_albumartist;
    mympd_mpd_state_features_copy(&src->feat, &dst->feat);
    list_init(&dst->sticker_types);
    list_append(&dst->sticker_types, &src->sticker_types);
}

/**
 * Sets all feature states to default
 * @param feat pointer to mpd feature struct
 */
void mympd_mpd_state_features_default(struct t_mpd_features *feat) {
    feat->mpd_0_25_0 = false;
    feat->mpd_0_24_0 = false;
    feat->stickers = false;
    feat->playlists = false;
    feat->tags = false;
    feat->fingerprint = false;
    feat->mount = false;
    feat->neighbor = false;
    feat->advqueue = false;
    feat->consume_oneshot = false;
    feat->playlist_dir_auto = false;
    feat->starts_with = false;
    feat->pcre = true;
    feat->db_added = false;
    feat->advsticker = false;
    feat->listplaylist_range = false;
}

/**
 * Copy mpd state feature flags
 * @param src source
 * @param dst destination
 */
void mympd_mpd_state_features_copy(struct t_mpd_features *src, struct t_mpd_features *dst) {
    memcpy((void *)dst, (void *)src, sizeof(struct t_mpd_features));
}

/**
 * Frees the t_mpd_state struct
 * @param mpd_state Pointer to mpd_state
 */
void mympd_mpd_state_free(struct t_mpd_state *mpd_state) {
    FREE_SDS(mpd_state->mpd_host);
    FREE_SDS(mpd_state->mpd_pass);
    FREE_SDS(mpd_state->tag_list);
    FREE_SDS(mpd_state->music_directory_value);
    FREE_SDS(mpd_state->playlist_directory_value);
    list_clear(&mpd_state->sticker_types);
    //struct itself
    FREE_PTR(mpd_state);
}
