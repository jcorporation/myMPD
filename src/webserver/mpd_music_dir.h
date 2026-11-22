/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD music directory functions
 */

#ifndef MYMPD_WEB_SERVER_MPD_MUSIC_DIR_H
#define MYMPD_WEB_SERVER_MPD_MUSIC_DIR_H

#include "dist/mongoose/mongoose.h"

#include <stdbool.h>

bool request_handler_mpd_music_directory(struct mg_connection *nc, struct mg_http_message *hm);

#endif
