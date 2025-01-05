/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief HTTP lua script handler
 */

#ifndef MYMPD_WEB_SERVER_SCRIPTS_H
#define MYMPD_WEB_SERVER_SCRIPTS_H

#include "dist/mongoose/mongoose.h"
#include "src/lib/config_def.h"

#include <stdbool.h>

bool script_execute_http(struct mg_connection *nc, struct mg_http_message *hm, struct t_config *config);

#endif
