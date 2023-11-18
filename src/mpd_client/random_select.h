/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_RANDOM_ADD_H
#define MYMPD_RANDOM_ADD_H

#include "src/lib/mympd_state.h"

struct t_random_add_constraints {
    const char *filter_include;
    const char *filter_exclude;
    enum mpd_tag_type uniq_tag;
    long last_played;
    bool ignore_hated;
    unsigned min_song_duration;
    unsigned max_song_duration;
};

unsigned random_select_albums(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        struct t_cache *album_cache, unsigned add_albums, struct t_list *queue_list, struct t_list *add_list,
        struct t_random_add_constraints *constraints);
unsigned random_select_songs(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        unsigned add_songs, const char *playlist, struct t_list *queue_list, struct t_list *add_list,
        struct t_random_add_constraints *constraints);
#endif
