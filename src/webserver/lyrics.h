/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lyrics functions
 */

#ifndef MYMPD_WEB_SERVER_LYRICS_H
#define MYMPD_WEB_SERVER_LYRICS_H

#include "dist/mongoose/mongoose.h"
#include "src/webserver/mg_user_data.h"

bool webserver_lyrics_get(struct mg_connection *nc, unsigned request_id, sds body);
#endif
