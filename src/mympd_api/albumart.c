/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD Albumart API
 */

#include "compile_time.h"
#include "src/mympd_api/albumart.h"

#include "src/lib/album.h"
#include "src/lib/api.h"
#include "src/lib/cache/cache_disk.h"
#include "src/lib/cache/cache_disk_images.h"
#include "src/lib/cache/cache_rax_album.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"
#include "src/mympd_api/trigger.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/search.h"

#include <string.h>

/**
 * Reads the albumart by album id
 * @param partition_state pointer to partition specific states
 * @param album_cache pointer to album cache
 * @param buffer already allocated sds string for the jsonrpc response
 * @param request_id request id
 * @param albumid the album id
 * @param size size of the albumart
 * @return jsonrpc response
 */
sds mympd_api_albumart_getcover_by_album_id(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds buffer, unsigned request_id, sds albumid, unsigned size)
{
    if (album_cache->cache == NULL) {
        buffer = jsonrpc_respond_message(buffer, INTERNAL_API_ALBUMART_BY_ALBUMID, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_WARN, "Albumcache not ready");
        return buffer;
    }

    struct mpd_song *album = album_cache_get_album(album_cache, albumid);
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
    sds expression = get_search_expression_album(sdsempty(), partition_state->mpd_state->tag_albumartist,
        album, &partition_state->config->albums);

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
        (song = mpd_recv_song(partition_state->conn)) != NULL)
    {
        // found a song - send redirect to albumart by uri
        buffer = jsonrpc_respond_start(buffer, INTERNAL_API_ALBUMART_BY_ALBUMID, request_id);
        buffer = tojson_char(buffer, "uri", mpd_song_get_uri(song), true);
        buffer = tojson_uint(buffer, "size", size, false);
        buffer = jsonrpc_end(buffer);
        // update album cache with uri
        album_set_uri(album, mpd_song_get_uri(song));
        mpd_song_free(song);
        mympd_check_error_and_recover(partition_state, NULL, "mpd_search_db_songs");
        FREE_SDS(expression);
        return buffer;
    }

    // no song found
    FREE_SDS(expression);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, INTERNAL_API_ALBUMART_BY_ALBUMID, request_id, "mpd_search_db_songs") == false) {
        return buffer;
    }
    return jsonrpc_respond_message(buffer, INTERNAL_API_ALBUMART_BY_ALBUMID, request_id, JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_WARN, "No albumart found by mpd");
}

/**
 * Reads the albumart from mpd by song uri
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string for the jsonrpc response
 * @param request_id request id
 * @param conn_id mongoose connection id
 * @param uri uri to get cover for
 * @param binary pointer to an already allocated sds string for the binary response
 * @return jsonrpc response
 */
sds mympd_api_albumart_getcover_by_uri(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
    sds buffer, unsigned request_id, unsigned long conn_id, sds uri, void **binary)
{
    unsigned offset = 0;
    void *binary_buffer = malloc_assert(partition_state->mpd_state->mpd_binarylimit);
    int recv_len = 0;

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

    if (offset == 0) {
        //silently clear the error if no albumart is found
        mympd_clear_finish(partition_state);
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
        mympd_clear_finish(partition_state);
    }
    FREE_PTR(binary_buffer);

    if (offset > 0) {
        MYMPD_LOG_DEBUG(partition_state->name, "Albumart found by mpd for uri \"%s\" (%lu bytes)", uri, (unsigned long)sdslen(*binary));
        const char *mime_type = get_mime_type_by_magic_stream(*binary);
        buffer = jsonrpc_respond_start(buffer, INTERNAL_API_ALBUMART_BY_URI, request_id);
        buffer = tojson_char(buffer, "mime_type", mime_type, false);
        buffer = jsonrpc_end(buffer);
        if (partition_state->config->cache_cover_keep_days != CACHE_DISK_DISABLED) {
            sds filename = cache_disk_images_write_file(partition_state->config->cachedir, DIR_CACHE_COVER, uri, mime_type, *binary, 0);
            FREE_SDS(filename);
        }
        else {
            MYMPD_LOG_DEBUG(partition_state->name, "Covercache is disabled");
        }
    }
    else {
        #ifdef MYMPD_ENABLE_LUA
            // no albumart found, check if there is a trigger to fetch albumart
            struct t_list arguments;
            list_init(&arguments);
            list_push(&arguments, "uri", 0, uri, NULL);
            int n = mympd_api_trigger_execute_http(&mympd_state->trigger_list, TRIGGER_MYMPD_ALBUMART,
                    partition_state->name, conn_id, request_id, &arguments);
            list_clear(&arguments);
            if (n > 0) {
                // return empty buffer, response must be send by triggered script
                if (n > 1) {
                    MYMPD_LOG_WARN(partition_state->name, "More than one script triggered for albumart.");
                }
                return buffer;
            }
        #else
            (void)mympd_state;
            (void)conn_id;
        #endif
        MYMPD_LOG_INFO(partition_state->name, "No albumart found by mpd for uri \"%s\"", uri);
        buffer = jsonrpc_respond_message(buffer, INTERNAL_API_ALBUMART_BY_URI, request_id,
                JSONRPC_FACILITY_MPD, JSONRPC_SEVERITY_WARN, "No albumart found by mpd");
    }
    return buffer;
}
