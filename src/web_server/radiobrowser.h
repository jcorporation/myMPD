/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_RADIOBROWSER_H
#define MYMPD_WEB_SERVER_RADIOBROWSER_H

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/api.h"

void radiobrowser_api(struct mg_connection *nc, struct mg_connection *backend_nc,
    enum mympd_cmd_ids cmd_id, sds body, unsigned request_id);
#endif
