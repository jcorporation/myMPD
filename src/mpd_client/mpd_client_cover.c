/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../dist/src/rax/rax.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "../mympd_state.h"
#include "../mpd_shared.h"
#include "mympd_config_defs.h"
#include "../utility.h"
#include "../covercache.h"
#include "mpd_client_utility.h"
#include "mpd_client_cover.h"

sds mpd_client_getcover(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                        const char *uri, sds *binary)
{
    unsigned offset = 0;
    void *binary_buffer = malloc(mympd_state->mpd_state->binarylimit);
    int recv_len = 0;
    if (mympd_state->mpd_state->feat_mpd_albumart == true) {
        MYMPD_LOG_DEBUG("Try mpd command albumart for \"%s\"", uri);
        while ((recv_len = mpd_run_albumart(mympd_state->mpd_state->conn, uri, offset, binary_buffer, mympd_state->mpd_state->binarylimit)) > 0) {
            *binary = sdscatlen(*binary, binary_buffer, recv_len);
            offset += recv_len;
        }
    }
    if (offset == 0 && mympd_state->mpd_state->feat_mpd_readpicture == true) {
        MYMPD_LOG_DEBUG("Try mpd command readpicture for \"%s\"", uri);
        while ((recv_len = mpd_run_readpicture(mympd_state->mpd_state->conn, uri, offset, binary_buffer, mympd_state->mpd_state->binarylimit)) > 0) {
            *binary = sdscatlen(*binary, binary_buffer, recv_len);
            offset += recv_len;
        }
    }
    if (recv_len == -1) {
        //silently clear the error if no albumart is found
        mpd_connection_clear_error(mympd_state->mpd_state->conn);
        mpd_response_finish(mympd_state->mpd_state->conn);
    }
    free(binary_buffer);
    if (offset > 0) {
        MYMPD_LOG_DEBUG("Albumart found by mpd for uri \"%s\"", uri);
        sds mime_type = get_mime_type_by_magic_stream(*binary);
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = tojson_char(buffer, "mime_type", mime_type, false);
        buffer = jsonrpc_result_end(buffer);
        if (mympd_state->config->covercache == true) {
            write_covercache_file(mympd_state->config, uri, mime_type, *binary);
        }
        sdsfree(mime_type);
    }
    else {
        MYMPD_LOG_DEBUG("No albumart found by mpd for uri \"%s\"", uri);
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "albumart", "warn", "No albumart found by mpd");
    }
    return buffer;
}
