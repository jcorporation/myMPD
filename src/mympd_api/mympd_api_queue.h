/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_QUEUE_H
#define MYMPD_MPD_CLIENT_QUEUE_H

#include "../lib/mympd_state.h"

sds mpd_client_get_queue_state(struct t_mympd_state *mympd_state, sds buffer);
sds mpd_client_get_queue(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                         unsigned int offset, unsigned int limit, const struct t_tags *tagcols);
sds mpd_client_crop_queue(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, bool or_clear);
sds mpd_client_search_queue(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                            const char *mpdtagtype, const unsigned int offset, const unsigned int limit, 
                            const char *searchstr, const struct t_tags *tagcols);
bool mpd_client_queue_replace_with_song(struct t_mympd_state *mympd_state, const char *uri);
bool mpd_client_queue_replace_with_playlist(struct t_mympd_state *mympd_state, const char *plist);
bool mpd_client_queue_prio_set_highest(struct t_mympd_state *mympd_state, const unsigned trackid);
#endif
