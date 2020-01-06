/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__
void *web_server_loop(void *arg_mgr);
bool web_server_init(void *arg_mgr, t_config *config, t_mg_user_data *mg_user_data);
void web_server_free(void *arg_mgr);
#endif
