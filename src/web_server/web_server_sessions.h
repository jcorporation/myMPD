/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_SESSIONS_H
#define MYMPD_WEB_SERVER_SESSIONS_H

#include "../../dist/sds/sds.h"
#include "../lib/list.h"

#include <stdbool.h>

sds webserver_session_new(struct t_list *session_list);
bool webserver_session_validate(struct t_list *session_list, const char *check_session);
bool webserver_session_remove(struct t_list *session_list, const char *session);

#endif
