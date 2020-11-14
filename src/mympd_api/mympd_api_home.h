/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MYMPD_API_HOME_H
#define __MYMPD_API_HOME_H
bool mympd_api_move_home_icon(t_mympd_state *mympd_state, unsigned int from, unsigned int to);
bool mympd_api_rm_home_icon(t_mympd_state *mympd_state, unsigned int pos);
bool mympd_api_save_home_icon(t_mympd_state *mympd_state, bool replace, unsigned int oldpos,
    const char *name, const char *ligature, const char *bgcolor, const char *image,
    const char *cmd, struct list *option_list);
void mympd_api_read_home_list(t_config *config, t_mympd_state *mympd_state);
bool mympd_api_write_home_list(t_config *config, t_mympd_state *mympd_state);
sds mympd_api_put_home_list(t_mympd_state *mympd_state, sds buffer, sds method, long request_id);
sds mympd_api_get_home_icon(t_mympd_state *mympd_state, sds buffer, sds method, long request_id, unsigned pos);
sds mympd_api_put_home_picture_list(t_config *config, sds buffer, sds method, long request_id);
#endif
