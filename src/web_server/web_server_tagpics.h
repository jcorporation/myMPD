/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __WEB_SERVER_TAGPICS_H__
#define __WEB_SERVER_TAGPICS_H__
bool handle_tagpics(struct mg_connection *nc, struct http_message *hm, t_mg_user_data *mg_user_data, t_config *config, int conn_id);
#endif
