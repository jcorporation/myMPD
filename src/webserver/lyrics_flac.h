/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Read lyrics from vorbis comments
 */

#ifndef MYMPD_WEB_SERVER_LYRICS_FLAC_H
#define MYMPD_WEB_SERVER_LYRICS_FLAC_H

#include "src/lib/list/list.h"

void lyricsextract_flac(struct t_list *extracted, sds media_file, bool is_ogg, const char *comment_name, bool synced);

#endif
