/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lyrics
 */

#ifndef MYMPD_LYRICS_H
#define MYMPD_LYRICS_H

/**
 * Lyrics settings
 */
struct t_lyrics {
    sds uslt_ext;     //!< file extension for unsynced lyrics
    sds sylt_ext;     //!< file extension for synced lyrics
    sds vorbis_uslt;  //!< vorbis comment for unsynced lyrics
    sds vorbis_sylt;  //!< vorbis comment for synced lyrics
};

#endif
