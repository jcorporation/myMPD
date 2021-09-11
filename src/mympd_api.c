/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
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
#include "mpd_client/mpd_client_loop.h"
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
    thread_logname = sdsreplace(thread_logname, "mympdapi");
    prctl(PR_SET_NAME, thread_logname, 0, 0, 0);

    //create mympd_state struct and set defaults
    struct t_mympd_state *mympd_state = (struct t_mympd_state *)malloc_assert(sizeof(struct t_mympd_state));
    mympd_state->config = (struct t_config *) arg_config;
    default_mympd_state(mympd_state);

    if (mympd_state->config->first_startup == true) {
        MYMPD_LOG_NOTICE("Starting myMPD autoconfiguration");
        mpd_client_autoconf(mympd_state);
    }

    //read myMPD states
    mympd_api_read_statefiles(mympd_state);

    //home icons
    mympd_api_read_home_list(mympd_state);

    //myMPD timer
    timerfile_read(mympd_state);
    
    //myMPD trigger
    triggerfile_read(mympd_state);

    //set timers
    if (mympd_state->covercache_keep_days > 0) {
        MYMPD_LOG_DEBUG("Setting timer action \"clear covercache\" to periodic each 7200s");
        add_timer(&mympd_state->timer_list, 60, 7200, timer_handler_covercache, 1, NULL, (void *)mympd_state);
    }

    while (s_signal_received == 0) {
        mpd_client_idle(mympd_state);
        if (mympd_state->mpd_state->conn_state == MPD_TOO_OLD) {
            break;
        }
        check_timer(&mympd_state->timer_list);
    }

    //cleanup trigger
    trigger_execute(mympd_state, TRIGGER_MYMPD_STOP);
    //disconnect from mpd
    mpd_shared_mpd_disconnect(mympd_state->mpd_state);
    //save states
    mympd_api_write_home_list(mympd_state);
    timerfile_save(mympd_state);
    mpd_client_last_played_list_save(mympd_state);
    triggerfile_save(mympd_state);
    //free anything
    free_trigerlist_arguments(mympd_state);
    free_mympd_state(mympd_state);
    FREE_SDS(thread_logname);
    return NULL;
}
