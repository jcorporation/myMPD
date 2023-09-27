/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/albumart.h"

#include "src/lib/album_cache.h"
#include "src/lib/api.h"
#include "src/lib/covercache.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/search.h"

#include <string.h>

/**
 * Reads the albumart by album id
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param request_id request id
 * @param albumid the album id
 * @param size size of the albumart
 * @return jsonrpc response
 */
sds mympd_api_albumart_getcover_by_album_id(struct t_partition_state *partition_state, sds buffer, long request_id,
        sds albumid, unsigned size)
{
    struct mpd_song *album = album_cache_get_album(&partition_state->mpd_state->album_cache, albumid);
    if (album == NULL) {
        return jsonrpc_respond_message(buffer, INTERNAL_API_ALBUMART_BY_ALBUMID, request_id, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_WARN, "No albumart found by mpd");
    }

    // check album cache for uri
    if (strcmp(mpd_song_get_uri(album), "albumid") != 0) {
        // uri is cached - send redirect to albumart by uri
        buffer = jsonrpc_respond_start(buffer, INTERNAL_API_ALBUMART_BY_ALBUMID, request_id);
        buffer = tojson_char(buffer, "uri", mpd_song_get_uri(album), true);
        buffer = tojson_uint(buffer, "size", size, false);
        buffer = jsonrpc_end(buffer);
        return buffer;
    }

    // search for one song in the album
    sds expression = get_search_expression_album(partition_state->mpd_state->tag_albumartist, album);

    if (mpd_search_db_songs(partition_state->conn, false) == false ||
        mpd_search_add_expression(partition_state->conn, expression) == false ||
        mpd_search_add_window(partition_state->conn, 0, 1) == false)
    {
        mpd_search_cancel(partition_state->conn);
        FREE_SDS(expression);
        return jsonrpc_respond_message(buffer, INTERNAL_API_ALBUMART_BY_ALBUMID, request_id, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_WARN, "No albumart found by mpd");
    }

    struct mpd_song *song = NULL;
    if (mpd_search_commit(partition_state->conn) == true &&
        (song = mpd_recv_song(partition_state->conn)) != NULL &&
        mpd_response_finish(partition_state->conn) == true)
    {
        // found a song - send redirect to albumart by uri
        buffer = jsonrpc_respond_start(buffer, INTERNAL_API_ALBUMART_BY_ALBUMID, request_id);
        buffer = tojson_char(buffer, "uri", mpd_song_get_uri(song), true);
        buffer = tojson_uint(buffer, "size", size, false);
        buffer = jsonrpc_end(buffer);
        // update album cache with uri
        album_cache_set_uri(album, mpd_song_get_uri(song));
        mpd_song_free(song);
        FREE_SDS(expression);
        return buffer;
    }

    // no song found
    if (song != NULL) {
        mpd_song_free(song);
    }
    FREE_SDS(expression);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, INTERNAL_API_ALBUMART_BY_ALBUMID, request_id, "mpd_search_db_songs") == false) {
        return buffer;
    }
    return jsonrpc_respond_message(buffer, INTERNAL_API_ALBUMART_BY_ALBUMID, request_id, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_WARN, "No albumart found by mpd");
}

/**
 * Reads the albumart from mpd by song uri
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param request_id request id
 * @param uri uri to get cover from
 * @param binary pointer to an already allocated sds string for the binary response
 * @return jsonrpc response
 */
sds mympd_api_albumart_getcover_by_uri(struct t_partition_state *partition_state, sds buffer, long request_id,
        const char *uri, sds *binary)
{
    unsigned offset = 0;
    void *binary_buffer = malloc_assert(partition_state->mpd_state->mpd_binarylimit);
    int recv_len = 0;
    if (partition_state->mpd_state->feat_albumart == true) {
        MYMPD_LOG_DEBUG(partition_state->name, "Try mpd command albumart for \"%s\"", uri);
        while ((recv_len = mpd_run_albumart(partition_state->conn, uri, offset, binary_buffer, partition_state->mpd_state->mpd_binarylimit)) > 0) {
            MYMPD_LOG_DEBUG(partition_state->name, "Received %d bytes from mpd albumart command", recv_len);
            *binary = sdscatlen(*binary, binary_buffer, (size_t)recv_len);
            if (sdslen(*binary) > MPD_BINARY_SIZE_MAX) {
                MYMPD_LOG_WARN(partition_state->name, "Retrieved binary data is too large, discarding");
                sdsclear(*binary);
                offset = 0;
                break;
            }
            offset += (unsigned)recv_len;
        }
        if (recv_len < 0) {
            MYMPD_LOG_DEBUG(partition_state->name, "MPD returned -1 for albumart command for uri \"%s\"", uri);
        }
    }
    if (offset == 0 &&
        partition_state->mpd_state->feat_readpicture == true)
    {
        //silently clear the error if no albumart is found
        mpd_connection_clear_error(partition_state->conn);
        mpd_response_finish(partition_state->conn);
        MYMPD_LOG_DEBUG(partition_state->name, "Try mpd command readpicture for \"%s\"", uri);
        while ((recv_len = mpd_run_readpicture(partition_state->conn, uri, offset, binary_buffer, partition_state->mpd_state->mpd_binarylimit)) > 0) {
            MYMPD_LOG_DEBUG(partition_state->name, "Received %d bytes from mpd readpicture command", recv_len);
            *binary = sdscatlen(*binary, binary_buffer, (size_t)recv_len);
            if (sdslen(*binary) > MPD_BINARY_SIZE_MAX) {
                MYMPD_LOG_WARN(partition_state->name, "Retrieved binary data is too large, discarding");
                sdsclear(*binary);
                offset = 0;
                break;
            }
            offset += (unsigned)recv_len;
        }
        if (recv_len < 0) {
            MYMPD_LOG_DEBUG(partition_state->name, "MPD returned -1 for readpicture command for uri \"%s\"", uri);
        }
    }
    if (offset == 0) {
        //silently clear the error if no albumart is found
        mpd_connection_clear_error(partition_state->conn);
        mpd_response_finish(partition_state->conn);
    }
    FREE_PTR(binary_buffer);
    if (offset > 0) {
        MYMPD_LOG_DEBUG(partition_state->name, "Albumart found by mpd for uri \"%s\" (%lu bytes)", uri, (unsigned long)sdslen(*binary));
        const char *mime_type = get_mime_type_by_magic_stream(*binary);
        buffer = jsonrpc_respond_start(buffer, INTERNAL_API_ALBUMART_BY_URI, request_id);
        buffer = tojson_char(buffer, "mime_type", mime_type, false);
        buffer = jsonrpc_end(buffer);
        if (partition_state->mympd_state->config->covercache_keep_days > 0) {
            covercache_write_file(partition_state->mympd_state->config->cachedir, uri, mime_type, *binary, 0);
        }
        else {
            MYMPD_LOG_DEBUG(partition_state->name, "Covercache is disabled");
        }
    }
    else {
        MYMPD_LOG_INFO(partition_state->name, "No albumart found by mpd for uri \"%s\"", uri);
        buffer = jsonrpc_respond_message(buffer, INTERNAL_API_ALBUMART_BY_URI, request_id, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_WARN, "No albumart found by mpd");
    }
    return buffer;
}
