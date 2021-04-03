/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_SHARED_FEATURES_H__
#define __MPD_SHARED_FEATURES_H__
void mpd_shared_feat_tags(struct t_mpd_state *mpd_state);
bool mpd_shared_feat_advsearch(struct t_mpd_state *mpd_state);
#endif
