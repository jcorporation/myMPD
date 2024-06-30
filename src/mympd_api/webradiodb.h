/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_WEBRADIODB_H
#define MYMPD_API_WEBRADIODB_H

#include "dist/sds/sds.h"
#include "src/lib/webradio.h"

#include <stdbool.h>

sds mympd_api_webradiodb_radio_get_by_name(struct t_webradios *webradiodb, sds buffer, unsigned request_id, sds name);
sds mympd_api_webradiodb_radio_get_by_uri(struct t_webradios *webradiodb, sds buffer, unsigned request_id, sds uri);

#endif
