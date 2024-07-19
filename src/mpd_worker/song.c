/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD song functions
 */

#include "compile_time.h"
#include "src/mpd_worker/song.h"

#include "src/lib/jsonrpc.h"
#include "src/mpd_client/errorhandler.h"

/**
 * Gets the chromaprint fingerprint for the song
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param uri song uri
 * @return pointer to buffer
 */
sds mpd_worker_song_fingerprint(struct t_partition_state *partition_state, sds buffer, unsigned request_id, const char *uri) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SONG_FINGERPRINT;
    if (partition_state->mpd_state->feat.fingerprint == false) {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
                JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Fingerprint command not supported");
    }
    char fp_buffer[8192];
    const char *fingerprint = mpd_run_getfingerprint_chromaprint(partition_state->conn, uri, fp_buffer, sizeof(fp_buffer));
    if (fingerprint == NULL) {
        mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_run_getfingerprint_chromaprint");
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = tojson_char(buffer, "fingerprint", fingerprint, false);
    buffer = jsonrpc_end(buffer);

    return buffer;
}
