/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "log.h"
#include "list.h"
#include "config_defs.h"
#include "tiny_queue.h"
#include "api.h"
#include "global.h"
#include "utility.h"
#include "mpd_shared.h"
#include "mpd_worker/mpd_worker_utility.h"
#include "mpd_worker/mpd_worker_api.h"
#include "mpd_worker.h"

//private definitions
static void mpd_worker_idle(t_config *config, t_mpd_state *mpd_state);
static void mpd_worker_disconnect(t_mpd_state *mpd_state);

//public functions
void *mpd_worker_loop(void *arg_config) {
    t_config *config = (t_config *) arg_config;
    //State of mpd connection
    t_mpd_worker_state *mpd_worker_state = (t_mpd_worker_state *)malloc(sizeof(t_mpd_worker_state));
    default_mpd_worker_state(mpd_worker_state);

    //wait for initial settings
    while (s_signal_received == 0) {
        t_work_request *request = tiny_queue_shift(mpd_worker_queue, 50);
        if (request != NULL) {
            if (request->cmd_id == MYMPD_API_SETTINGS_SET) {
                LOG_DEBUG("Got initial settings from mympd_api");
                mpd_worker_api(config, mpd_worker_state, request);
                break;
            }
            LOG_DEBUG("MPD worker not initialized, discarding message");
            free_request(request);
        }
    }

    LOG_INFO("Starting mpd_worker");
    //On startup connect instantly
    mpd_state->conn_state = MPD_DISCONNECTED;
    while (s_signal_received == 0) {
        mpd_worker_idle(config, mpd_state);
    }
    //Cleanup
    mpd_worker_disconnect(mpd_worker_state);
    free_mpd_worker_state(mpd_worker_state);
    return NULL;
}

static void mpd_worker_idle(t_config *config, t_mpd_worker_state *mpd_worker_state) {
    struct pollfd fds[1];
    int pollrc;
    sds buffer = sdsempty();
    unsigned mpd_worker_queue_length = 0;
    
    switch (mpd_worker_state->conn_state) {
        case MPD_WAIT: {
            time_t now = time(NULL);
            if (now > mpd_worker_state->reconnect_time) {
                mpd_worker_state->conn_state = MPD_DISCONNECTED;
            }
            if (now < mpd_worker_state->reconnect_time) {
                //pause 100ms to prevent high cpu usage
                usleep(100000);
            }
            break;
        }
        case MPD_DISCONNECTED:
            /* Try to connect */
            if (strncmp(mpd_worker_state->mpd_host, "/", 1) == 0) {
                LOG_INFO("MPD worker connecting to socket %s", mpd_worker_state->mpd_host);
            }
            else {
                LOG_INFO("MPD worker connecting to %s:%d", mpd_worker_state->mpd_host, mpd_worker_state->mpd_port);
            }
            mpd_worker_state->conn = mpd_connection_new(mpd_worker_state->mpd_host, mpd_worker_state->mpd_port, mpd_worker_state->timeout);
            if (mpd_worker_state->conn == NULL) {
                LOG_ERROR("MPD worker connection to failed: out-of-memory");
                mpd_worker_state->conn_state = MPD_FAILURE;
                mpd_connection_free(mpd_worker_state->conn);
                return;
            }

            if (mpd_connection_get_error(mpd_worker_state->conn) != MPD_ERROR_SUCCESS) {
                LOG_ERROR("MPD worker connection: %s", mpd_connection_get_error_message(mpd_worker_state->conn));
                mpd_worker_state->conn_state = MPD_FAILURE;
                return;
            }

            if (sdslen(mpd_worker_state->mpd_pass) > 0 && !mpd_run_password(mpd_worker_state->conn, mpd_worker_state->mpd_pass)) {
                LOG_ERROR("MPD worker connection: %s", mpd_connection_get_error_message(mpd_worker_state->conn));
                mpd_worker_state->conn_state = MPD_FAILURE;
                return;
            }

            LOG_INFO("MPD worker connected");
            mpd_connection_set_timeout(mpd_worker_state->conn, mpd_worker_state->timeout);
            mpd_worker_state->conn_state = MPD_CONNECTED;
            mpd_worker_state->reconnect_interval = 0;
            mpd_worker_state->reconnect_time = 0;
            }
            if (!mpd_send_idle(mpd_worker_state->conn,)) {
                LOG_ERROR("MPD worker entering idle mode failed");
                mpd_worker_state->conn_state = MPD_FAILURE;
            }
            break;

        case MPD_FAILURE:
            LOG_ERROR("MPD worker connection failed");
            // fall through
        case MPD_DISCONNECT:
        case MPD_RECONNECT:
            if (mpd_worker_state->conn != NULL) {
                mpd_connection_free(mpd_worker_state->conn);
            }
            mpd_worker_state->conn = NULL;
            mpd_worker_state->conn_state = MPD_WAIT;
            if (mpd_worker_state->reconnect_interval <= 20) {
                mpd_worker_state->reconnect_interval += 2;
            }
            mpd_worker_state->reconnect_time = time(NULL) + mpd_worker_state->reconnect_interval;
            LOG_VERBOSE("MPD worker waiting %u seconds before reconnection", mpd_worker_state->reconnect_interval);
            break;

        case MPD_CONNECTED:
            mpd_worker_queue_length = tiny_queue_length(mpd_worker_queue, 50);
            if (mpd_worker_queue_length > 0) {
                LOG_DEBUG("Leaving mpd worker idle mode");
                if (!mpd_send_noidle(mpd_worker_state->conn)) {
                    check_error_and_recover(mpd_worker_state, NULL, NULL, 0);
                    mpd_worker_state->conn_state = MPD_FAILURE;
                    break;
                }
                //Handle request
                LOG_DEBUG("Handle request");
                t_work_request *request = tiny_queue_shift(mpd_worker_queue, 50);
                if (request != NULL) {
                    mpd_worker_api(config, mpd_worker_state, request);
                }
                LOG_DEBUG("Entering mpd worker idle mode");
                if (!mpd_send_idle(mpd_worker_state->conn)) {
                    check_error_and_recover(mpd_worker_state, NULL, NULL, 0);
                    mpd_worker_state->conn_state = MPD_FAILURE;
                }
            }
            break;
        default:
            LOG_ERROR("Invalid mpd worker connection state");
    }
    sdsfree(buffer);
}

static void mpd_worker_disconnect(t_mpd_worker_state *mpd_worker_state) {
    mpd_worker_state->conn_state = MPD_DISCONNECT;
    if (mpd_worker_state->conn != NULL) {
        mpd_connection_free(mpd_worker_state->conn);
    }
}
