/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief websocket handling
 */


#ifndef MYMPD_WEB_SERVER_WEBSOCKET_H
#define MYMPD_WEB_SERVER_WEBSOCKET_H

#include "dist/mongoose/mongoose.h"
#include "src/lib/api.h"

void send_ws_notify(struct mg_mgr *mgr, struct t_work_response *response);
void send_ws_notify_client(struct mg_mgr *mgr, struct t_work_response *response);

#endif
