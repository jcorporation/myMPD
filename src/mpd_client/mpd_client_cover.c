/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mpd_client_utility.h"
#include "mpd_client_cover.h"

#ifdef EMBEDDED_LIBMPDCLIENT
sds mpd_client_getcover(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                        const char *uri, sds *binary)
{
    unsigned offset = 0;
    if (mpd_state->feat_mpd_albumart == true) {
        struct mpd_albumart albumart_buffer;
        struct mpd_albumart *albumart;
        while ((albumart = mpd_run_albumart(mpd_state->conn, uri, offset, &albumart_buffer)) != NULL) {
            *binary = sdscatlen(*binary, albumart->data, albumart->data_length);
            offset += albumart->data_length;
        }
    }
    if (offset == 0 && mpd_state->feat_mpd_readpicture == true) {
        struct mpd_readpicture readpicture_buffer;
        struct mpd_readpicture *readpicture;
        while ((readpicture = mpd_run_readpicture(mpd_state->conn, uri, offset, &readpicture_buffer)) != NULL) {
            *binary = sdscatlen(*binary, readpicture->data, readpicture->data_length);
            offset += readpicture->data_length;
            mpd_free_readpicture(&readpicture_buffer);
        }
        mpd_free_readpicture(&readpicture_buffer);
    }
    if (offset > 0) {
        sds mime_type = get_mime_type_by_magic_stream(*binary);
        buffer = jsonrpc_start_result(buffer, method, request_id);
        buffer = sdscat(buffer, ",");
        buffer = tojson_char(buffer, "mime_type", mime_type, false);
        buffer = jsonrpc_end_result(buffer);
        if (config->covercache == true) {
            write_covercache_file(config, uri, mime_type, *binary);
        }
        sdsfree(mime_type);
    }
    else {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No albumart found by mpd", true);
    }
    return buffer;
}
#else
sds mpd_client_getcover(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                        const char *uri, sds *binary)
{
    (void) config;
    (void) mpd_state;
    (void) uri;
    (void) binary;
    buffer = jsonrpc_respond_message(buffer, method, request_id, "No albumart found by mpd", true);
    return buffer;
}
#endif
