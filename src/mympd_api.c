/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <libgen.h>
#include <dirent.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <inttypes.h>

#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "../dist/src/rax/rax.h"
#include "sds_extras.h"
#include "../dist/src/frozen/frozen.h"
#include "api.h"
#include "log.h"
#include "list.h"
#include "tiny_queue.h"
#include "mympd_config_defs.h"
#include "utility.h"
#include "global.h"
#include "lua_mympd_state.h"
#include "mympd_state.h"
#include "mpd_client.h"
#include "covercache.h"
#include "mympd_api/mympd_api_utility.h"
#include "mympd_api/mympd_api_timer.h"
#include "mympd_api/mympd_api_settings.h"
#include "mympd_api/mympd_api_timer.h"
#include "mympd_api/mympd_api_timer_handlers.h"
#include "mympd_api/mympd_api_scripts.h"
#include "mympd_api/mympd_api_home.h"
#include "mpd_client/mpd_client_trigger.h"
#include "mpd_client/mpd_client_stats.h"
#include "mpd_shared.h"
#include "mpd_shared/mpd_shared_sticker.h"
#include "mpd_shared/mpd_shared_tags.h"
#include "mympd_api.h"

//public functions
void *mympd_api_loop(void *arg_config) {
    thread_logname = sdsreplace(thread_logname, "mympdapi");

    //create mympd_state struct
    struct t_mympd_state *mympd_state = (struct t_mympd_state *)malloc(sizeof(struct t_mympd_state));
    assert(mympd_state);
    mympd_state->config = (struct t_config *) arg_config;
    default_mympd_state(mympd_state);
    mpd_shared_default_mpd_state(mympd_state->mpd_state);

    //read myMPD states under config.varlibdir
    mympd_api_read_statefiles(mympd_state);

    //home icons
    mympd_api_read_home_list(mympd_state);

    //myMPD timer
    timerfile_read(mympd_state);
    
    //myMPD trigger
    triggerfile_read(mympd_state);

    //set timers
    if (mympd_state->config->covercache == true) {
        MYMPD_LOG_DEBUG("Setting timer action \"clear covercache\" to periodic each 7200s");
        add_timer(&mympd_state->timer_list, 60, 7200, timer_handler_covercache, 1, NULL, (void *)mympd_state->config);
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
    //saev states
    mympd_api_write_home_list(mympd_state);
    timerfile_save(mympd_state);
    mpd_client_last_played_list_save(mympd_state);
    triggerfile_save(mympd_state);
    //free anything
    free_trigerlist_arguments(mympd_state);
    free_mympd_state(mympd_state);
    sdsfree(thread_logname);
    return NULL;
}
