
/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Systemd helpers
 */

#include "compile_time.h"
#include "src/lib/log.h"
#include "src/lib/systemd.h"


#include <systemd/sd-daemon.h>

/**
 * Notify systemd
 */
void systemd_notify_ready(void) {
    if (log_type != LOG_TO_SYSTEMD) {
        return;
    }
    sd_notify(0, "READY=1");
}

/**
 * Notify systemd
 */
void systemd_notify_stopping(void) {
    if (log_type != LOG_TO_SYSTEMD) {
        return;
    }
    sd_notify(0, "STOPPING=1");
}
