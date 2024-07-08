/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD webradio favorites API
 */

#ifndef MYMPD_API_WEBRADIO_FAVORITES_H
#define MYMPD_API_WEBRADIO_FAVORITES_H

#include "dist/sds/sds.h"
#include "src/lib/webradio.h"

#include <stdbool.h>

bool mympd_api_webradio_favorite_save(struct t_webradios *webradio_favorites, struct t_webradio_data *webradio, sds old_name);
void mympd_api_webradio_favorite_delete(struct t_webradios *webradio_favorites, struct t_list *names);

#endif
