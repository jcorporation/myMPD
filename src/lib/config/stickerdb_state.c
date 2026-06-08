/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD state for the mympd_api thread
 */

#include "src/lib/config/stickerdb_state.h"

#include "src/lib/mem.h"
#include "src/lib/sds/sds_extras.h"

#include <string.h>

/**
 * Sets stickerdb state defaults
 * @param stickerdb pointer to stickerdb state
 * @param config pointer to static config
 */
void stickerdb_state_default(struct t_stickerdb_state *stickerdb, struct t_config *config) {
    stickerdb->config = config;
    stickerdb->mpd_state = NULL;
    stickerdb->conn_state = MPD_DISCONNECTED;
    stickerdb->conn = NULL;
    stickerdb->name = sdsnew("stickerdb");
}

/**
 * Frees the t_stickerdb_state struct
 * @param stickerdb pointer to struct
 */
void stickerdb_state_free(struct t_stickerdb_state *stickerdb) {
    FREE_SDS(stickerdb->name);
    FREE_PTR(stickerdb);
}
