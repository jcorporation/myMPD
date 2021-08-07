/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_ALBUMART_H
#define MYMPD_WEB_SERVER_ALBUMART_H

#include <stdbool.h>

#include "../../dist/src/mongoose/mongoose.h"
#include "../../dist/src/sds/sds.h"

#include "mympd_config_defs.h"
#include "web_server_utility.h"

void send_albumart(struct mg_connection *nc, sds data, sds binary);
bool handle_albumart(struct mg_connection *nc, struct mg_http_message *hm, struct t_mg_user_data *mg_user_data, struct t_config *config, long long conn_id);
#endif
