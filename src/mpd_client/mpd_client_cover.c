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
#include "../utility.h"
#include "mpd_client_utility.h"
#include "mpd_client_cover.h"

#ifdef EMBEDDED_LIBMPDCLIENT
sds mpd_client_getcover(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                        const char *uri, sds *binary)
{
    unsigned offset = 0;
    FILE *fp = NULL;
    sds filename = sdsnew(uri);
    uri_to_filename(filename);
    sds tmp_file = sdscatfmt(sdsempty(), "%s/covercache/%s.XXXXXX", config->varlibdir, filename);
    if (config->covercache == true) {
        int fd = mkstemp(tmp_file);
        if (fd < 0 ) {
            LOG_ERROR("Can't open %s for write", tmp_file);
            sdsfree(filename);
            sdsfree(tmp_file);
            buffer = jsonrpc_respond_message(buffer, method, request_id, "Can't write covercache file", true);
            return buffer;
        }
        fp = fdopen(fd, "w");
    }
    if (mpd_state->feat_mpd_albumart == true) {
        struct mpd_albumart albumart_buffer;
        struct mpd_albumart *albumart;
        while ((albumart = mpd_run_albumart(mpd_state->conn, uri, offset, &albumart_buffer)) != NULL) {
            if (config->covercache == true) {
                fwrite(albumart->data, 1, albumart->data_length, fp);
            }
            if (albumart->size < config->covercache_avoid || config->covercache == false) {
                // smaller than covercacheavoid (default 2 MB), or disabled covercache
                *binary = sdscatlen(*binary, albumart->data, albumart->data_length);
            }
            offset += albumart->data_length;
        }
    }
    if (offset == 0 && mpd_state->feat_mpd_readpicture == true) {
        struct mpd_readpicture readpicture_buffer;
        struct mpd_readpicture *readpicture;
        while ((readpicture = mpd_run_readpicture(mpd_state->conn, uri, offset, &readpicture_buffer)) != NULL) {
            if (config->covercache == true) {
                fwrite(readpicture->data, 1, readpicture->data_length, fp);
            }
            if (readpicture->size < config->covercache_avoid || config->covercache == false) {
                // smaller than covercacheavoid (default 2 MB), or disabled covercache
                *binary = sdscatlen(*binary, readpicture->data, readpicture->data_length);
            }
            offset += readpicture->data_length;
            mpd_free_readpicture(&readpicture_buffer);
        }
    }
    if (config->covercache == true) {
        fclose(fp);
    }
    if (offset > 0) {
        sds mime_type;
        if (sdslen(*binary) > 0) {
            mime_type = get_mime_type_by_magic_stream(*binary);
        }
        else {
            mime_type = get_mime_type_by_magic(tmp_file);
        }
        sds ext = get_ext_by_mime_type(mime_type);
        sds cover_file = sdscatfmt(sdsempty(), "%s/covercache/%s.%s", config->varlibdir, filename, ext);
        if (config->covercache == true) {
            if (rename(tmp_file, cover_file) == -1) {
                LOG_ERROR("Rename file from %s to %s failed", tmp_file, cover_file);
                sdsfree(mime_type);
                sdsfree(ext);
                sdsfree(cover_file);
                sdsfree(filename);
                buffer = jsonrpc_respond_message(buffer, method, request_id, "Can't write covercache file", true);
                return buffer;
            }
        }

        buffer = jsonrpc_start_result(buffer, method, request_id);
        buffer = sdscat(buffer, ",");
        if (sdslen(*binary) > 0) {
            buffer = tojson_char(buffer, "coverfile", "binary", true);
        }
        else {
            buffer = tojson_char(buffer, "coverfile", "file", true);
        }
        buffer = tojson_char(buffer, "coverfile_name", cover_file, true);
        buffer = tojson_char(buffer, "mime_type", mime_type, false);
        buffer = jsonrpc_end_result(buffer);
        sdsfree(mime_type);
        sdsfree(ext);
        sdsfree(cover_file);
    }
    else {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No albumart found by mpd", true);
        if (config->readonly == false) {
            unlink(tmp_file);
        }
    }
    sdsfree(tmp_file);
    sdsfree(filename);
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
