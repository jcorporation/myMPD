/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_CLIENT_SEARCH_H
#define MYMPD_MPD_CLIENT_SEARCH_H

#include "../lib/mympd_state.h"

sds mpd_client_search_response(struct t_mpd_state *mpd_state, sds buffer, long request_id,
        const char *expression, const char *sort, const bool sortdesc, const unsigned offset, const unsigned limit,
        const struct t_tags *tagcols, struct t_cache *sticker_cache, bool *result);
bool mpd_client_search_add_to_plist(struct t_mpd_state *mpd_state, const char *expression,
        const char *plist, unsigned to);
bool mpd_client_search_add_to_queue(struct t_mpd_state *mpd_state, const char *expression,
        unsigned to, enum mpd_position_whence whence);

sds escape_mpd_search_expression(sds buffer, const char *tag, const char *operator, const char *value);
#endif
