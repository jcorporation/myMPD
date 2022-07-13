/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_JUKEBOX_H
#define MYMPD_JUKEBOX_H

#include "../lib/mympd_state.h"
enum jukebox_modes mpd_client_parse_jukebox_mode(const char *str);
const char *mpd_client_lookup_jukebox_mode(enum jukebox_modes mode);
void mpd_client_clear_jukebox(struct t_list *list);
bool mpd_client_rm_jukebox_entry(struct t_list *list, long pos);
sds mpd_client_get_jukebox_list(struct t_mympd_state *mympd_state, sds buffer, sds method,
        long request_id, const long offset, const long limit, sds searchstr,
        const struct t_tags *tagcols);
bool mpd_client_jukebox(struct t_mympd_state *mympd_state);
bool mpd_client_jukebox_add_to_queue(struct t_mympd_state *mympd_state, long add_songs,
        enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
#endif
