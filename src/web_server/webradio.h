/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webradio functions
 */

#ifndef MYMPD_WEB_SERVER_WEBRADIO_H
#define MYMPD_WEB_SERVER_WEBRADIO_H

#include "dist/sds/sds.h"
#include "src/lib/webradio.h"

sds webserver_webradio_get_cover_uri(struct t_webradios *webradio_favorites, struct t_webradios *webradiodb,
        sds buffer, sds uri);
sds webserver_webradio_get_extm3u(struct t_webradios *webradio_favorites, struct t_webradios *webradiodb,
        sds buffer, sds uri);

#endif
