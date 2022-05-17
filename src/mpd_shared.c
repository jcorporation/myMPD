/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_shared.h"

#include "lib/jsonrpc.h"
#include "lib/log.h"
#include "lib/mem.h"
#include "lib/sds_extras.h"
#include "mpd_shared/mpd_shared_tags.h"

#include <stdlib.h>

//mpd state
void mpd_shared_default_mpd_state(struct t_mpd_state *mpd_state) {
    mpd_state->conn = NULL;
    mpd_state->conn_state = MPD_DISCONNECTED;
    mpd_state->reconnect_time = 0;
    mpd_state->reconnect_interval = 0;
    mpd_state->mpd_keepalive = MYMPD_MPD_KEEPALIVE;
    mpd_state->mpd_timeout = MYMPD_MPD_TIMEOUT;
    mpd_state->state = MPD_STATE_UNKNOWN;
    mpd_state->mpd_host = sdsnew(MYMPD_MPD_HOST);
    mpd_state->mpd_port = MYMPD_MPD_PORT;
    mpd_state->mpd_pass = sdsnew(MYMPD_MPD_PASS);
    mpd_state->mpd_binarylimit = MYMPD_MPD_BINARYLIMIT;
    mpd_state->song_id = -1;
    mpd_state->song_uri = sdsempty();
    mpd_state->next_song_id = -1;
    mpd_state->last_song_id = -1;
    mpd_state->last_song_uri = sdsempty();
    mpd_state->queue_version = 0;
    mpd_state->queue_length = 0;
    mpd_state->last_last_played_id = -1;
    mpd_state->song_end_time = 0;
    mpd_state->song_start_time = 0;
    mpd_state->last_song_end_time = 0;
    mpd_state->last_song_start_time = 0;
    mpd_state->last_skipped_id = 0;
    mpd_state->set_song_played_time = 0;
    mpd_state->last_song_set_song_played_time = 0;
    mpd_state->crossfade = 0;
    mpd_state->set_song_played_time = 0;
    mpd_state->tag_list = sdsnew(MYMPD_MPD_TAG_LIST);
    reset_t_tags(&mpd_state->tag_types_mympd);
    reset_t_tags(&mpd_state->tag_types_mpd);
    mpd_state->tag_albumartist = MPD_TAG_ALBUM_ARTIST;
}

void mpd_shared_free_mpd_state(struct t_mpd_state *mpd_state) {
    FREE_SDS(mpd_state->mpd_host);
    FREE_SDS(mpd_state->mpd_pass);
    FREE_SDS(mpd_state->song_uri);
    FREE_SDS(mpd_state->last_song_uri);
    FREE_SDS(mpd_state->tag_list);
    FREE_PTR(mpd_state);
}

void mpd_shared_mpd_disconnect(struct t_mpd_state *mpd_state) {
    mpd_state->conn_state = MPD_DISCONNECT;
    if (mpd_state->conn != NULL) {
        mpd_connection_free(mpd_state->conn);
    }
}

bool check_rc_error_and_recover(struct t_mpd_state *mpd_state, sds *buffer,
                                sds method, long request_id, bool notify, bool rc,
                                const char *command)
{
    if (check_error_and_recover2(mpd_state, buffer, method, request_id, notify) == false) {
        MYMPD_LOG_ERROR("Error in response to command %s", command);
        return false;
    }
    if (rc == false) {
        if (buffer != NULL && *buffer != NULL) {
            if (notify == false) {
                *buffer = respond_with_command_error(*buffer, method, request_id, command);
            }
            else {
                *buffer = jsonrpc_notify_phrase(*buffer, "mpd", "error", "Error in response to command: %{command}",
                            2, "command", command);
            }
        }
        MYMPD_LOG_ERROR("Error in response to command %s", command);
        return false;
    }
    return true;
}

bool check_error_and_recover2(struct t_mpd_state *mpd_state, sds *buffer, sds method, long request_id,
                              bool notify)
{
    enum mpd_error error = mpd_connection_get_error(mpd_state->conn);
    if (error != MPD_ERROR_SUCCESS) {
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
        //try to recover from error
        bool recovered = mpd_connection_clear_error(mpd_state->conn);
        if (recovered == false) {
            mpd_state->conn_state = MPD_FAILURE;
        }
        else {
            mpd_response_finish(mpd_state->conn);
            //enable default mpd tags after cleaning error
            enable_mpd_tags(mpd_state, &mpd_state->tag_types_mympd);
        }
        return false;
    }
    return true;
}

sds check_error_and_recover(struct t_mpd_state *mpd_state, sds buffer, sds method, long request_id) {
    check_error_and_recover2(mpd_state, &buffer, method, request_id, false);
    return buffer;
}

sds check_error_and_recover_notify(struct t_mpd_state *mpd_state, sds buffer) {
    check_error_and_recover2(mpd_state, &buffer, NULL, 0, true);
    return buffer;
}

sds respond_with_command_error(sds buffer, sds method, long request_id, const char *command) {
    return jsonrpc_respond_message_phrase(buffer, method, request_id,
        true, "mpd", "error", "Error in response to command: %{command}",
        2, "command", command);
}

sds respond_with_mpd_error_or_ok(struct t_mpd_state *mpd_state, sds buffer, sds method,
                                 long request_id, bool rc, const char *command, bool *result)
{
    sdsclear(buffer);
    *result = check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false,
                                   rc, command);
    if (*result == false) {
        return buffer;
    }
    return jsonrpc_respond_ok(buffer, method, request_id, "mpd");
}

bool mpd_shared_set_keepalive(struct t_mpd_state *mpd_state) {
    bool rc = mpd_connection_set_keepalive(mpd_state->conn, mpd_state->mpd_keepalive);
    return check_rc_error_and_recover(mpd_state, NULL, NULL, 0, false, rc, "mpd_connection_set_keepalive");
}
