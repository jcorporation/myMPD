/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_SHARED_FEATURES_H__
#define __MPD_SHARED_FEATURES_H__
bool mpd_shared_feat_mpd_searchwindow(t_mpd_state *mpd_state);
void mpd_shared_feat_tags(t_mpd_state *mpd_state);
bool mpd_shared_feat_advsearch(t_mpd_state *mpd_state);
#endif
