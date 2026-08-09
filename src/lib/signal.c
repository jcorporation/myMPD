/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Signal handling
 */

#include "compile_time.h"
#include "src/lib/signal.h"

#include "src/lib/api.h"
#include "src/lib/event.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>

// Global variables
sig_atomic_t s_signal_received;  //!< Signal received indicator

// Private definitions
static const char *signal_name(int signo);
static void signal_handler(int signo);

static int signal_eventfd = -1; //!< Eventfd for signal handling

// Public functions

/**
 * Initializes the eventfd and installs a minimal signal handler for SIGTERM, SIGINT, and SIGHUP.
 * @return true on success, else false
 */
bool signal_eventfd_init(void) {
    // Create an eventfd for signal handling
    signal_eventfd = event_eventfd_create_simple();
    if (signal_eventfd == -1) {
        return false;
    }

    // Install signal handlers for SIGTERM, SIGINT, and SIGHUP
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart functions if interrupted by handler

    // Ignore SIGPIPE to prevent termination when writing to closed sockets
    struct sigaction sa_ignore;
    memset(&sa_ignore, 0, sizeof(sa_ignore));
    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = SA_RESTART; // Restart functions if interrupted by handler

    if (sigaction(SIGTERM, &sa, NULL) == -1 ||
        sigaction(SIGINT, &sa, NULL) == -1 ||
        sigaction(SIGHUP, &sa, NULL) == -1 ||
        sigaction(SIGPIPE, &sa_ignore, NULL) == -1)
    {
        MYMPD_LOG_ERROR(NULL, "sigaction failed");
        signal_eventfd_close();
        return false;
    }

    return true;
}

/**
 * Returns the signal eventfd.
 * @return the signal eventfd
 */
int signal_eventfd_get(void) {
    return signal_eventfd;
}

/**
 * Handles the signal_eventfd event.
 * @return false if the parent loop should exit, else true
 */
bool signal_eventfd_handler(void) {
    // Process the pending signal
    int signo = (int)event_eventfd_read_number(signal_eventfd);
    if (errno != 0) {
        return false;
    }

    switch (signo) {
        case SIGTERM:
        case SIGINT:
            MYMPD_LOG_NOTICE(NULL, "Signal %s received, exiting", signal_name(signo));
            // Set loop break condition
            s_signal_received = 1;
            // Wakeup threads
            pthread_cond_signal(&mympd_api_queue->wakeup);
            #ifdef MYMPD_ENABLE_LUA
                pthread_cond_signal(&script_queue->wakeup);
                pthread_cond_signal(&script_worker_queue->wakeup);
            #endif
            pthread_cond_signal(&webserver_queue->wakeup);
            event_eventfd_write(mympd_api_queue->event_fd);
            if (webserver_queue->mg_mgr != NULL) {
                mympd_mg_wakeup_send("X");
            }
            return false;
        case SIGHUP:
            MYMPD_LOG_NOTICE(NULL, "Signal SIGHUP received, saving states");
            struct t_work_request *request1 = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_STATE_SAVE, "", MPD_PARTITION_DEFAULT);
            mympd_queue_push(mympd_api_queue, request1, 0);
            #ifdef MYMPD_ENABLE_LUA
                struct t_work_request *request2 = create_request(REQUEST_TYPE_DISCARD, 0, 0, INTERNAL_API_STATE_SAVE, "", MPD_PARTITION_DEFAULT);
                mympd_queue_push(script_queue, request2, 0);
            #endif
            return true;
        default:
            // Ignore other signals
            MYMPD_LOG_DEBUG(NULL, "Ignoring signal %d", signo);
            return true;
    }
}

/**
 * Minimal signal handler that writes to the eventfd to wake up the main loop.
 * @param signo signal number
 */
static void signal_handler(int signo) {
    if (signal_eventfd > -1) {
        // Store the signal number in a volatile variable to be processed later
        if (eventfd_write(signal_eventfd, (eventfd_t)signo) != 0) {
            // Do nothing, we are in a signal handler and cannot log errors
        }
    }
}

/**
 * Closes the eventfd and sets it to -1.
 */
void signal_eventfd_close(void) {
    event_fd_close(signal_eventfd);
    signal_eventfd = -1;
}

// Private functions

/**
 * Looks up the name of a signal number
 * @param signo signal number
 * @return signal name as string literal
 */
static const char *signal_name(int signo) {
    switch(signo) {
        case SIGTERM: return "SIGTERM";
        case SIGINT:  return "SIGINT";
        case SIGHUP:  return "SIGHUP";
        case SIGPIPE: return "SIGPIPE";
        default:      return "UNKNOWN";
    }
}
