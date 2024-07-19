/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver implementation
 */

#ifndef MYMPD_WEB_SERVER_H
#define MYMPD_WEB_SERVER_H

#include "src/lib/config_def.h"
#include "src/web_server/utility.h"

#include <stdbool.h>

void *web_server_loop(void *arg_mgr);
bool web_server_init(struct mg_mgr *mgr, struct t_config *config, struct t_mg_user_data *mg_user_data);
bool webserver_read_certs(struct t_mg_user_data *mg_user_data, struct t_config *config);
void *web_server_free(struct mg_mgr *mgr);
#endif
