/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <libgen.h>
#include <pthread.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "list.h"
#include "mympd_config_defs.h"
#include "tiny_queue.h"
#include "api.h"
#include "global.h"
#include "utility.h"
#include "log.h"
#include "mpd_shared/mpd_shared_typedefs.h"
#include "mpd_shared/mpd_shared_tags.h"
#include "mpd_shared.h"

//mpd state
void mpd_shared_default_mpd_state(t_mpd_state *mpd_state) {
    mpd_state->conn_state = MPD_DISCONNECTED;
    mpd_state->reconnect_time = 0;
    mpd_state->reconnect_interval = 0;
    mpd_state->timeout = 10000;
    mpd_state->state = MPD_STATE_UNKNOWN;
    mpd_state->mpd_host = sdsempty();
    mpd_state->mpd_port = 0;
    mpd_state->mpd_pass = sdsempty();
    mpd_state->taglist = sdsempty();
    reset_t_tags(&mpd_state->mympd_tag_types);
    reset_t_tags(&mpd_state->mpd_tag_types);
}

void mpd_shared_free_mpd_state(t_mpd_state *mpd_state) {
    sdsfree(mpd_state->mpd_host);
    sdsfree(mpd_state->mpd_pass);
    sdsfree(mpd_state->taglist);
    free(mpd_state);
}

void mpd_shared_mpd_disconnect(t_mpd_state *mpd_state) {
    mpd_state->conn_state = MPD_DISCONNECT;
    if (mpd_state->conn != NULL) {
        mpd_connection_free(mpd_state->conn);
    }
}

bool check_rc_error_and_recover(t_mpd_state *mpd_state, sds *buffer, 
                                sds method, long request_id, bool notify, bool rc, const char *command)
{
    if (check_error_and_recover2(mpd_state, buffer, method, request_id, notify) == false) {
        MYMPD_LOG_ERROR("Error in response to command %s", command);
        return false;
    }
    if (rc == false) {
        //TODO: implement notify jsonrpc message on demand
        if (buffer != NULL && *buffer != NULL) {
            *buffer = respond_with_command_error(*buffer, method, request_id, command);
        }
        MYMPD_LOG_ERROR("Error in response to command %s", command);
        return false;
    }
    return true;
}

bool check_error_and_recover2(t_mpd_state *mpd_state, sds *buffer, sds method, long request_id, bool notify) {
    enum mpd_error error = mpd_connection_get_error(mpd_state->conn);
    if (error  != MPD_ERROR_SUCCESS) {
        const char *error_msg = mpd_connection_get_error_message(mpd_state->conn);
        MYMPD_LOG_ERROR("MPD error: %s (%d)", error_msg , error);
        if (buffer != NULL && *buffer != NULL) {
            if (notify == false) {
                *buffer = jsonrpc_respond_message(*buffer, method, request_id, true, 
                    "mpd", "error", error_msg);
            }
            else {
                *buffer = jsonrpc_notify(*buffer, "mpd", "error", error_msg);
            }
        }

        if (error == 8 || //Connection closed by the server
            error == 5 || //Broken pipe 
            error == 4    //Timeout
        ) { 
            mpd_state->conn_state = MPD_FAILURE;
        }
        mpd_connection_clear_error(mpd_state->conn);
        if (mpd_state->conn_state != MPD_FAILURE) {
            mpd_response_finish(mpd_state->conn);
            //enable default mpd tags after cleaning error
            enable_mpd_tags(mpd_state, mpd_state->mympd_tag_types);
        }
        return false;
    }
    return true;
}

sds check_error_and_recover(t_mpd_state *mpd_state, sds buffer, sds method, long request_id) {
    check_error_and_recover2(mpd_state, &buffer, method, request_id, false);
    return buffer;
}

sds check_error_and_recover_notify(t_mpd_state *mpd_state, sds buffer) {
    check_error_and_recover2(mpd_state, &buffer, NULL, 0, true);
    return buffer;
}

sds respond_with_command_error(sds buffer, sds method, long request_id, const char *command) {
    return jsonrpc_respond_message_phrase(buffer, method, request_id, 
                            true, "mpd", "error", "Error in response to command: %{command}",
                            2, "command", command);
}

sds respond_with_mpd_error_or_ok(t_mpd_state *mpd_state, sds buffer, sds method, long request_id, bool rc, const char *command) {
    buffer = sdscrop(buffer);
    if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, command) == false) {
        return buffer;
    }
    return jsonrpc_respond_ok(buffer, method, request_id, "mpd");
}
