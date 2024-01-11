/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_worker/playlists.h"

#include "src/lib/jsonrpc.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/tags.h"

/**
 * Enumerates the playlist and returns the count and total length
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param plist playlist name to enumerate
 * @return pointer to buffer 
 */
sds mpd_worker_playlist_content_enumerate(struct t_partition_state *partition_state, sds buffer, unsigned request_id, sds plist) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYLIST_CONTENT_ENUMERATE;
    unsigned entity_count = 0;
    unsigned total_time = 0;
    disable_all_mpd_tags(partition_state);
    if (mpd_send_list_playlist_meta(partition_state->conn, plist)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        struct mpd_song *song;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            total_time += mpd_song_get_duration(song);
            entity_count++;
            mpd_song_free(song);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_list_playlist_meta") == false) {
        return buffer;
    }
    buffer = tojson_uint(buffer, "entities", entity_count, true);
    buffer = tojson_uint(buffer, "playtime", total_time, true);
    buffer = tojson_sds(buffer, "plist", plist, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}
