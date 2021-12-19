/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_WEB_SERVER_PROXY_H
#define MYMPD_WEB_SERVER_PROXY_H

#include "../../dist/mongoose/mongoose.h"
#include "../../dist/sds/sds.h"
#include "../lib/api.h"

struct mg_connection *create_backend_connection(struct mg_connection *nc, struct mg_connection *backend_nc,
        sds uri, mg_event_handler_t fn);
void forward_backend_to_frontend(struct mg_connection *nc, int ev, void *ev_data, void *fn_data);
#endif
