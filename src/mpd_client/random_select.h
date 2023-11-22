/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_RANDOM_ADD_H
#define MYMPD_RANDOM_ADD_H

#include "src/lib/mympd_state.h"

/**
 * Jukebox constraints for song/album selection
 */
struct t_random_add_constraints {
    const char *filter_include;  //!< mpd search filter to include songs / albums
    const char *filter_exclude;  //!< mpd search filter to exclude songs / albums
    enum mpd_tag_type uniq_tag;  //!< single tag for the jukebox uniq constraint
    unsigned last_played;        //!< only add songs with last_played state older than seconds from now
    bool ignore_hated;           //!< ignores hated songs for the jukebox mode
    unsigned min_song_duration;  //!< minimum song duration
    unsigned max_song_duration;  //!< maximum song duration
};

unsigned random_select_albums(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        struct t_cache *album_cache, unsigned add_albums, struct t_list *queue_list, struct t_list *add_list,
        struct t_random_add_constraints *constraints);
unsigned random_select_songs(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        unsigned add_songs, const char *playlist, struct t_list *queue_list, struct t_list *add_list,
        struct t_random_add_constraints *constraints);
#endif
