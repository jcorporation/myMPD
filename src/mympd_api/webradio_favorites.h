/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_WEBRADIO_FAVORITES_H
#define MYMPD_API_WEBRADIO_FAVORITES_H

#include "dist/sds/sds.h"
#include "src/lib/webradio.h"

#include <stdbool.h>

sds mympd_api_webradio_favorite_get_by_name(struct t_webradios *webradio_favorites, sds buffer, unsigned request_id, sds name);
sds mympd_api_webradio_favorite_get_by_uri(struct t_webradios *webradio_favorites, sds buffer, unsigned request_id, sds uri);
sds mympd_api_webradio_favorites_search(struct t_webradios *webradio_favorites, sds buffer, unsigned request_id,
        sds expression, unsigned offset, unsigned limit);
bool mympd_api_webradio_favorite_save(struct t_webradios *webradio_favorites, struct t_webradio_data *webradio, sds old_name);
bool mympd_api_webradio_favorite_delete(struct t_webradios *webradio_favorites, struct t_list *ids);

#endif
