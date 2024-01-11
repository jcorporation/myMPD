/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/mympd_api.h"

#include "src/lib/album_cache.h"
#include "src/lib/event.h"
#include "src/lib/filehandler.h"
#include "src/lib/last_played.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/lib/thread.h"
#include "src/lib/timer.h"
#include "src/mpd_client/autoconf.h"
#include "src/mpd_client/connection.h"
#include "src/mpd_client/idle.h"
#include "src/mpd_client/partitions.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mympd_api/home.h"
#include "src/mympd_api/settings.h"
#include "src/mympd_api/timer.h"
#include "src/mympd_api/timer_handlers.h"
#include "src/mympd_api/trigger.h"

#include <errno.h>

// private definitions

static void populate_pfds(struct t_mympd_state *mympd_state);
static void handle_socket_pollin(struct t_mympd_state *mympd_state, nfds_t i, struct t_work_request **request);
static void handle_socket_error(struct t_mympd_state *mympd_state, nfds_t i);

// public functions

/**
 * This is the main function for the mympd_api thread
 * @param arg_config void pointer to t_config struct
 */
void *mympd_api_loop(void *arg_config) {
    thread_logname = sds_replace(thread_logname, "mympdapi");
    set_threadname(thread_logname);

    // create initial mympd_state struct and set defaults
    struct t_mympd_state *mympd_state = malloc_assert(sizeof(struct t_mympd_state));
    mympd_state_default(mympd_state, (struct t_config *)arg_config);

    // start autoconfiguration, if mpd_host does not exist
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/mpd_host", mympd_state->config->workdir, DIR_WORK_STATE);
    if (testfile_read(filepath) == false) {
        mpd_client_autoconf(mympd_state);
    }
    FREE_SDS(filepath);

    // read global states
    mympd_api_settings_statefiles_global_read(mympd_state);
    // read myMPD states for default partition
    mympd_api_settings_statefiles_partition_read(mympd_state->partition_state);
    // last played for default partition
    last_played_file_read(mympd_state->partition_state);
    // home icons
    mympd_api_home_file_read(&mympd_state->home_list, mympd_state->config->workdir);
    // timer
    mympd_api_timer_file_read(&mympd_state->timer_list, mympd_state->config->workdir);
    // trigger
    mympd_api_trigger_file_read(&mympd_state->trigger_list, mympd_state->config->workdir);
    // caches
    if (mympd_state->config->save_caches == true) {
        // album cache
        album_cache_read(&mympd_state->album_cache, mympd_state->config->workdir, &mympd_state->config->albums);
    }
    // set timers
    if (mympd_state->config->covercache_keep_days > 0) {
        MYMPD_LOG_DEBUG(NULL, "Adding timer for \"crop covercache\" to execute periodic each day");
        mympd_api_timer_add(&mympd_state->timer_list, TIMER_COVERCACHE_CLEANUP_OFFSET, TIMER_COVERCACHE_CLEANUP_INTERVAL,
            timer_handler_by_id, TIMER_ID_COVERCACHE_CROP, NULL);
    }

    // start trigger
    mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_START, MPD_PARTITION_ALL);

    // push ready state to webserver
    struct t_work_response *web_server_response = create_response_new(RESPONSE_TYPE_PUSH_CONFIG, 0, 0, INTERNAL_API_WEBSERVER_READY, MPD_PARTITION_DEFAULT);
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

    // connect to default mpd partition
    mympd_timer_set(mympd_state->partition_state->timer_fd_mpd_connect, 0, 5);

    // thread loop
    while (s_signal_received == 0) {
        populate_pfds(mympd_state);
        errno = 0;
        int cnt = poll(mympd_state->pfds.fds, mympd_state->pfds.len, -1);
        if (cnt < 0) {
            MYMPD_LOG_ERROR(NULL, "Error polling file descriptors");
            MYMPD_LOG_ERRNO(NULL, errno);
            continue;
        }
        struct t_work_request *request = NULL;
        for (nfds_t i = 0; i < mympd_state->pfds.len; i++) {
            if (mympd_state->pfds.fds[i].revents & POLLIN) {
                handle_socket_pollin(mympd_state, i, &request);
            }
            else if (mympd_state->pfds.fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                handle_socket_error(mympd_state, i);
            }
        }
        // Iterate through mpd partitions and handle the events
        mpd_client_idle(mympd_state, request);
    }
    MYMPD_LOG_DEBUG(NULL, "Stopping mympd_api thread");

    // stop trigger
    mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_STOP, MPD_PARTITION_ALL);

    // disconnect from mpd
    mpd_client_disconnect_all(mympd_state);
    if (mympd_state->stickerdb->conn != NULL) {
        stickerdb_disconnect(mympd_state->stickerdb);
    }

    // write album cache to disc
    // only for simple mode to save the cached uris
    if (mympd_state->config->save_caches == true &&
        mympd_state->config->albums.mode == ALBUM_MODE_SIMPLE)
    {
        album_cache_write(&mympd_state->album_cache, mympd_state->config->workdir,
            &mympd_state->mpd_state->tags_album, &mympd_state->config->albums, true);
    }

    // save and free states
    mympd_state_save(mympd_state, true);

    FREE_SDS(thread_logname);
    return NULL;
}

