/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief tagart functions
 */

#ifndef MYMPD_WEB_SERVER_TAGART_H
#define MYMPD_WEB_SERVER_TAGART_H

#include "dist/mongoose/mongoose.h"
#include "src/webserver/utility.h"

#include <stdbool.h>

bool request_handler_tagart(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data);
#endif
