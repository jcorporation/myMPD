/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_SONG_H
#define MYMPD_API_SONG_H

#include "../lib/mympd_state.h"

sds mympd_api_fingerprint(struct t_mympd_state *mympd_state, sds buffer, sds method,
        long request_id, const char *uri);
sds mympd_api_songdetails(struct t_mympd_state *mympd_state, sds buffer, sds method,
        long request_id, const char *uri);
sds mympd_api_read_comments(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
        const char *uri);
#endif
