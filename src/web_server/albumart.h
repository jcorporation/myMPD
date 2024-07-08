/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Albumart functions
 */

#ifndef MYMPD_WEB_SERVER_ALBUMART_H
#define MYMPD_WEB_SERVER_ALBUMART_H

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/web_server/utility.h"

/**
 * Albumart types
 */
enum albumart_sizes {
    ALBUMART_THUMBNAIL,
    ALBUMART_FULL
};

void webserver_send_albumart_redirect(struct mg_connection *nc, sds data);
void webserver_send_albumart(struct mg_connection *nc, sds data, sds binary);
void request_handler_albumart_by_album_id(struct mg_http_message *hm, unsigned long conn_id, enum albumart_sizes size);
bool request_handler_albumart_by_uri(struct mg_connection *nc, struct mg_http_message *hm,
    struct t_mg_user_data *mg_user_data, unsigned long conn_id, enum albumart_sizes size);
#endif
