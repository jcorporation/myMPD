/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/mympd_api.h"

#include "src/lib/album_cache.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/autoconf.h"
#include "src/mpd_client/connection.h"
#include "src/mpd_client/idle.h"
#include "src/mpd_client/stickerdb.h"
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

    //start autoconfiguration, if mpd_host does not exist
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/mpd_host", mympd_state->config->workdir, DIR_WORK_STATE);
    if (testfile_read(filepath) == false) {
        mpd_client_autoconf(mympd_state);
    }
    FREE_SDS(filepath);

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
    //caches
    if (mympd_state->config->save_caches == true) {
        //album cache
        MYMPD_LOG_INFO(NULL, "Reading album cache from disc");
        album_cache_read(&mympd_state->mpd_state->album_cache, mympd_state->config->workdir);
    }
    //set timers
    if (mympd_state->config->covercache_keep_days > 0) {
        MYMPD_LOG_DEBUG(NULL, "Adding timer for \"crop covercache\" to execute periodic each day");
        mympd_api_timer_add(&mympd_state->timer_list, COVERCACHE_CLEANUP_OFFSET, COVERCACHE_CLEANUP_INTERVAL,
            timer_handler_by_id, TIMER_ID_COVERCACHE_CROP, NULL);
    }

    //start trigger
    mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_START, MPD_PARTITION_ALL);

    //push ready state to webserver
    struct t_work_response *web_server_response = create_response_new(CONN_ID_CONFIG_TO_WEBSERVER, 0, INTERNAL_API_WEBSERVER_READY, MPD_PARTITION_DEFAULT);
    mympd_queue_push(web_server_queue, web_server_response, 0);

    // connect to stickerdb
    if (mympd_state->config->stickers == true) {
        if (stickerdb_connect(mympd_state->stickerdb) == true) {
            stickerdb_enter_idle(mympd_state->stickerdb);
        }
    }
    else {
        MYMPD_LOG_NOTICE("stickerdb", "Stickers are disabled by config");
    }

    //thread loop
    while (s_signal_received == 0) {
        mpd_client_idle(mympd_state);
        if (mympd_state->timer_list.active > 0) {
            mympd_api_timer_check(&mympd_state->timer_list);
        }
        if (mympd_state->mpd_state->feat_stickers == true) {
            stickerdb_idle(mympd_state->stickerdb);
        }
    }
    MYMPD_LOG_DEBUG(NULL, "Stopping mympd_api thread");

    //stop trigger
    mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_STOP, MPD_PARTITION_ALL);

    //disconnect from mpd
    mpd_client_disconnect_all(mympd_state, MPD_DISCONNECT_INSTANT);
    if (mympd_state->stickerdb->conn != NULL) {
        mpd_client_disconnect(mympd_state->stickerdb, MPD_DISCONNECT_INSTANT);
    }

    // write album cache to disc
    // only for simple mode to save the cached uris
    if (mympd_state->config->save_caches == true &&
        mympd_state->config->albums == false)
    {
        album_cache_write(&mympd_state->mpd_state->album_cache, mympd_state->config->workdir, &mympd_state->mpd_state->tags_album, true);
    }

    //save and free states
    mympd_state_save(mympd_state, true);

    FREE_SDS(thread_logname);
    return NULL;
}
