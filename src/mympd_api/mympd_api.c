/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/mympd_api.h"

#include "src/lib/album_cache.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/sticker_cache.h"
#include "src/mpd_client/autoconf.h"
#include "src/mpd_client/connection.h"
#include "src/mpd_client/idle.h"
#include "src/mympd_api/home.h"
#include "src/mympd_api/settings.h"
#include "src/mympd_api/timer.h"
#include "src/mympd_api/timer_handlers.h"
#include "src/mympd_api/trigger.h"

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
        mpd_client_autoconf(mympd_state);
    }

    //read global states
    mympd_api_settings_statefiles_global_read(mympd_state);
    //read myMPD states for default partition
    mympd_api_settings_statefiles_partition_read(mympd_state->partition_state);
    //home icons
    mympd_api_home_file_read(&mympd_state->home_list, mympd_state->config->workdir);
    //timer
    mympd_api_timer_file_read(&mympd_state->timer_list, mympd_state->config->workdir);
    //trigger
    mympd_api_trigger_file_read(&mympd_state->trigger_list, mympd_state->config->workdir);
    //album cache
    album_cache_read(&mympd_state->mpd_state->album_cache, mympd_state->config->cachedir);
    //sticker cache
    sticker_cache_read(&mympd_state->mpd_state->sticker_cache, mympd_state->config->cachedir);
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
    MYMPD_LOG_DEBUG("Stopping mympd_api thread");

    //stop trigger
    mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_STOP, MPD_PARTITION_ALL);

    //disconnect from mpd
    mpd_client_disconnect_all(mympd_state, MPD_DISCONNECT_INSTANT);

    //save and free states
    mympd_state_save(mympd_state, true);

    FREE_SDS(thread_logname);
    return NULL;
}
