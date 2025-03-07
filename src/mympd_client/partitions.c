/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD partitions specific functions
 */

#include "compile_time.h"
#include "src/mympd_client/partitions.h"

#include "src/lib/last_played.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mympd_state.h"
#include "src/lib/sds_extras.h"
#include "src/lib/timer.h"
#include "src/mympd_client/connection.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/features.h"
#include "src/mympd_client/jukebox.h"
#include "src/mympd_api/settings.h"
#include "src/mympd_api/status.h"
#include "src/mympd_api/timer.h"
#include "src/mympd_api/timer_handlers.h"
#include "src/mympd_api/trigger.h"

#include <string.h>

/**
 * Connects to MPD and switches to the defined partition
 * @param mympd_state Pointer to mympd_state
 * @param partition_state Pointer to partition state
 * @return true on success, else false
 */
bool partitions_connect(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state) {
    MYMPD_LOG_INFO(partition_state->name, "Creating mpd connection for partition \"%s\"", partition_state->name);
    if (mympd_client_connect(partition_state) == false) {
        return false;
    }
    // we are connected
    if (partition_state->is_default == true) {
        // check version
        if (mpd_connection_cmp_server_version(partition_state->conn, MPD_VERSION_MIN_MAJOR, MPD_VERSION_MIN_MINOR, MPD_VERSION_MIN_PATCH) < 0) {
            MYMPD_LOG_EMERG(partition_state->name, "MPD version too old, myMPD supports only MPD version >= 0.21");
            s_signal_received = 1;
            return false;
        }
        mympd_client_mpd_features(mympd_state, partition_state);
        // initiate cache updates
        if (mympd_state->mpd_state->feat.tags == true) {
            mympd_api_timer_replace(&mympd_state->timer_list, 2, TIMER_ONE_SHOT_REMOVE,
                timer_handler_by_id, TIMER_ID_CACHES_CREATE, NULL);
        }
        // set timer for smart playlist update
        if (mympd_state->smartpls_interval > 0) {
            MYMPD_LOG_DEBUG(NULL, "Adding timer to update the smart playlists");
            mympd_api_timer_replace(&mympd_state->timer_list, TIMER_SMARTPLS_UPDATE_OFFSET, mympd_state->smartpls_interval,
                timer_handler_by_id, TIMER_ID_SMARTPLS_UPDATE, NULL);
        }
        // populate the partition list
        if (partition_state->mpd_state->feat.partitions == true) {
            partitions_populate(mympd_state);
        }
    }
    else {
        // change partition
        MYMPD_LOG_INFO(partition_state->name, "Switching to partition \"%s\"", partition_state->name);
        mpd_run_switch_partition(partition_state->conn, partition_state->name);
        if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_switch_partition") == false) {
            MYMPD_LOG_ERROR(partition_state->name, "Could not switch to partition \"%s\"", partition_state->name);
            mympd_client_disconnect(partition_state);
            return false;
        }
    }

    // update state
    sds buffer = sdsempty();
    buffer = mympd_api_status_get(partition_state, &mympd_state->album_cache, buffer, 0, RESPONSE_TYPE_JSONRPC_RESPONSE);
    FREE_SDS(buffer);

    // disarm connect timer
    mympd_timer_set(partition_state->timer_fd_mpd_connect, 0, 0);

    // jukebox
    if (partition_state->jukebox.mode != JUKEBOX_OFF &&
        partition_state->queue_length == 0)
    {
        jukebox_run(mympd_state, partition_state, &mympd_state->album_cache);
    }

    // enter idle mode
    if (mpd_send_idle_mask(partition_state->conn, partition_state->idle_mask) == false) {
        mympd_check_error_and_recover(partition_state, NULL, "mpd_send_idle_mask");
        return false;
    }
    
    send_jsonrpc_event(JSONRPC_EVENT_MPD_CONNECTED, partition_state->name);
    mympd_api_trigger_execute(&mympd_state->trigger_list, TRIGGER_MYMPD_CONNECTED, partition_state->name, NULL);
    return true;
}

