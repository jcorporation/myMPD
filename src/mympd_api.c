/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api.h"

#include "lib/api.h"
#include "lib/log.h"
#include "lib/mympd_configuration.h"
#include "lib/sds_extras.h"
#include "lib/validate.h"
#include "mpd_client.h"
#include "mpd_client/mpd_client_stats.h"
#include "mpd_client/mpd_client_trigger.h"
#include "mpd_shared.h"
#include "mympd_autoconf.h"
#include "mympd_api/mympd_api_home.h"
#include "mympd_api/mympd_api_settings.h"
#include "mympd_api/mympd_api_timer.h"
#include "mympd_api/mympd_api_timer_handlers.h"
#include "mympd_api/mympd_api_utility.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

//private definitions
void mympd_autoconf(struct t_mympd_state *mympd_state);

//public functions
void *mympd_api_loop(void *arg_config) {
    thread_logname = sdsreplace(thread_logname, "mympdapi");
    prctl(PR_SET_NAME, thread_logname, 0, 0, 0);

    //create mympd_state struct and set defaults
    struct t_mympd_state *mympd_state = (struct t_mympd_state *)malloc(sizeof(struct t_mympd_state));
    assert(mympd_state);
    mympd_state->config = (struct t_config *) arg_config;
    default_mympd_state(mympd_state);

    if (mympd_state->config->first_startup == true) {
        MYMPD_LOG_NOTICE("Starting myMPD autoconfiguration");
        mympd_autoconf(mympd_state);
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
    sdsfree(thread_logname);
    return NULL;
}

//private functions
void mympd_autoconf(struct t_mympd_state *mympd_state) {
    sds mpd_conf = find_mpd_conf();
    if (sdslen(mpd_conf) > 0) {
        MYMPD_LOG_NOTICE("Found %s", mpd_conf);
        //get config from mpd configuration file
        sds mpd_host = get_mpd_conf("bind_to_address", mympd_state->mpd_state->mpd_host, vcb_isname);
        mympd_state->mpd_state->mpd_host = sdsreplace(mympd_state->mpd_state->mpd_host, mpd_host);
        sdsfree(mpd_host);

        sds mpd_pass = get_mpd_conf("password", mympd_state->mpd_state->mpd_pass, vcb_isname);
        mympd_state->mpd_state->mpd_pass = sdsreplace(mympd_state->mpd_state->mpd_pass, mpd_pass);
        sdsfree(mpd_pass);
        
        sds mpd_port = get_mpd_conf("port", mympd_state->mpd_state->mpd_host, vcb_isdigit);
        int port = (int)strtoimax(mpd_port, NULL, 10);
        if (port > 1024 && port <= 65534) {
            mympd_state->mpd_state->mpd_port = port;
        }
        sdsfree(mpd_port);
        
        sds music_directory = get_mpd_conf("music_directory", mympd_state->music_directory, vcb_isfilepath);
        mympd_state->music_directory = sdsreplace(mympd_state->music_directory, music_directory);
        sdsfree(music_directory);
        
        sds playlist_directory = get_mpd_conf("playlist_directory", mympd_state->playlist_directory, vcb_isfilepath);
        mympd_state->playlist_directory = sdsreplace(mympd_state->playlist_directory, playlist_directory);
        sdsfree(playlist_directory);
    }
    else {
        MYMPD_LOG_NOTICE("Reading environment");
        const char *mpd_host_env = getenv("MPD_HOST"); /* Flawfinder: ignore */
        if (mpd_host_env != NULL && strlen(mpd_host_env) <= 100) {
            sds mpd_host = sdsnew(mpd_host_env);
            if (vcb_isname(mpd_host) == true) {
                if (mpd_host[0] != '@' && strstr(mpd_host, "@") != NULL) {
                    int count;
                    sds *tokens = sdssplitlen(mpd_host, (ssize_t)strlen(mpd_host), "@", 1, &count);
                    mympd_state->mpd_state->mpd_host = sdsreplace(mympd_state->mpd_state->mpd_host, tokens[1]);
                    mympd_state->mpd_state->mpd_pass = sdsreplace(mympd_state->mpd_state->mpd_pass, tokens[0]);
                    sdsfreesplitres(tokens,count);
                }
                else {
                    //no password
                    mympd_state->mpd_state->mpd_host = sdsreplace(mympd_state->mpd_state->mpd_host, mpd_host);
                }
                MYMPD_LOG_NOTICE("Setting mpd host to \"%s\"", mympd_state->mpd_state->mpd_host);
            }
            sdsfree(mpd_host);
        }
        const char *mpd_port_env = getenv("MPD_PORT"); /* Flawfinder: ignore */
        if (mpd_port_env != NULL && strlen(mpd_port_env) <= 5) {
            sds mpd_port = sdsnew(mpd_port_env);
            if (vcb_isdigit(mpd_port) == true) {
                int port = (int)strtoimax(mpd_port, NULL, 10);
                if (port > 1024 && port <= 65534) {
                    mympd_state->mpd_state->mpd_port = port;
                    MYMPD_LOG_NOTICE("Setting mpd port to \"%d\"", mympd_state->mpd_state->mpd_port);
                }
                else {
                    MYMPD_LOG_WARN("MPD port must between 1024 and 65534, default is 6600");
                }
            }
            sdsfree(mpd_port);
        }
    }
    sdsfree(mpd_conf);
}
