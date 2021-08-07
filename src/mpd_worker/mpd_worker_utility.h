/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_WORKER_UTILITY_H
#define MYMPD_MPD_WORKER_UTILITY_H

#include "../global.h"
#include "../mympd_state.h"

struct t_mpd_worker_state {
    bool smartpls;
    sds smartpls_sort;
    sds smartpls_prefix;
    struct t_tags smartpls_generate_tag_types;
    //mpd state
    struct t_mpd_state *mpd_state;
    struct t_config *config;
    t_work_request *request;
};

void free_mpd_worker_state(struct t_mpd_worker_state *mpd_worker_state);
void default_mpd_worker_state(struct t_mpd_worker_state *mpd_worker_state);
void mpd_worker_features(struct t_mpd_worker_state *mpd_worker_state);
#endif
