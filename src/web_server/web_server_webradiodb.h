/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_WEBRADIODB_H
#define MYMPD_WEB_SERVER_WEBRADIODB_H

#include "../../dist/mongoose/mongoose.h"
#include "../../dist/sds/sds.h"
#include "../lib/api.h"

void webradiodb_api(struct mg_connection *nc, struct mg_connection *backend_nc,
    enum mympd_cmd_ids cmd_id, sds body, int id);
#endif
