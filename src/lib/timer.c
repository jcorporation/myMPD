/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Timerfd helpers
 */

#include "compile_time.h"
#include "src/lib/timer.h"

#include "src/lib/datetime.h"
#include "src/lib/log.h"

#include <errno.h>
#include <sys/timerfd.h>
#include <unistd.h>

/**
 * Creates a new timer
 * @param clock one off CLOCK_MONOTONIC or CLOCK_REALTIME
 * @param timeout timeout in seconds
 * @param interval interval in seconds
 * @return timer fd
 */
int mympd_timer_create(int clock, int timeout, int interval) {
    int fd = timerfd_create(clock, TFD_NONBLOCK | TFD_CLOEXEC);
    errno = 0;
    if (fd == -1) {
        MYMPD_LOG_ERROR(NULL, "Unable to create timerfd");
        MYMPD_LOG_ERRNO(NULL, errno);
    }
    if (mympd_timer_set(fd, timeout, interval) == true) {
        return fd;
    }
    mympd_timer_close(fd);
    return -1;
}

/**
 * Reads from a timerfd
 * @param fd read from this fd
 * @return true on success, else false
 */
bool mympd_timer_read(int fd) {
    uint64_t exp;
    errno = 0;
    ssize_t s = read(fd, &exp, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) {
        MYMPD_LOG_ERROR(NULL, "Unable to read from timerfd");
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }
    return true;
}

/**
 * Sets the relative timeout and interval for a timer fd.
 * @param timer_fd timer fd
 * @param timeout relative timeout in seconds
 * @param interval interval in seconds
 * @return true on success, else false
 */
bool mympd_timer_set(int timer_fd, int timeout, int interval) {
    if (timer_fd == -1) {
        MYMPD_LOG_DEBUG(NULL, "Unable to set timeout, timerfd is closed");
        return false;
    }
    struct itimerspec its;
    its.it_value.tv_sec = timeout;
    its.it_value.tv_nsec = timeout == 0 && interval > 0 ? 1 : 0;
    its.it_interval.tv_sec = interval;
    its.it_interval.tv_nsec = 0;

    errno = 0;
    if (timerfd_settime(timer_fd, 0, &its, NULL) == -1) {
        MYMPD_LOG_ERROR(NULL, "Can not set expiration for timer");
        MYMPD_LOG_ERRNO(NULL, errno);
        close(timer_fd);
        return false;
    }
    return true;
}

/**
 * Logs the next timer expiration.
 * @param timer_fd timer fd
 */
void mympd_timer_log_next_expire(int timer_fd) {
    struct itimerspec its;
    errno = 0;
    if (timerfd_gettime(timer_fd, &its) == -1) {
        MYMPD_LOG_DEBUG(NULL, "Can not get expiration for timer");
        MYMPD_LOG_ERRNO(NULL, errno);
        return;
    }
    #ifdef MYMPD_DEBUG
        char fmt_time[32];
        readable_time(fmt_time, its.it_value.tv_sec);
        MYMPD_LOG_DEBUG(NULL, "Timer expires at %s", fmt_time);
    #else
        MYMPD_LOG_DEBUG(NULL, "Timer expires in %lld seconds", (long long)its.it_value.tv_sec);
    #endif
}

/**
 * Closes the timer
 * @param fd fd to close
 */
void mympd_timer_close(int fd) {
    if (fd > -1) {
        close(fd);
    }
}
