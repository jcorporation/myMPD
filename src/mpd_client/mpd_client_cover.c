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
#include "../utility.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../plugins.h"
#include "../utility.h"
#include "mpd_client_utility.h"
#include "mpd_client_cover.h"

//private definitions
static int mpd_client_albumart(t_mpd_state *mpd_state, const char *uri, int fd);

//public functions
sds mpd_client_getcover(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                     const char *uri) 
{
    sds tmp_file = sdscatfmt(sdsempty(), "%s/covercache/%s.XXXXXX", config->varlibdir, uri);
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sdsfree(tmp_file);
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Can't write covercache file", true);
        return buffer;
    }
    unsigned size = 0;
    if (mpd_state->feat_mpd_albumart == true) {
        size = mpd_client_albumart(mpd_state, uri, fd);
    }
    if (size == 0 && mpd_state->feat_mpd_readpicture == true) {
      //todo: implement readpicture command
    }

    if (size > 0) {
        sds mime_type = get_mime_type_by_magic(tmp_file);
        sds ext = get_ext_by_mime_type(mime_type);        
        sds cover_file = sdscatfmt(sdsempty(), "%s/covercache/%s.%s", config->varlibdir, uri, ext);
        if (rename(tmp_file, cover_file) == -1) {
            LOG_ERROR("Rename file from %s to %s failed", tmp_file, cover_file);
            buffer = jsonrpc_respond_message(buffer, method, request_id, "Can't write covercache file", true);
            sdsfree(tmp_file);
            sdsfree(cover_file);
            sdsfree(mime_type);
            sdsfree(ext);
            return buffer;
        }
        buffer = jsonrpc_start_result(buffer, method, request_id);
        buffer = sdscat(buffer, ",");
        buffer = tojson_char(buffer, "coverfile", cover_file, true);
        buffer = tojson_char(buffer, "mime_type", mime_type, false);
        buffer = jsonrpc_end_result(buffer);
        sdsfree(mime_type);
        sdsfree(ext);
        sdsfree(cover_file);
    }
    else {
        unlink(tmp_file);
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No albumart found by mpd", true);
    }
    sdsfree(tmp_file);
    return buffer;
}

//private functions
static int mpd_client_albumart(t_mpd_state *mpd_state, const char *uri, int fd) {
    unsigned size = 0;
#ifdef EMBEDDED_LIBMPDCLIENT
    struct mpd_albumart albumart_buffer;
    struct mpd_albumart *albumart;
    unsigned offset = 0;
    FILE *fp = fdopen(fd, "w");
    while ((albumart = mpd_run_albumart(mpd_state->conn, uri, offset, &albumart_buffer)) != NULL) {
        fwrite(albumart->data, 1, albumart->data_length, fp);
	offset += albumart->data_length;
	size = albumart->size;
    }
    fclose(fp);
#else
    (void) uri;
    (void) fd;
#endif
    return size;
}
