/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/partitions.h"

#include "src/lib/last_played.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mympd_api/settings.h"


#include <string.h>

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
    //connection will be established in next idle loop run
    partition_state->next = malloc_assert(sizeof(struct t_partition_state));
    //set default partition state
    partition_state_default(partition_state->next, name, mympd_state->mpd_state, mympd_state->config);
    //read partition specific state from disc
    mympd_api_settings_statefiles_partition_read(partition_state->next);
    last_played_file_read(partition_state->next);
    //push settings to web_server_queue
    settings_to_webserver(mympd_state);
}

/**
 * Populates the mpd connection fds
 * @param mympd_state pointer to t_mympd_state struct
 */
void partitions_get_fds(struct t_mympd_state *mympd_state) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    mympd_state->nfds = 0;
    while (partition_state != NULL) {
        if (mympd_state->nfds == MPD_CONNECTION_MAX) {
            MYMPD_LOG_ERROR(NULL, "Too many partitions");
            break;
        }
        if (partition_state->conn != NULL &&
            partition_state->conn_state == MPD_CONNECTED)
        {
            mympd_state->fds[mympd_state->nfds].fd = mpd_connection_get_fd(partition_state->conn);
            mympd_state->fds[mympd_state->nfds].events = POLLIN;
            mympd_state->nfds++;
        }
        partition_state = partition_state->next;
    }
}
