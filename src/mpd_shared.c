/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
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
#include "config_defs.h"
#include "tiny_queue.h"
#include "api.h"
#include "global.h"
#include "utility.h"
#include "log.h"
#include "mpd_shared.h"

void mpd_shared_default_mpd_state(t_mpd_state *mpd_state) {
    mpd_state->conn_state = MPD_DISCONNECTED;
    mpd_state->reconnect_time = 0;
    mpd_state->reconnect_interval = 0;
    mpd_state->timeout = 10000;
    mpd_state->state = MPD_STATE_UNKNOWN;
    mpd_state->mpd_host = sdsempty();
    mpd_state->mpd_port = 0;
    mpd_state->mpd_pass = sdsempty();
    reset_t_tags(&mpd_state->mympd_tag_types);
}

void mpd_shared_free_mpd_state(t_mpd_state *mpd_state) {
    sdsfree(mpd_state->mpd_host);
    sdsfree(mpd_state->mpd_pass);
    free(mpd_state);
    mpd_state = NULL;
}

void mpd_shared_mpd_disconnect(t_mpd_state *mpd_state) {
    mpd_state->conn_state = MPD_DISCONNECT;
    if (mpd_state->conn != NULL) {
        mpd_connection_free(mpd_state->conn);
    }
}

bool mpd_shared_feat_mpd_searchwindow(t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 20, 0) >= 0) {
        return true;
    }

    LOG_WARN("Disabling searchwindow support, depends on mpd >= 0.20.0");
    return false;
}

bool mpd_shared_feat_tags(t_mpd_state *mpd_state) {
    bool feat_tags = false;
    
    if (mpd_send_list_tag_types(mpd_state->conn) == true) {
        struct mpd_pair *pair;
        while ((pair = mpd_recv_tag_type_pair(mpd_state->conn)) != NULL) {
            mpd_return_pair(mpd_state->conn, pair);
            feat_tags = true;
            break;
        }
    }
    else {
        LOG_ERROR("Error in response to command: mpd_send_list_tag_types");
    }
    mpd_response_finish(mpd_state->conn);
    check_error_and_recover2(mpd_state, NULL, NULL, 0, false);
    return feat_tags;
}

bool check_rc_error_and_recover(t_mpd_state *mpd_state, sds *buffer, 
                                sds method, int request_id, bool notify, bool rc, const char *command)
{
    if (check_error_and_recover2(mpd_state, buffer, method, request_id, notify) == false) {
        LOG_ERROR("Error in response to command %s", command);
        return false;
    }
    if (rc == false) {
        //todo: implement notify jsonrpc message on demand
        if (buffer != NULL && *buffer != NULL) {
            *buffer = respond_with_command_error(*buffer, method, request_id, command);
        }
        LOG_ERROR("Error in response to command %s", command);
        return false;
    }
    return true;
}

bool check_error_and_recover2(t_mpd_state *mpd_state, sds *buffer, sds method, int request_id, bool notify) {
    enum mpd_error error = mpd_connection_get_error(mpd_state->conn);
    if (error  != MPD_ERROR_SUCCESS) {
        const char *error_msg = mpd_connection_get_error_message(mpd_state->conn);
        LOG_ERROR("MPD error: %s (%d)", error_msg , error);
        if (buffer != NULL) {
            if (*buffer != NULL) {
                if (notify == false) {
                    *buffer = jsonrpc_respond_message(*buffer, method, request_id, mpd_connection_get_error_message(mpd_state->conn), true);
                }
                else {
                    *buffer = jsonrpc_start_notify(*buffer, "error");
                    *buffer = tojson_char(*buffer, "message", mpd_connection_get_error_message(mpd_state->conn), false);
                    *buffer = jsonrpc_end_notify(*buffer);
                }
            }
        }

        if (error == 8 || //Connection closed by the server
            error == 5) { //Broken pipe
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

sds check_error_and_recover(t_mpd_state *mpd_state, sds buffer, sds method, int request_id) {
    check_error_and_recover2(mpd_state, &buffer, method, request_id, false);
    return buffer;
}

sds check_error_and_recover_notify(t_mpd_state *mpd_state, sds buffer) {
    check_error_and_recover2(mpd_state, &buffer, NULL, 0, true);
    return buffer;
}

sds respond_with_command_error(sds buffer, sds method, int request_id, const char *command) {
    buffer = sdscrop(buffer);
    buffer = jsonrpc_start_phrase(buffer, method, request_id, "Error in response to command: %{command}", true);
    buffer = tojson_char(buffer, "command", command, false);
    buffer = jsonrpc_end_phrase(buffer);
    return buffer;
}

sds respond_with_mpd_error_or_ok(t_mpd_state *mpd_state, sds buffer, sds method, int request_id, bool rc, const char *command) {
    buffer = sdscrop(buffer);
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        LOG_ERROR("Error in response to command: %s", command);
        return buffer;
    }
    if (rc == false) {
        LOG_ERROR("Error in response to command: %s", command);
        return respond_with_command_error(buffer, method, request_id, command);
    }
    return jsonrpc_respond_ok(buffer, method, request_id);
}

void reset_t_tags(t_tags *tags) {
    tags->len = 0;
    memset(tags->tags, 0, sizeof(tags->tags));
}

void disable_all_mpd_tags(t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        LOG_DEBUG("Disabling all mpd tag types");
        bool rc = mpd_run_clear_tag_types(mpd_state->conn);
        check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_clear_tag_types");
    }
}

void enable_all_mpd_tags(t_mpd_state *mpd_state) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        LOG_DEBUG("Enabling all mpd tag types");
        bool rc = mpd_run_all_tag_types(mpd_state->conn);
        check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_run_all_tag_types");
    }
}

void enable_mpd_tags(t_mpd_state *mpd_state, t_tags enable_tags) {
    if (mpd_connection_cmp_server_version(mpd_state->conn, 0, 21, 0) >= 0) {
        LOG_DEBUG("Setting interesting mpd tag types");
        if (mpd_command_list_begin(mpd_state->conn, false)) {
            bool rc = mpd_send_clear_tag_types(mpd_state->conn);
            if (rc == false) {
                LOG_ERROR("Error adding command to command list mpd_send_clear_tag_types");
            }
            if (enable_tags.len > 0) {
                rc = mpd_send_enable_tag_types(mpd_state->conn, enable_tags.tags, enable_tags.len);
                if (rc == false) {
                    LOG_ERROR("Error adding command to command list mpd_send_enable_tag_types");
                }
            }
            if (mpd_command_list_end(mpd_state->conn)) {
                mpd_response_finish(mpd_state->conn);
            }
        }
        check_error_and_recover(mpd_state, NULL, NULL, 0);
    }
}
