/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_H
#define MYMPD_WEB_SERVER_H

#include "src/lib/config_def.h"
#include "src/web_server/utility.h"

#include <stdbool.h>

void *web_server_loop(void *arg_mgr);
bool web_server_init(struct mg_mgr *mgr, struct t_config *config, struct t_mg_user_data *mg_user_data);
void *web_server_free(struct mg_mgr *mgr);
#endif
