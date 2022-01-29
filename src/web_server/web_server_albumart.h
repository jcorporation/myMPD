/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_ALBUMART_H
#define MYMPD_WEB_SERVER_ALBUMART_H

#include "../../dist/mongoose/mongoose.h"
#include "../../dist/sds/sds.h"
#include "../lib/mympd_configuration.h"
#include "web_server_utility.h"

#include <stdbool.h>

void webserver_albumart_send(struct mg_connection *nc, sds data, sds binary);
bool webserver_albumart_handler(struct mg_connection *nc, struct mg_http_message *hm, struct t_mg_user_data *mg_user_data, struct t_config *config, long long conn_id);
#endif
