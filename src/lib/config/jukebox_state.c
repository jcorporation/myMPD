/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD state for the mympd_api thread
 */

#include "compile_time.h"
#include "src/lib/config/jukebox_state.h"

#include "src/lib/sds/sds_extras.h"

#include <string.h>

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
    jukebox_state->autostart = true;
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
