/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD worker state functions
 */

#include "compile_time.h"
#include "src/mympd_worker/state.h"

#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"

/**
 * Frees the mympd_worker_state struct
 * @param mympd_worker_state pointer to the t_mympd_worker_state struct
 */
void mympd_worker_state_free(struct t_mympd_worker_state *mympd_worker_state) {
    FREE_SDS(mympd_worker_state->smartpls_sort);
    FREE_SDS(mympd_worker_state->smartpls_prefix);
    if (mympd_worker_state->mpd_state != NULL) {
        mympd_mpd_state_free(mympd_worker_state->mpd_state);
    }
    if (mympd_worker_state->partition_state != NULL) {
        partition_state_free(mympd_worker_state->partition_state);
    }
    if (mympd_worker_state->stickerdb != NULL) {
        mympd_mpd_state_free(mympd_worker_state->stickerdb->mpd_state);
        stickerdb_state_free(mympd_worker_state->stickerdb);
    }
    FREE_PTR(mympd_worker_state);
}
