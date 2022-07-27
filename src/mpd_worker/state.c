/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "state.h"

#include "../lib/mem.h"
#include "../lib/sds_extras.h"

#include <stdlib.h>

void *mpd_worker_state_free(struct t_mpd_worker_state *mpd_worker_state) {
    FREE_SDS(mpd_worker_state->smartpls_sort);
    FREE_SDS(mpd_worker_state->smartpls_prefix);
    //mpd state
    mpd_shared_state_free(mpd_worker_state->mpd_shared_state);
    partition_state_free(mpd_worker_state->partition_state);
    FREE_PTR(mpd_worker_state);
    return NULL;
}
