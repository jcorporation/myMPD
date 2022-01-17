/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_SHARED_SEARCH_H
#define MYMPD_MPD_SHARED_SEARCH_H

#include "../mpd_shared.h"

sds mpd_shared_search(struct t_mpd_state *mympd_state, sds buffer, sds method, long request_id,
                      const char *searchstr, const char *searchtag, const char *plist,
                      const unsigned offset, unsigned limit, const struct t_tags *tagcols,
                      rax *sticker_cache, bool *result);

sds mpd_shared_search_adv(struct t_mpd_state *mympd_state, sds buffer, sds method, long request_id,
                          const char *expression, const char *sort, const bool sortdesc,
                          const char *plist, unsigned to, unsigned whence,
                          const unsigned offset, unsigned limit, const struct t_tags *tagcols,
                          rax *sticker_cache, bool *result);

sds escape_mpd_search_expression(sds buffer, const char *tag, const char *operator, const char *value);
#endif
