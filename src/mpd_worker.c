/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
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
#include "mympd_config_defs.h"
#include "tiny_queue.h"
#include "api.h"
#include "global.h"
#include "utility.h"
#include "mpd_shared/mpd_shared_typedefs.h"
#include "mpd_shared.h"
#include "mpd_shared/mpd_shared_sticker.h"
#include "mpd_worker/mpd_worker_utility.h"
#include "mpd_worker/mpd_worker_api.h"
#include "mpd_worker/mpd_worker_smartpls.h"
#include "mpd_worker/mpd_worker_cache.h"
#include "mpd_worker.h"

//private definitions
static void mpd_worker_idle(t_config *config, t_mpd_worker_state *mpd_worker_state);
static void mpd_worker_parse_idle(t_config *config, t_mpd_worker_state *mpd_worker_state, int idle_bitmask);

//public functions
void *mpd_worker_loop(void *arg_config) {
    thread_logname = sdsreplace(thread_logname, "mpdworker");
    t_config *config = (t_config *) arg_config;
    //State of mpd connection
    t_mpd_worker_state *mpd_worker_state = (t_mpd_worker_state *)malloc(sizeof(t_mpd_worker_state));
    assert(mpd_worker_state);
    default_mpd_worker_state(mpd_worker_state);

    //wait for initial settings
    while (s_signal_received == 0) {
        t_work_request *request = tiny_queue_shift(mpd_worker_queue, 50, 0);
        if (request != NULL) {
            if (request->cmd_id == MYMPD_API_SETTINGS_SET) {
                MYMPD_LOG_DEBUG("Got initial settings from mympd_api");
                mpd_worker_api(config, mpd_worker_state, request);
                break;
            }
            MYMPD_LOG_DEBUG("MPD worker not initialized, discarding message");
            free_request(request);
        }
    }

    MYMPD_LOG_NOTICE("Starting mpd_worker");
    //On startup connect instantly
    mpd_worker_state->mpd_state->conn_state = MPD_DISCONNECTED;
    while (s_signal_received == 0) {
        mpd_worker_idle(config, mpd_worker_state);
        if (mpd_worker_state->mpd_state->conn_state == MPD_TOO_OLD) {
            break;
        }
    }
    //Cleanup
    mpd_shared_mpd_disconnect(mpd_worker_state->mpd_state);
    free_mpd_worker_state(mpd_worker_state);
    sdsfree(thread_logname);
    return NULL;
}

