/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "autoconf.h"

#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
 * Tries to autodetect the mpd connection configuration
 * https://mpd.readthedocs.io/en/latest/client.html#environment-variables
 * @param mympd_state pointer to mympd_state structure
 */
void mpd_client_autoconf(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_NOTICE("Reading environment");
    bool mpd_configured = false;
    const char *mpd_host_env = getenv_check("MPD_HOST", 100);
    if (mpd_host_env != NULL) {
        sds mpd_host = sdsnew(mpd_host_env);
        if (vcb_isname(mpd_host) == true) {
            if (mpd_host[0] != '@' && strstr(mpd_host, "@") != NULL) {
                int count = 0;
                sds *tokens = sdssplitlen(mpd_host, (ssize_t)sdslen(mpd_host), "@", 1, &count);
                mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, tokens[1]);
                mympd_state->mpd_state->mpd_pass = sds_replace(mympd_state->mpd_state->mpd_pass, tokens[0]);
                sdsfreesplitres(tokens,count);
            }
            else {
                //no password
                mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, mpd_host);
            }
            MYMPD_LOG_NOTICE("Setting mpd host to \"%s\"", mympd_state->mpd_state->mpd_host);
            mpd_configured = true;
        }
        FREE_SDS(mpd_host);
    }
    const char *mpd_port_env = getenv_check("MPD_PORT", 5);
    if (mpd_port_env != NULL) {
        sds mpd_port = sdsnew(mpd_port_env);
        if (vcb_isdigit(mpd_port) == true) {
            unsigned port = (unsigned)strtoumax(mpd_port, NULL, 10);
            if (port == 0) {
                mympd_state->mpd_state->mpd_port = 6600;
                MYMPD_LOG_NOTICE("Setting mpd port to \"%d\"", mympd_state->mpd_state->mpd_port);
            }
            else if (port > MPD_PORT_MIN && port <= MPD_PORT_MAX) {
                mympd_state->mpd_state->mpd_port = port;
                MYMPD_LOG_NOTICE("Setting mpd port to \"%d\"", mympd_state->mpd_state->mpd_port);
            }
            else {
                MYMPD_LOG_WARN("MPD port must be between 1024 and 65534, default is 6600");
            }
        }
        FREE_SDS(mpd_port);
    }
    const char *mpd_timeout_env = getenv_check("MPD_TIMEOUT", 5);
    if (mpd_timeout_env != NULL) {
        sds mpd_timeout = sdsnew(mpd_timeout_env);
        if (vcb_isdigit(mpd_timeout) == true) {
            unsigned timeout = (unsigned)strtoumax(mpd_timeout, NULL, 10);
            timeout = timeout * 1000; //convert to ms
            if (timeout >= MPD_TIMEOUT_MIN && timeout < MPD_TIMEOUT_MAX) {
                mympd_state->mpd_state->mpd_timeout = timeout;
                MYMPD_LOG_NOTICE("Setting mpd timeout to \"%d\"", mympd_state->mpd_state->mpd_timeout);
            }
            else {
                MYMPD_LOG_WARN("MPD timeout must be between %d and %d", MPD_TIMEOUT_MIN, MPD_TIMEOUT_MAX);
            }
        }
        FREE_SDS(mpd_timeout);
    }
    if (mpd_configured == true) {
        return;
    }

    //check for socket
    const char *xdg_runtime_dir = getenv_check("XDG_RUNTIME_DIR", 100);
    if (xdg_runtime_dir != NULL) {
        sds socket = sdscatfmt(sdsempty(), "%s/mpd/socket", xdg_runtime_dir);
        if (access(socket, F_OK ) == 0) { /* Flawfinder: ignore */
            mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, socket);
            FREE_SDS(socket);
            return;
        }
        FREE_SDS(socket);
    }
    if (access("/run/mpd/socket", F_OK ) == 0) { /* Flawfinder: ignore */
        mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, "/run/mpd/socket");
        return;
    }
    if (access("/var/run/mpd/socket", F_OK ) == 0) { /* Flawfinder: ignore */
        mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, "/var/run/mpd/socket");
        return;
    }
    if (access("/var/lib/mpd/socket", F_OK ) == 0) { /* Flawfinder: ignore */
        //gentoo default 
        mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, "/var/lib/mpd/socket");
        return;
    }
    //fallback to localhost:6600
    mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, MYMPD_MPD_HOST);
    mympd_state->mpd_state->mpd_port = MYMPD_MPD_PORT;
}
