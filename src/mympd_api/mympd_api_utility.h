/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_UTILITY_H
#define MYMPD_API_UTILITY_H

#include "../lib/mympd_state.h"

void mympd_state_default(struct t_mympd_state *mympd_state);
void mympd_state_free(struct t_mympd_state *mympd_state);
bool is_smartpls(const char *workdir, sds playlist);
bool is_streamuri(const char *uri);
sds get_extra_files(struct t_mympd_state *mympd_state, sds buffer, const char *uri, bool is_dirname);
bool mympd_api_set_binarylimit(struct t_mympd_state *mympd_state);
unsigned mympd_api_get_elapsed_seconds(struct mpd_status *status);
sds resolv_mympd_uri(sds uri, struct t_mympd_state *mympd_state);
#endif