/**
 * Get the partition state struct by partition name
 * @param mympd_state pointer to central myMPD state
 * @param name mpd partition name
 * @return pointer to partition_state, NULL if partition is not found
 */
struct t_partition_state *partitions_get_by_name(struct t_mympd_state *mympd_state, const char *name) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        if (strcmp(partition_state->name, name) == 0) {
            return partition_state;
        }
        partition_state = partition_state->next;
    }
    return NULL;
}

/**
 * Removes all but default partition from the list
 * @param mympd_state pointer to central myMPD state
 */
void partitions_list_clear(struct t_mympd_state *mympd_state) {
    struct t_partition_state *current = mympd_state->partition_state->next;
    while (current != NULL) {
        MYMPD_LOG_INFO(NULL, "Removing partition \"%s\" from the partition list", current->name);
        struct t_partition_state *next = current->next;
        //free partition state
        partition_state_free(current);
        current = next;
    }
    mympd_state->partition_state->next = NULL;
}

/**
 * Populates the partition list: removes and adds partitions according to mpd
 * @param mympd_state pointer to t_mympd_state struct
 * @return true on success, else false
 */
bool partitions_populate(struct t_mympd_state *mympd_state) {
    struct t_list mpd_partitions;
    list_init(&mpd_partitions);
    //first add all missing partitions to the list
    if (mpd_send_listpartitions(mympd_state->partition_state->conn)) {
        struct mpd_pair *partition;
        while ((partition = mpd_recv_partition_pair(mympd_state->partition_state->conn)) != NULL) {
            const char *name = partition->value;
            if (partitions_check(mympd_state, name) == false) {
                MYMPD_LOG_INFO(NULL, "Adding partition \"%s\" to the partition list", name);
                partitions_add(mympd_state, name);
            }
            list_push(&mpd_partitions, name, 0, NULL, NULL);
            mpd_return_pair(mympd_state->partition_state->conn, partition);
        }
    }
    mpd_response_finish(mympd_state->partition_state->conn);
    if (mympd_check_error_and_recover(mympd_state->partition_state, NULL, "mpd_send_listpartitions") == false) {
        list_clear(&mpd_partitions);
        return false;
    }
    //remove obsolete partitions
    //skip default partition (first entry)
    struct t_partition_state *current = mympd_state->partition_state->next;
    struct t_partition_state *previous = mympd_state->partition_state;
    for (; current != NULL; previous = current, current = current->next) {
        if (list_get_node(&mpd_partitions, current->name) == NULL) {
            MYMPD_LOG_INFO(NULL, "Removing partition \"%s\" from the partition list", current->name);
            struct t_partition_state *next = current->next;
            //free partition state
            partition_state_free(current);
            //partition was removed from mpd
            previous->next = next;
            //go back to previous node
            current = previous;
        }
    }
    list_clear(&mpd_partitions);
    return true;
}

/**
 * Checks if the partition is already in the list
 * @param mympd_state pointer to t_mympd_state struct
 * @param name partition name
 * @return true if partition is in the list, else false
 */
bool partitions_check(struct t_mympd_state *mympd_state, const char *name) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        if (strcmp(partition_state->name, name) == 0) {
            return true;
        }
        partition_state = partition_state->next;
    }
    return false;
}

/**
 * Adds a partition to the partition list
 * @param mympd_state pointer to t_mympd_state struct
 * @param name partition name
 */
void partitions_add(struct t_mympd_state *mympd_state, const char *name) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    //goto end
    while (partition_state->next != NULL) {
        partition_state = partition_state->next;
    }
    //append new partition struct and set defaults
    //connection will be established through connect timer
    partition_state->next = malloc_assert(sizeof(struct t_partition_state));
    //set default partition state
    partition_state_default(partition_state->next, name, mympd_state->mpd_state, mympd_state->config);
    //read partition specific state from disc
    mympd_api_settings_statefiles_partition_read(partition_state->next);
    last_played_file_read(partition_state->next);
    //set connect timer
    mympd_timer_set(partition_state->next->timer_fd_mpd_connect, 0, 5);
    //push settings to web_server_queue
    settings_to_webserver(mympd_state);
}
