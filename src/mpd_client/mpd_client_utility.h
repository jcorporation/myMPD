/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_CLIENT_UTILITY_H__
#define __MPD_CLIENT_UTILITY_H__

#include "../mympd_state.h"
#include <stdbool.h>

void json_to_tags(const char *str, int len, void *user_data);
bool is_smartpls(struct t_mympd_state *mympd_state, const char *plpath);
sds put_extra_files(struct t_mympd_state *mympd_state, sds buffer, const char *uri, bool is_dirname);
bool mpd_client_set_binarylimit(struct t_mympd_state *mympd_state);
unsigned mpd_client_get_elapsed_seconds(struct mpd_status *status);
#endif
