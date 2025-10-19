/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Jukebox library
 */

#ifndef MYMPD_LIB_JUKEBOX_H
#define MYMPD_LIB_JUKEBOX_H

#include <stdbool.h>

struct t_partition_state;

/**
 * Jukebox state
 */
enum jukebox_modes {
    JUKEBOX_OFF,        //!< jukebox is disabled
    JUKEBOX_ADD_SONG,   //!< jukebox adds single songs
    JUKEBOX_ADD_ALBUM,  //!< jukebox adds whole albums
    JUKEBOX_SCRIPT,     //!< jukebox queue is filled by a script
    JUKEBOX_UNKNOWN     //!< jukebox mode is unknown
};

enum jukebox_modes jukebox_mode_parse(const char *str);
const char *jukebox_mode_lookup(enum jukebox_modes mode);

bool jukebox_file_save(struct t_partition_state *partition_state);
bool jukebox_file_read(struct t_partition_state *partition_state);

#endif