static void mpd_worker_idle(t_config *config, t_mpd_worker_state *mpd_worker_state) {
    unsigned mpd_worker_queue_length = 0;
    struct pollfd fds[1];
    int pollrc;
    enum mpd_idle set_idle_mask = MPD_IDLE_DATABASE;
    
    switch (mpd_worker_state->mpd_state->conn_state) {
        case MPD_WAIT: {
            mpd_worker_queue_length = tiny_queue_length(mpd_worker_queue, 50);
            if (mpd_worker_queue_length > 0) {
                //Handle request
                MYMPD_LOG_DEBUG("Handle request (mpd disconnected)");
                t_work_request *request = tiny_queue_shift(mpd_worker_queue, 50, 0);
                if (request != NULL) {
                    if (request->cmd_id == MYMPD_API_SETTINGS_SET) {
                        //allow to change mpd host
                        mpd_worker_api(config, mpd_worker_state, request);
                        mpd_worker_state->mpd_state->conn_state = MPD_DISCONNECTED;
                    }
                    else {
                        //other requests not allowed
                        free_request(request);
                    }
                }
            }

            time_t now = time(NULL);
            if (now > mpd_worker_state->mpd_state->reconnect_time) {
                mpd_worker_state->mpd_state->conn_state = MPD_DISCONNECTED;
            }
            if (now < mpd_worker_state->mpd_state->reconnect_time) {
                //pause 100ms to prevent high cpu usage
                my_usleep(100000);
            }
            break;
        }
        case MPD_DISCONNECTED:
            /* Try to connect */
            if (strncmp(mpd_worker_state->mpd_state->mpd_host, "/", 1) == 0) {
                MYMPD_LOG_NOTICE("MPD worker connecting to socket %s", mpd_worker_state->mpd_state->mpd_host);
            }
            else {
                MYMPD_LOG_NOTICE("MPD worker connecting to %s:%d", mpd_worker_state->mpd_state->mpd_host, mpd_worker_state->mpd_state->mpd_port);
            }
            mpd_worker_state->mpd_state->conn = mpd_connection_new(mpd_worker_state->mpd_state->mpd_host, mpd_worker_state->mpd_state->mpd_port, mpd_worker_state->mpd_state->timeout);
            if (mpd_worker_state->mpd_state->conn == NULL) {
                MYMPD_LOG_ERROR("MPD worker connection to failed: out-of-memory");
                mpd_worker_state->mpd_state->conn_state = MPD_FAILURE;
                mpd_connection_free(mpd_worker_state->mpd_state->conn);
                return;
            }

            if (mpd_connection_get_error(mpd_worker_state->mpd_state->conn) != MPD_ERROR_SUCCESS) {
                MYMPD_LOG_ERROR("MPD worker connection: %s", mpd_connection_get_error_message(mpd_worker_state->mpd_state->conn));
                mpd_worker_state->mpd_state->conn_state = MPD_FAILURE;
                return;
            }

            if (sdslen(mpd_worker_state->mpd_state->mpd_pass) > 0 && !mpd_run_password(mpd_worker_state->mpd_state->conn, mpd_worker_state->mpd_state->mpd_pass)) {
                MYMPD_LOG_ERROR("MPD worker connection: %s", mpd_connection_get_error_message(mpd_worker_state->mpd_state->conn));
                mpd_worker_state->mpd_state->conn_state = MPD_FAILURE;
                return;
            }

            MYMPD_LOG_NOTICE("MPD worker connected");
            //check version
            if (mpd_connection_cmp_server_version(mpd_worker_state->mpd_state->conn, 0, 20, 0) < 0) {
                MYMPD_LOG_EMERG("MPD version to old, myMPD supports only MPD version >= 0.20.0");
                mpd_worker_state->mpd_state->conn_state = MPD_TOO_OLD;               
                return;
            }

            mpd_connection_set_timeout(mpd_worker_state->mpd_state->conn, mpd_worker_state->mpd_state->timeout);
            mpd_worker_state->mpd_state->conn_state = MPD_CONNECTED;
            mpd_worker_state->mpd_state->reconnect_interval = 0;
            mpd_worker_state->mpd_state->reconnect_time = 0;
            
            mpd_worker_features(mpd_worker_state);
            
            //update database and sticker cache
            mpd_worker_cache_init(mpd_worker_state);
            
            if (!mpd_send_idle_mask(mpd_worker_state->mpd_state->conn, set_idle_mask)) {
                MYMPD_LOG_ERROR("MPD worker entering idle mode failed");
                mpd_worker_state->mpd_state->conn_state = MPD_FAILURE;
            }
            break;

        case MPD_FAILURE:
            MYMPD_LOG_ERROR("MPD worker connection failed");
            // fall through
        case MPD_DISCONNECT:
        case MPD_RECONNECT:
            if (mpd_worker_state->mpd_state->conn != NULL) {
                mpd_connection_free(mpd_worker_state->mpd_state->conn);
            }
            mpd_worker_state->mpd_state->conn = NULL;
            mpd_worker_state->mpd_state->conn_state = MPD_WAIT;
            if (mpd_worker_state->mpd_state->reconnect_interval <= 20) {
                mpd_worker_state->mpd_state->reconnect_interval += 2;
            }
            mpd_worker_state->mpd_state->reconnect_time = time(NULL) + mpd_worker_state->mpd_state->reconnect_interval;
            MYMPD_LOG_INFO("MPD worker waiting %u seconds before reconnection", mpd_worker_state->mpd_state->reconnect_interval);
            break;

        case MPD_CONNECTED:
            fds[0].fd = mpd_connection_get_fd(mpd_worker_state->mpd_state->conn);
            fds[0].events = POLLIN;
            pollrc = poll(fds, 1, 50);

            mpd_worker_queue_length = tiny_queue_length(mpd_worker_queue, 50);
            if (pollrc > 0 || mpd_worker_queue_length > 0) {
                MYMPD_LOG_DEBUG("Leaving mpd worker idle mode");
                if (!mpd_send_noidle(mpd_worker_state->mpd_state->conn)) {
                    check_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0);
                    mpd_worker_state->mpd_state->conn_state = MPD_FAILURE;
                    break;
                }
                if (pollrc > 0) {
                    //Handle idle events
                    MYMPD_LOG_DEBUG("Checking for idle events");
                    enum mpd_idle idle_bitmask = mpd_recv_idle(mpd_worker_state->mpd_state->conn, false);
                    mpd_worker_parse_idle(config, mpd_worker_state, idle_bitmask);

                }
                else {
                    mpd_response_finish(mpd_worker_state->mpd_state->conn);
                }
                if (mpd_worker_queue_length > 0) {
                    //Handle request
                    MYMPD_LOG_DEBUG("MPD worker handle request");
                    t_work_request *request = tiny_queue_shift(mpd_worker_queue, 50, 0);
                    if (request != NULL) {
                        mpd_worker_api(config, mpd_worker_state, request);
                    }
                }
                MYMPD_LOG_DEBUG("Entering mpd worker idle mode");
                if (!mpd_send_idle_mask(mpd_worker_state->mpd_state->conn, set_idle_mask)) {
                    check_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0);
                    mpd_worker_state->mpd_state->conn_state = MPD_FAILURE;
                }
            }
            break;
        default:
            MYMPD_LOG_ERROR("Invalid mpd worker connection state");
    }
}

static void mpd_worker_parse_idle(t_config *config, t_mpd_worker_state *mpd_worker_state, int idle_bitmask) {
    for (unsigned j = 0;; j++) {
        enum mpd_idle idle_event = 1 << j;
        const char *idle_name = mpd_idle_name(idle_event);
        if (idle_name == NULL) {
            break;
        }
        if (idle_bitmask & idle_event) {
            MYMPD_LOG_INFO("MPD idle event: %s", idle_name);
            switch(idle_event) {
                case MPD_IDLE_DATABASE:
                    mpd_worker_smartpls_update_all(config, mpd_worker_state, false);
                    mpd_worker_cache_init(mpd_worker_state);
                    break;
                default: {
                    //other idle events not used
                }
            }
        }
    }
}
