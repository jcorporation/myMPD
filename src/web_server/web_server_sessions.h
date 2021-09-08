/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_SESSIONS_H
#define MYMPD_WEB_SERVER_SESSIONS_H

#include "../../dist/src/sds/sds.h"
#include "../lib/list.h"

#include <stdbool.h>

sds new_session(struct t_list *session_list);
bool validate_session(struct t_list *session_list, const char *check_session);
bool remove_session(struct t_list *session_list, const char *session);

#endif
