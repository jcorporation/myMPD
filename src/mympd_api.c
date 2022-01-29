/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api.h"

#include "lib/api.h"
#include "lib/log.h"
#include "lib/mem.h"
#include "lib/mympd_configuration.h"
#include "lib/sds_extras.h"
#include "mpd_client/mpd_client_autoconf.h"
#include "mpd_client/mpd_client_idle.h"
#include "mpd_shared.h"
#include "mympd_api/mympd_api_home.h"
#include "mympd_api/mympd_api_settings.h"
#include "mympd_api/mympd_api_stats.h"
#include "mympd_api/mympd_api_timer.h"
#include "mympd_api/mympd_api_timer_handlers.h"
#include "mympd_api/mympd_api_trigger.h"
#include "mympd_api/mympd_api_utility.h"

#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>

void *mympd_api_loop(void *arg_config) {
    thread_logname = sds_replace(thread_logname, "mympdapi");
    prctl(PR_SET_NAME, thread_logname, 0, 0, 0);

    //create mympd_state struct and set defaults
    struct t_mympd_state *mympd_state = malloc_assert(sizeof(struct t_mympd_state));
    mympd_state->config = (struct t_config *) arg_config;
    mympd_state_default(mympd_state);

    if (mympd_state->config->first_startup == true) {
        MYMPD_LOG_NOTICE("Starting myMPD autoconfiguration");
        mpd_client_autoconf(mympd_state);
    }

    //read myMPD states
    mympd_api_settings_statefiles_read(mympd_state);
    //home icons
    mympd_api_home_file_read(mympd_state);
    //myMPD timer
    mympd_api_timer_file_read(mympd_state);
    //myMPD trigger
    mympd_api_trigger_file_read(mympd_state);
    //set timers
    if (mympd_state->covercache_keep_days > 0) {
        MYMPD_LOG_DEBUG("Setting timer action \"clear covercache\" to periodic each 7200s");
        mympd_api_timer_add(&mympd_state->timer_list, 60, 7200, timer_handler_covercache, 1, NULL, (void *)mympd_state);
    }
    //start trigger
    mympd_api_trigger_execute(mympd_state, TRIGGER_MYMPD_START);
    //thread loop
    while (s_signal_received == 0) {
        mpd_client_idle(mympd_state);
        if (mympd_state->mpd_state->conn_state == MPD_TOO_OLD) {
            break;
        }
        mympd_api_timer_check(&mympd_state->timer_list);
    }
    //stop trigger
    mympd_api_trigger_execute(mympd_state, TRIGGER_MYMPD_STOP);
    //disconnect from mpd
    mpd_shared_mpd_disconnect(mympd_state->mpd_state);
    //save states
    mympd_api_home_file_save(mympd_state);
    mympd_api_timer_file_save(mympd_state);
    mympd_api_stats_last_played_file_save(mympd_state);
    mympd_api_trigger_file_save(mympd_state);
    //free anything
    mympd_api_trigerlist_free_arguments(mympd_state);
    mympd_state_free(mympd_state);
    FREE_SDS(thread_logname);
    return NULL;
}
