/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "mympd_api.h"

#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"
#include "../mpd_client/autoconf.h"
#include "../mpd_client/connection.h"
#include "../mpd_client/idle.h"
#include "home.h"
#include "settings.h"
#include "src/lib/mympd_state.h"
#include "timer.h"
#include "timer_handlers.h"
#include "trigger.h"

#include <sys/prctl.h>

/**
 * This is the main function for the mympd_api thread
 * @param arg_config void pointer to t_config struct
 */
void *mympd_api_loop(void *arg_config) {
    thread_logname = sds_replace(thread_logname, "mympdapi");
    prctl(PR_SET_NAME, thread_logname, 0, 0, 0);

    //create initial mympd_state struct and set defaults
    struct t_mympd_state *mympd_state = malloc_assert(sizeof(struct t_mympd_state));
    mympd_state_default(mympd_state, (struct t_config *)arg_config);

    //start autoconfiguration for first startup
    if (mympd_state->config->first_startup == true) {
        MYMPD_LOG_NOTICE("Starting myMPD autoconfiguration");
        mpd_client_autoconf(mympd_state);
    }

    //read myMPD global states
    mympd_api_settings_statefiles_global_read(mympd_state);
    //read myMPD states for default partition
    mympd_api_settings_statefiles_partition_read(mympd_state->partition_state);
    //home icons
    mympd_api_home_file_read(&mympd_state->home_list, mympd_state->config->workdir);
    //myMPD timer
    mympd_api_timer_file_read(&mympd_state->timer_list, mympd_state->config->workdir);
    //myMPD trigger
    mympd_api_trigger_file_read(&mympd_state->trigger_list, mympd_state->config->workdir);

    //set timers
    if (mympd_state->config->covercache_keep_days > 0) {
        MYMPD_LOG_DEBUG("Adding timer for \"crop covercache\" to execute periodic each day");
        mympd_api_timer_add(&mympd_state->timer_list, COVERCACHE_CLEANUP_OFFSET, COVERCACHE_CLEANUP_INTERVAL,
            timer_handler_by_id, TIMER_ID_COVERCACHE_CROP, NULL);
    }

    //start trigger
    mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_START, MPD_PARTITION_ALL);

    //thread loop
    while (s_signal_received == 0) {
        mpd_client_idle(mympd_state);
        mympd_api_timer_check(&mympd_state->timer_list);
    }

    //stop trigger
    mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_STOP, MPD_PARTITION_ALL);

    //disconnect from mpd
    mpd_client_disconnect_all(mympd_state, MPD_DISCONNECT_INSTANT);

    //save states
    mympd_state_save(mympd_state);

    //free anything
    mympd_state_free(mympd_state);
    FREE_SDS(thread_logname);
    return NULL;
}
