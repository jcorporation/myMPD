/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD connection autoconfiguration
 */

#include "compile_time.h"
#include "src/mpd_client/autoconf.h"

#include "dist/libmympdclient/include/mpd/client.h"
#include "src/lib/env.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/validate.h"

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
    sds state_file = sdscatfmt(sdsempty(), "%S/%s/mpd_host", mympd_state->config->workdir, DIR_WORK_STATE);
    if (testfile_read(state_file) == true) {
        MYMPD_LOG_NOTICE(NULL, "Skipping myMPD autoconfiguration");
        FREE_SDS(state_file);
        return;
    }
    FREE_SDS(state_file);

    //autoconfigure mpd connection
    MYMPD_LOG_NOTICE(NULL, "Starting myMPD autoconfiguration");
    MYMPD_LOG_NOTICE(NULL, "Reading environment");
    bool mpd_configured = false;

    sds mpd_host = getenv_string("MPD_HOST", MYMPD_MPD_HOST, vcb_isname);
    if (strcmp(mpd_host, mympd_state->mpd_state->mpd_host) != 0) {
        if (mpd_host[0] != '@' &&
            strstr(mpd_host, "@") != NULL)
        {
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
        MYMPD_LOG_NOTICE(NULL, "Setting mpd host to \"%s\"", mympd_state->mpd_state->mpd_host);
        mpd_configured = true;
    }
    FREE_SDS(mpd_host);

    unsigned mpd_port = getenv_uint("MPD_PORT", MYMPD_MPD_PORT, MPD_PORT_MIN, MPD_PORT_MAX);
    if (mpd_port != mympd_state->mpd_state->mpd_port) {
        mympd_state->mpd_state->mpd_port = mpd_port;
        MYMPD_LOG_NOTICE(NULL, "Setting mpd port to \"%d\"", mympd_state->mpd_state->mpd_port);
    }

    unsigned timeout = getenv_uint("MPD_TIMEOUT", MYMPD_MPD_TIMEOUT_SEC, MPD_TIMEOUT_MIN, MPD_TIMEOUT_MAX);
    timeout = timeout * 1000; //convert to ms
    if (timeout != mympd_state->mpd_state->mpd_timeout) {
        mympd_state->mpd_state->mpd_timeout = timeout;
        MYMPD_LOG_NOTICE(NULL, "Setting mpd timeout to \"%d\"", mympd_state->mpd_state->mpd_timeout);
    }

    if (mpd_configured == true) {
        return;
    }

    //check for socket
    sds xdg_runtime_dir = getenv_string("XDG_RUNTIME_DIR", "", vcb_isfilepath);
    if (sdslen(xdg_runtime_dir) > 0) {
        sds socket = sdscatfmt(sdsempty(), "%s/mpd/socket", xdg_runtime_dir);
        if (test_mpd_conn(socket) == true) {
            MYMPD_LOG_NOTICE(NULL, "Setting mpd host to \"%s\"", socket);
            mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, socket);
            FREE_SDS(socket);
            FREE_SDS(xdg_runtime_dir);
            return;
        }
        FREE_SDS(socket);
    }
    FREE_SDS(xdg_runtime_dir);

    const char *test_sockets[] = {
        "/run/mpd/socket",
        "/var/run/mpd/socket",
        "/var/lib/mpd/socket", //Gentoo default
        NULL
    };

    const char **p = test_sockets;
    while (*p != NULL) {
        if (test_mpd_conn(*p) == true) {
            MYMPD_LOG_NOTICE(NULL, "Setting mpd host to \"%s\"", *p);
            mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, *p);
            return;
        }
        p++;
    }

    //fallback to localhost:6600
    MYMPD_LOG_WARN(NULL, "MPD autoconfiguration failed");
    MYMPD_LOG_NOTICE(NULL, "Setting mpd host to \"%s\"", MYMPD_MPD_HOST);
    MYMPD_LOG_NOTICE(NULL, "Setting mpd port to \"%d\"", MYMPD_MPD_PORT);
    mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, MYMPD_MPD_HOST);
    mympd_state->mpd_state->mpd_port = MYMPD_MPD_PORT;

    // copy config to stickerdb connection
    mympd_state->stickerdb->mpd_state->mpd_host = sds_replace(mympd_state->stickerdb->mpd_state->mpd_host, mympd_state->mpd_state->mpd_host);
    mympd_state->stickerdb->mpd_state->mpd_port = mympd_state->mpd_state->mpd_port;
    mympd_state->stickerdb->mpd_state->mpd_timeout = mympd_state->mpd_state->mpd_timeout;
    mympd_state->stickerdb->mpd_state->mpd_keepalive = mympd_state->mpd_state->mpd_keepalive;
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
        MYMPD_LOG_DEBUG(NULL, "MPD connection \"%s\": %s", socket_path, mpd_connection_get_error_message(conn));
        mpd_connection_free(conn);
        return false;
    }
    mpd_connection_free(conn);
    return true;
}
