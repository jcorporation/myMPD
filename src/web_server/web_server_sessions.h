/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_SESSIONS_H
#define MYMPD_WEB_SERVER_SESSIONS_H

#include "../../dist/mongoose/mongoose.h"
#include "../../dist/sds/sds.h"
#include "../lib/api.h"
#include "../lib/list.h"
#include "web_server_utility.h"

#include <stdbool.h>

void webserver_session_api(struct mg_connection *nc, enum mympd_cmd_ids cmd_id, sds body, int id, sds session, struct t_mg_user_data *mg_user_data);
sds webserver_session_new(struct t_list *session_list);
bool webserver_session_validate(struct t_list *session_list, const char *check_session);
bool webserver_session_remove(struct t_list *session_list, const char *session);

#endif
