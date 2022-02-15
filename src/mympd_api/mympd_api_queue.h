/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_QUEUE_H
#define MYMPD_API_QUEUE_H

#include "../lib/mympd_state.h"

bool mympd_api_queue_play_newly_inserted(struct t_mympd_state *mympd_state);
sds mympd_api_queue_status(struct t_mympd_state *mympd_state, sds buffer);
sds mympd_api_queue_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                         long offset, long limit, const struct t_tags *tagcols);
sds mympd_api_queue_crop(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, bool or_clear);
sds mympd_api_queue_search(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                            const char *mpdtagtype, const long offset, const long limit,
                            const char *searchstr, const struct t_tags *tagcols);
sds mympd_api_queue_search_adv(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                            sds expression, sds sort, bool sortdesc, unsigned offset, unsigned limit,
                            const struct t_tags *tagcols);
bool mympd_api_queue_replace_with_song(struct t_mympd_state *mympd_state, const char *uri);
bool mympd_api_queue_replace_with_playlist(struct t_mympd_state *mympd_state, const char *plist);
bool mympd_api_queue_prio_set(struct t_mympd_state *mympd_state, const unsigned trackid, const unsigned priority);
bool mympd_api_queue_prio_set_highest(struct t_mympd_state *mympd_state, const unsigned trackid);
#endif
