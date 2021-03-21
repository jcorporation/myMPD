/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __MPD_WORKER_UTILITY_H__
#define __MPD_WORKER_UTILITY_H__

typedef struct t_mpd_worker_state {
    bool smartpls;
    bool feat_smartpls;
    bool stickers;
    sds smartpls_sort;
    sds smartpls_prefix;
    sds generate_pls_tags;
    t_tags generate_pls_tag_types;
    //mpd state
    struct t_mpd_state *mpd_state;
} t_mpd_worker_state;

void free_mpd_worker_state(t_mpd_worker_state *mpd_worker_state);
void default_mpd_worker_state(t_mpd_worker_state *mpd_worker_state);
void mpd_worker_features(t_mpd_worker_state *mpd_worker_state);
#endif
