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

#include <pthread.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>

// Global variables
sig_atomic_t s_signal_received;  //!< Signal received indicator

// Private definitions
static const char *signal_name(unsigned signo);

// Public functions

/**
 * Initializes the signalfd and blocks the signals for the process.
 * @return the signalfd on success, else -1
 */
int signalfd_init(void) {
    sigset_t mask;
    if (sigemptyset(&mask) == -1) {
        MYMPD_LOG_ERROR(NULL, "sigemptyset failed");
        return false;
    }
    if (sigaddset(&mask, SIGTERM) == -1 ||
        sigaddset(&mask, SIGINT) == -1 ||
        sigaddset(&mask, SIGHUP) == -1)
    {
        MYMPD_LOG_ERROR(NULL, "sigaddset failed");
        return false;
    }
    // Block signals for the process so they are delivered to signalfd
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
        MYMPD_LOG_ERROR(NULL, "pthread_sigmask failed");
        return false;
    }
    return signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
}

/**
 * Closes the signalfd it is open
 * @param fd signalfd to close
 */
void signalfd_close(int fd) {
    if (fd > -1) {
        close(fd);
    }
}

/**
 * Handles the signalfd event
 * @param fd signalfd to read
 * @return false if the parent loop should exit, else true
 */
bool signalfd_handler(int fd) {
    struct signalfd_siginfo fdsi;
    ssize_t s = read(fd, &fdsi, sizeof(fdsi));
    if (s != sizeof(fdsi)) {
        MYMPD_LOG_EMERG(NULL, "Failed to read signalfd");
        return false;
    }
    switch (fdsi.ssi_signo) {
        case SIGTERM:
        case SIGINT:
            MYMPD_LOG_NOTICE(NULL, "Signal %s received, exiting", signal_name(fdsi.ssi_signo));
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
            MYMPD_LOG_DEBUG(NULL, "Ignoring signal %d", fdsi.ssi_signo);
            return true;
    }
}

// Private functions

/**
 * Looks up the name of a signal number
 * @param signo signal number
 * @return signal name as string literal
 */
static const char *signal_name(unsigned signo) {
    switch(signo) {
        case SIGTERM: return "SIGTERM";
        case SIGINT:  return "SIGINT";
        case SIGHUP:  return "SIGHUP";
        default:      return "UNKNOWN";
    }
}
