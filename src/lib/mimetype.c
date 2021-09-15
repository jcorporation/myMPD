/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
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

const char *get_mime_type_by_magic(const char *filename) {
    errno = 0;
    FILE *fp = fopen(filename, OPEN_FLAGS_READ_BIN);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Can not open file \"%s\"", filename);
        MYMPD_LOG_ERRNO(errno);
        return sdsempty();
    }
    unsigned char binary_buffer[12];
    size_t read = fread(binary_buffer, 1, sizeof(binary_buffer), fp);
    MYMPD_LOG_DEBUG("Read %u bytes from file %s", read, filename);
    fclose(fp);
    sds stream = sdsnewlen(binary_buffer, read);
    const char *mime_type = get_mime_type_by_magic_stream(stream);
    FREE_SDS(stream);
    return mime_type;
}

const char *get_mime_type_by_magic_stream(sds stream) {
    sds hex_buffer = sdsempty();
    size_t len = sdslen(stream) < 12 ? sdslen(stream) : 12;
    for (size_t i = 0; i < len; i++) {
        hex_buffer = sdscatprintf(hex_buffer, "%02X", (unsigned char) stream[i]);
    }
    const struct t_mime_type_entry *p = NULL;
    for (p = mime_entries; p->magic_bytes != NULL; p++) {
        char *tmp_buffer = hex_buffer;
        if (p->skip > 0 && sdslen(hex_buffer) < p->skip) {
            tmp_buffer += p->skip;
        }
        if (strncmp(tmp_buffer, p->magic_bytes, strlen(p->magic_bytes)) == 0) {
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