// private functions

/**
 * Handles socket read event
 * @param mympd_state pointer to mympd state
 * @param i fd number from pfds array
 * @param request pointer to work request struct to populate
 */
static void handle_socket_pollin(struct t_mympd_state *mympd_state, nfds_t i, struct t_work_request **request) {
    switch (mympd_state->pfds.fd_types[i]) {
        case PFD_TYPE_TIMER:
            MYMPD_LOG_DEBUG(NULL, "Timer event");
            if (event_pfd_read_fd(mympd_state->pfds.fds[i].fd) == true) {
                mympd_api_timer_check(mympd_state->pfds.fds[i].fd, &mympd_state->timer_list);
            }
            break;
        case PFD_TYPE_STICKERDB:
            MYMPD_LOG_DEBUG("stickerdb", "Stickerdb event");
            stickerdb_idle(mympd_state->stickerdb);
            break;
        case PFD_TYPE_QUEUE:
            // check the mympd_api_queue
            MYMPD_LOG_DEBUG(NULL, "Queue event");
            if (event_pfd_read_fd(mympd_state->pfds.fds[i].fd) == true) {
                *request = mympd_queue_shift(mympd_api_queue, -1, 0);
                if (*request == NULL) {
                    break;
                }
                struct t_partition_state *partition_state = partitions_get_by_name(mympd_state, (*request)->partition);
                if (partition_state == NULL) {
                    MYMPD_LOG_ERROR(NULL, "Unable to find partition for queue fd: %d", mympd_state->pfds.fds[i].fd);
                    break;
                }
                MYMPD_LOG_DEBUG(partition_state->name, "Queue event");
                partition_state->waiting_events |= PFD_TYPE_QUEUE;
            }
            break;
        case PFD_TYPE_PARTITION:
            // mpd idle event
            MYMPD_LOG_DEBUG(mympd_state->pfds.partition_states[i]->name, "Partition event");
            mympd_state->pfds.partition_states[i]->waiting_events |= PFD_TYPE_PARTITION;
            break;
        case PFD_TYPE_TIMER_JUKEBOX:
            // jukebox should add a song
            MYMPD_LOG_DEBUG(mympd_state->pfds.partition_states[i]->name, "Jukebox event");
            if (event_pfd_read_fd(mympd_state->pfds.fds[i].fd) == true) {
                mympd_state->pfds.partition_states[i]->waiting_events |= PFD_TYPE_TIMER_JUKEBOX;
            }
            break;
        case PFD_TYPE_TIMER_SCROBBLE:
            MYMPD_LOG_DEBUG(mympd_state->pfds.partition_states[i]->name, "Scrobble event");
            if (event_pfd_read_fd(mympd_state->pfds.fds[i].fd) == true) {
                mpd_client_scrobble(mympd_state, mympd_state->pfds.partition_states[i]);
            }
            break;
        case PFD_TYPE_TIMER_MPD_CONNECT:
            // connect to mpd
            MYMPD_LOG_DEBUG(mympd_state->pfds.partition_states[i]->name, "Connect event");
            if (event_pfd_read_fd(mympd_state->pfds.fds[i].fd) == true) {
                partitions_connect(mympd_state, mympd_state->pfds.partition_states[i]);
            }
            break;
    }
}

