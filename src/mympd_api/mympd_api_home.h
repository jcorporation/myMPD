/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_API_HOME_H
#define __MYMPD_API_HOME_H
void mympd_api_read_home_list(t_config *config, t_mympd_state *mympd_state);
sds mympd_api_put_home_list(t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
#endif
