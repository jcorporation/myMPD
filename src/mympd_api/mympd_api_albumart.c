/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_albumart.h"

#include "../lib/covercache.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mimetype.h"
#include "../lib/mympd_configuration.h"

#include <stdlib.h>

sds mympd_api_albumart_getcover(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                        const char *uri, sds *binary)
{
    unsigned offset = 0;
    void *binary_buffer = malloc_assert(mympd_state->mpd_state->mpd_binarylimit);
    int recv_len = 0;
    if (mympd_state->mpd_state->feat_mpd_albumart == true) {
        MYMPD_LOG_DEBUG("Try mpd command albumart for \"%s\"", uri);
        while ((recv_len = mpd_run_albumart(mympd_state->mpd_state->conn, uri, offset, binary_buffer, mympd_state->mpd_state->mpd_binarylimit)) > 0) {
            MYMPD_LOG_DEBUG("Received %d bytes from mpd albumart command", recv_len);
            *binary = sdscatlen(*binary, binary_buffer, (size_t)recv_len);
            if (sdslen(*binary) > MPD_BINARY_SIZE_MAX) {
                MYMPD_LOG_WARN("Retrieved binary data is too large, discarding");
                sdsclear(*binary);
                offset = 0;
                break;
            }
            offset += (unsigned)recv_len;
        }
        if (recv_len < 0) {
            MYMPD_LOG_DEBUG("MPD returned -1 for albumart command for uri \"%s\"", uri);
        }
    }
    if (offset == 0 && mympd_state->mpd_state->feat_mpd_readpicture == true) {
        //silently clear the error if no albumart is found
        mpd_connection_clear_error(mympd_state->mpd_state->conn);
        mpd_response_finish(mympd_state->mpd_state->conn);
        MYMPD_LOG_DEBUG("Try mpd command readpicture for \"%s\"", uri);
        while ((recv_len = mpd_run_readpicture(mympd_state->mpd_state->conn, uri, offset, binary_buffer, mympd_state->mpd_state->mpd_binarylimit)) > 0) {
            MYMPD_LOG_DEBUG("Received %d bytes from mpd readpicture command", recv_len);
            *binary = sdscatlen(*binary, binary_buffer, (size_t)recv_len);
            if (sdslen(*binary) > MPD_BINARY_SIZE_MAX) {
                MYMPD_LOG_WARN("Retrieved binary data is too large, discarding");
                sdsclear(*binary);
                offset = 0;
                break;
            }
            offset += (unsigned)recv_len;
        }
        if (recv_len < 0) {
            MYMPD_LOG_DEBUG("MPD returned -1 for readpicture command for uri \"%s\"", uri);
        }
    }
    if (offset == 0) {
        //silently clear the error if no albumart is found
        mpd_connection_clear_error(mympd_state->mpd_state->conn);
        mpd_response_finish(mympd_state->mpd_state->conn);
    }
    FREE_PTR(binary_buffer);
    if (offset > 0) {
        MYMPD_LOG_DEBUG("Albumart found by mpd for uri \"%s\" (%lu bytes)", uri, (unsigned long)sdslen(*binary));
        const char *mime_type = get_mime_type_by_magic_stream(*binary);
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = tojson_char(buffer, "mime_type", mime_type, false);
        buffer = jsonrpc_result_end(buffer);
        if (mympd_state->covercache_keep_days > 0) {
            covercache_write_file(mympd_state->config->cachedir, uri, mime_type, *binary, 0);
        }
        else {
            MYMPD_LOG_DEBUG("Covercache is disabled");
        }
    }
    else {
        MYMPD_LOG_INFO("No albumart found by mpd for uri \"%s\"", uri);
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "albumart", "warn", "No albumart found by mpd");
    }
    return buffer;
}
