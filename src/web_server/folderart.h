/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_FOLDERART_H
#define MYMPD_WEB_SERVER_FOLDERART_H

#include "dist/mongoose/mongoose.h"
#include "src/web_server/utility.h"

#include <stdbool.h>

bool request_handler_folderart(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data);
#endif