/**
 * Handles socket errors
 * @param mympd_state pointer to mympd state
 * @param i fd number from pfds array
 */
static void handle_socket_error(struct t_mympd_state *mympd_state, nfds_t i) {
    MYMPD_LOG_ERROR(NULL, "Socket error %s for %d of type %s", lookup_pfd_revents(mympd_state->pfds.fds[i].revents),
        mympd_state->pfds.fds[i].fd, lookup_pfd_type(mympd_state->pfds.fd_types[i]));
    switch (mympd_state->pfds.fd_types[i]) {
        case PFD_TYPE_PARTITION:
            mpd_client_disconnect(mympd_state->pfds.partition_states[i]);
            break;
        case PFD_TYPE_STICKERDB:
            stickerdb_disconnect(mympd_state->stickerdb);
            break;
        case PFD_TYPE_QUEUE:
            event_fd_close(mympd_state->pfds.fds[i].fd);
            mympd_api_queue->event_fd = event_eventfd_create();
            break;
        default:
            MYMPD_LOG_DEBUG(NULL, "Closing socket");
            event_fd_close(mympd_state->pfds.fds[i].fd);
    }
}

/**
 * Populates the poll fds array with fds from:
 * mpd partitions, mpd stickerdb, timers
 * @param mympd_state pointer to mympd state
 */
static void populate_pfds(struct t_mympd_state *mympd_state) {
    mympd_state->pfds.len = 0;
    // Connections for MPD partitions
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        if (partition_state->conn != NULL) {
            event_pfd_add_fd(&mympd_state->pfds, mpd_connection_get_fd(partition_state->conn), PFD_TYPE_PARTITION, partition_state);
            event_pfd_add_fd(&mympd_state->pfds, partition_state->timer_fd_scrobble, PFD_TYPE_TIMER_SCROBBLE, partition_state);
            event_pfd_add_fd(&mympd_state->pfds, partition_state->timer_fd_jukebox, PFD_TYPE_TIMER_JUKEBOX, partition_state);
        }
        event_pfd_add_fd(&mympd_state->pfds, partition_state->timer_fd_mpd_connect, PFD_TYPE_TIMER_MPD_CONNECT, partition_state);
        partition_state = partition_state->next;
    }
    // StickerDB MPD connection
    if (mympd_state->stickerdb->conn != NULL &&
        mympd_state->stickerdb->conn_state == MPD_CONNECTED)
    {
        event_pfd_add_fd(&mympd_state->pfds, mpd_connection_get_fd(mympd_state->stickerdb->conn), PFD_TYPE_STICKERDB, NULL);
    }
    // mympd_api_queue
    event_pfd_add_fd(&mympd_state->pfds, mympd_api_queue->event_fd, PFD_TYPE_QUEUE, NULL);
    // Timer
    struct t_list_node *current = mympd_state->timer_list.list.head;
    while (current != NULL) {
        struct t_timer_node *current_timer = (struct t_timer_node *)current->user_data;
        event_pfd_add_fd(&mympd_state->pfds, current_timer->fd, PFD_TYPE_TIMER, NULL);
        current = current->next;
    }
    #ifdef MYMPD_DEBUG
        MYMPD_LOG_DEBUG(NULL, "Polling %lu fds", mympd_state->pfds.len);
    #endif
}
