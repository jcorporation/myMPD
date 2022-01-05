/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mimetype.h"

#include "log.h"
#include "sds_extras.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct t_mime_type_entry {
    unsigned skip;
    const char *magic_bytes;
    const char *extension;
    const char *mime_type;
};

const struct t_mime_type_entry mime_entries[] = {
    {0, "89504E470D0A1A0A", "png",  "image/png"},
    {0, "FFD8FFDB",         "jpg",  "image/jpeg"},
    {0, "FFD8FFE0",         "jpeg", "image/jpeg"},
    {0, "FFD8FFEE",         "jpeg", "image/jpeg"},
    {0, "FFD8FFE1",         "jpeg", "image/jpeg"},
    {0, "52494646",         "webp", "image/webp"},
    {4, "667479706d696631", "avif", "image/avif"},
    {0, "494433",           "mp3",  "audio/mpeg"},
    {0, "664C6143",         "flac", "audio/flac"},
    {0, "4F676753",         "oga",  "audio/ogg"},
    {0, "4F676753",         "ogg",  "audio/ogg"},
    {0, "4F676753",         "opus", "audio/ogg"},
    {0, "4F676753",         "spx",  "audio/ogg"},
    {0, NULL,               NULL,   "application/octet-stream"}
};

const char *get_mime_type_by_ext(const char *filename) {
    sds ext = sds_get_extension_from_filename(filename);

    const struct t_mime_type_entry *p = NULL;
    for (p = mime_entries; p->extension != NULL; p++) {
        if (strcmp(ext, p->extension) == 0) {
            break;
        }
    }
    FREE_SDS(ext);
    return p->mime_type;
}

const char *get_ext_by_mime_type(const char *mime_type) {
    const struct t_mime_type_entry *p = NULL;
    for (p = mime_entries; p->extension != NULL; p++) {
        if (strcmp(mime_type, p->mime_type) == 0) {
            break;
        }
    }
    if (p->extension == NULL) {
        MYMPD_LOG_WARN("No extension found for mime_type \"%s\"", mime_type);
    }
    return p->extension;
}

const char *get_mime_type_by_magic_stream(sds stream) {
    sds hex_buffer = sdsempty();
    size_t stream_len = sdslen(stream) < 12 ? sdslen(stream) : 12;
    for (size_t i = 0; i < stream_len; i++) {
        hex_buffer = sdscatprintf(hex_buffer, "%02X", (unsigned char) stream[i]);
    }
    const struct t_mime_type_entry *p = NULL;
    for (p = mime_entries; p->magic_bytes != NULL; p++) {
        char *tmp_buffer = hex_buffer;
        if (p->skip > 0 && sdslen(hex_buffer) > p->skip) {
            tmp_buffer += p->skip;
        }
        size_t magic_bytes_len = strlen(p->magic_bytes);
        if (strlen(tmp_buffer) >= magic_bytes_len &&
            strncmp(tmp_buffer, p->magic_bytes, magic_bytes_len) == 0)
        {
            MYMPD_LOG_DEBUG("Matched magic bytes for mime_type: %s", p->mime_type);
            break;
        }
    }
    if (p->magic_bytes == NULL) {
        MYMPD_LOG_WARN("Could not determine mime type from bytes \"%s\"", hex_buffer);
    }
    FREE_SDS(hex_buffer);
    return p->mime_type;
}
