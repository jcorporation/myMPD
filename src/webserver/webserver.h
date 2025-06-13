/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver implementation
 */

#ifndef MYMPD_WEB_SERVER_H
#define MYMPD_WEB_SERVER_H

#include "src/lib/config_def.h"
#include "src/webserver/mg_user_data.h"

#include <stdbool.h>

void *webserver_loop(void *arg_mgr);
bool webserver_init(struct mg_mgr *mgr, struct t_config *config, struct t_mg_user_data *mg_user_data);
void webserver_free(struct mg_mgr *mgr);

#endif
