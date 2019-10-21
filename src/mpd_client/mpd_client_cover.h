/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __COVER_H__
#define __COVER_H__
sds mpd_client_get_cover(t_config *config, t_mpd_state *mpd_state, const char *uri, sds cover);
#endif
