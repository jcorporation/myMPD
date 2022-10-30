/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/albumart.h"

#include "src/lib/covercache.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mimetype.h"

/**
 * Reads the albumart from mpd
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param request_id request id
 * @param uri uri to get cover from
 * @param binary pointer to an already allocated sds string for the binary response
 * @return jsonrpc response
 */
sds mympd_api_albumart_getcover(struct t_partition_state *partition_state, sds buffer, long request_id,
        const char *uri, sds *binary)
{
    unsigned offset = 0;
    void *binary_buffer = malloc_assert(partition_state->mpd_state->mpd_binarylimit);
    int recv_len = 0;
    if (partition_state->mpd_state->feat_albumart == true) {
        MYMPD_LOG_DEBUG("Try mpd command albumart for \"%s\"", uri);
        while ((recv_len = mpd_run_albumart(partition_state->conn, uri, offset, binary_buffer, partition_state->mpd_state->mpd_binarylimit)) > 0) {
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
    if (offset == 0 &&
        partition_state->mpd_state->feat_readpicture == true)
    {
        //silently clear the error if no albumart is found
        mpd_connection_clear_error(partition_state->conn);
        mpd_response_finish(partition_state->conn);
        MYMPD_LOG_DEBUG("Try mpd command readpicture for \"%s\"", uri);
        while ((recv_len = mpd_run_readpicture(partition_state->conn, uri, offset, binary_buffer, partition_state->mpd_state->mpd_binarylimit)) > 0) {
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
        mpd_connection_clear_error(partition_state->conn);
        mpd_response_finish(partition_state->conn);
    }
    FREE_PTR(binary_buffer);
    if (offset > 0) {
        MYMPD_LOG_DEBUG("Albumart found by mpd for uri \"%s\" (%lu bytes)", uri, (unsigned long)sdslen(*binary));
        const char *mime_type = get_mime_type_by_magic_stream(*binary);
        buffer = jsonrpc_respond_start(buffer, INTERNAL_API_ALBUMART, request_id);
        buffer = tojson_char(buffer, "mime_type", mime_type, false);
        buffer = jsonrpc_end(buffer);
        if (partition_state->mympd_state->config->covercache_keep_days > 0) {
            covercache_write_file(partition_state->mympd_state->config->cachedir, uri, mime_type, *binary, 0);
        }
        else {
            MYMPD_LOG_DEBUG("Covercache is disabled");
        }
    }
    else {
        MYMPD_LOG_INFO("No albumart found by mpd for uri \"%s\"", uri);
        buffer = jsonrpc_respond_message(buffer, INTERNAL_API_ALBUMART, request_id, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_WARN, "No albumart found by mpd");
    }
    return buffer;
}
