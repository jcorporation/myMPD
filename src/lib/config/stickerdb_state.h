/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Central myMPD stickerdb state for the mympd_api thread
 */

#ifndef MYMPD_STICKERDB_STATE_H
#define MYMPD_STICKERDB_STATE_H

#include "dist/sds/sds.h"
#include "src/lib/config/config_def.h"
#include "src/lib/config/mympd_mpd_state.h"

/**
 * Holds stickerdb specific states
 */
struct t_stickerdb_state {
    struct t_config *config;               //!< pointer to static config
    struct t_mpd_state *mpd_state;         //!< pointer to shared mpd state
    //mpd connection
    struct mpd_connection *conn;           //!< mpd connection object from libmpdclient
    enum mympd_mpd_conn_states conn_state; //!< mpd connection state
    sds name;                              //!< name for logging
    bool *repopulate_pfds;                 //!< Pointer to repopulate state in mympd_state struct
};

/**
 * Public functions
 */
void stickerdb_state_default(struct t_stickerdb_state *stickerdb, struct t_config *config);
void stickerdb_state_free(struct t_stickerdb_state *stickerdb);

#endif
