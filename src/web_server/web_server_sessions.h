/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __WEB_SERVER_SESSIONS_H__
#define __WEB_SERVER_SESSIONS_H__

#include "../../dist/src/sds/sds.h"
#include "../list.h"

sds new_session(struct list *session_list);

#endif
