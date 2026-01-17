/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Read lyrics from ID3 tags
 */

#ifndef MYMPD_WEB_SERVER_LYRICS_ID3_H
#define MYMPD_WEB_SERVER_LYRICS_ID3_H

#include "src/lib/list/list.h"

void lyricsextract_unsynced_id3(struct t_list *extracted, sds media_file);
void lyricsextract_synced_id3(struct t_list *extracted, sds media_file);

#endif
