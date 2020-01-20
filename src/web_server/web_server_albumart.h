/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __WEB_SERVER_ALBUMART_H__
#define __WEB_SERVER_ALBUMART_H__
void send_albumart(struct mg_connection *nc, sds data, sds binary);
bool handle_albumart(struct mg_connection *nc, struct http_message *hm, t_mg_user_data *mg_user_data, t_config *config, int conn_id);
#endif
