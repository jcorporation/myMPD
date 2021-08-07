/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __JUKEBOX_H__
#define __JUKEBOX_H__

#include "../mympd_state.h"

bool mpd_client_rm_jukebox_entry(struct t_mympd_state *mympd_state, unsigned pos);
sds mpd_client_put_jukebox_list(struct t_mympd_state *mympd_state, sds buffer, sds method,
                                long request_id, const unsigned int offset, const unsigned int limit, 
                                const struct t_tags *tagcols);
bool mpd_client_jukebox(struct t_mympd_state *mympd_state, unsigned attempt);
bool mpd_client_jukebox_add_to_queue(struct t_mympd_state *mympd_state, unsigned add_songs,
                                     enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
#endif
