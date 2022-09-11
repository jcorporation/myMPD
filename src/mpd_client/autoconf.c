/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "autoconf.h"

#include "../lib/filehandler.h"
#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "mpd/connection.h"

#include <mpd/client.h>

#include <inttypes.h>
#include <string.h>

/**
 * Private definitions
 */
 static bool test_mpd_conn(const char *socket_path);

 /**
  * Public functions
  */

/**
 * Tries to autodetect the mpd connection configuration
 * https://mpd.readthedocs.io/en/latest/client.html#environment-variables
 * @param mympd_state pointer to mympd_state structure
 */
void mpd_client_autoconf(struct t_mympd_state *mympd_state) {
    //skip autoconfiguration if mpd_host state file is configured
    sds state_file = sdscatfmt(sdsempty(), "%S/state/mpd_host", mympd_state->config->workdir);
    if (testfile_read(state_file) == true) {
        MYMPD_LOG_NOTICE("Skipping myMPD autoconfiguration");
        FREE_SDS(state_file);
        return;
    }
    FREE_SDS(state_file);

    //autoconfigure mpd connection
    MYMPD_LOG_NOTICE("Starting myMPD autoconfiguration");
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
        if (test_mpd_conn(socket) == true) {
            MYMPD_LOG_NOTICE("Setting mpd host to \"%s\"", socket);
            mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, socket);
            FREE_SDS(socket);
            return;
        }
        FREE_SDS(socket);
    }
    if (test_mpd_conn("/run/mpd/socket") == true) {
        MYMPD_LOG_NOTICE("Setting mpd host to \"/run/mpd/socket\"");
        mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, "/run/mpd/socket");
        return;
    }
    if (test_mpd_conn("/var/run/mpd/socket") == true) {
        MYMPD_LOG_NOTICE("Setting mpd host to \"/var/run/mpd/socket\"");
        mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, "/var/run/mpd/socket");
        return;
    }
    if (test_mpd_conn("/var/lib/mpd/socket") == true) {
        //gentoo default 
        MYMPD_LOG_NOTICE("Setting mpd host to \"/var/lib/mpd/socket\"");
        mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, "/var/lib/mpd/socket");
        return;
    }
    //fallback to localhost:6600
    MYMPD_LOG_WARN("MPD autoconfiguration failed");
    MYMPD_LOG_NOTICE("Setting mpd host to \"%s\"", MYMPD_MPD_HOST);
    MYMPD_LOG_NOTICE("Setting mpd port to \"%d\"", MYMPD_MPD_PORT);
    mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, MYMPD_MPD_HOST);
    mympd_state->mpd_state->mpd_port = MYMPD_MPD_PORT;
}

 /**
  * Public functions
  */

/**
 * Tries a connection to mpd
 * @param socket_path mpd socket
 * @return true on success, else false
 */
static bool test_mpd_conn(const char *socket_path) {
    struct mpd_connection *conn = mpd_connection_new(socket_path, 0, MYMPD_MPD_TIMEOUT);
    if (conn == NULL) {
        return false;
    }
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        MYMPD_LOG_DEBUG("MPD connection: %s", mpd_connection_get_error_message(conn));
        mpd_connection_free(conn);
        return false;
    }
    mpd_connection_free(conn);
    return true;
}
