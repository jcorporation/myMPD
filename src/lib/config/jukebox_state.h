/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD state for the mympd_api thread
 */

#ifndef MYMPD_JUKEBOX_STATE_H
#define MYMPD_JUKEBOX_STATE_H

#include "dist/sds/sds.h"
#include "src/lib/fields.h"
#include "src/lib/jukebox.h"
#include "src/lib/list/list.h"

/**
 * Holds the jukebox states for a partition
 */
struct t_jukebox_state {
    enum jukebox_modes mode;       //!< the jukebox mode
    sds playlist;                  //!< playlist from which the jukebox queue is generated
    unsigned queue_length;         //!< how many songs should the mpd queue have
    unsigned last_played;          //!< only add songs with last_played state older than seconds from now
    struct t_mympd_mpd_tags uniq_tag;      //!< single tag for the jukebox uniq constraint
    struct t_list *queue;          //!< the jukebox queue itself
    bool ignore_hated;             //!< ignores hated songs for the jukebox mode
    sds filter_include;            //!< mpd search filter to include songs / albums
    sds filter_exclude;            //!< mpd search filter to exclude songs / albums
    unsigned min_song_duration;    //!< minimum song duration
    unsigned max_song_duration;    //!< maximum song duration
    bool filling;                  //!< indication flag for filling jukebox thread
    sds last_error;                //!< last jukebox error message
    bool autostart;                //!< Run jukebox after MPD connection is established
};

void jukebox_state_default(struct t_jukebox_state *jukebox_state);
void jukebox_state_copy(struct t_jukebox_state *src, struct t_jukebox_state *dst);
void jukebox_state_free(struct t_jukebox_state *jukebox_state);

#endif
