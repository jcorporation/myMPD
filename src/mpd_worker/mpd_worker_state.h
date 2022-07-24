/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_MPD_WORKER_UTILITY_H
#define MYMPD_MPD_WORKER_UTILITY_H

#include "../lib/api.h"
#include "../lib/mympd_state.h"

struct t_mpd_worker_state {
    bool smartpls;
    sds smartpls_sort;
    sds smartpls_prefix;
    struct t_tags smartpls_generate_tag_types;
    //mpd state
    struct t_partition_state *partition_state;
    struct t_mpd_shared_state *mpd_shared_state;
    struct t_config *config;
    struct t_work_request *request;
};

void *mpd_worker_state_free(struct t_mpd_worker_state *mpd_worker_state);
#endif
