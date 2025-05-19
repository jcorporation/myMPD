/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Configuration handling
 */

#include "compile_time.h"
#include "src/lib/signal.h"

#include "dist/mongoose/mongoose.h"
#include "src/lib/api.h"
#include "src/lib/event.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"

#include <pthread.h>
#include <signal.h>

sig_atomic_t s_signal_received;  //!< Signal received indicator

// Private definitions
static void mympd_signal_handler(int sig_num);

// Public functions

/**
 * Sets the mympd_signal_handler for the given signal
 * @param sig_num signal to handle
 * @return true on success, else false
 */
bool set_signal_handler(int sig_num) {
    struct sigaction sa;
    sa.sa_handler = mympd_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart functions if interrupted by handler
    if (sigaction(sig_num, &sa, NULL) == -1) {
        return false;
    }
    return true;
}

// Private functions

/**
 * Signal handler that stops myMPD on SIGTERM and SIGINT and saves
 * states on SIGHUP
 * @param sig_num the signal to handle
 */
static void mympd_signal_handler(int sig_num) {
    switch(sig_num) {
        case SIGTERM:
        case SIGINT: {
            MYMPD_LOG_NOTICE(NULL, "Signal \"%s\" received, exiting", (sig_num == SIGTERM ? "SIGTERM" : "SIGINT"));
            //Set loop end condition for threads
            s_signal_received = sig_num;
            //Wakeup queue loops
            pthread_cond_signal(&mympd_api_queue->wakeup);
            #ifdef MYMPD_ENABLE_LUA
                pthread_cond_signal(&script_queue->wakeup);
                pthread_cond_signal(&script_worker_queue->wakeup);
            #endif
            pthread_cond_signal(&webserver_queue->wakeup);
            event_eventfd_write(mympd_api_queue->event_fd);
            if (webserver_queue->mg_mgr != NULL) {
                mg_wakeup(webserver_queue->mg_mgr, webserver_queue->mg_conn_id, "X", 1);
            }
            break;
        }
        case SIGHUP: {
            MYMPD_LOG_NOTICE(NULL, "Signal SIGHUP received, saving states");
            struct t_work_request *request1 = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_STATE_SAVE, "", MPD_PARTITION_DEFAULT);
            mympd_queue_push(mympd_api_queue, request1, 0);
            #ifdef MYMPD_ENABLE_LUA
                struct t_work_request *request2 = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_STATE_SAVE, "", MPD_PARTITION_DEFAULT);
                mympd_queue_push(script_queue, request2, 0);
            #endif
            break;
        }
        default: {
            //Other signals are not handled
        }
    }
}
