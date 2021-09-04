/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_worker_utility.h"

#include "../lib/sds_extras.h"
#include "../mpd_shared.h"

#include <stdlib.h>

void free_mpd_worker_state(struct t_mpd_worker_state *mpd_worker_state) {
    FREE_SDS(mpd_worker_state->smartpls_sort);
    FREE_SDS(mpd_worker_state->smartpls_prefix);
    //mpd state
    mpd_shared_free_mpd_state(mpd_worker_state->mpd_state);
    free(mpd_worker_state);
}
