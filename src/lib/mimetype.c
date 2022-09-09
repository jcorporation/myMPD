/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "mimetype.h"

#include "log.h"
#include "sds_extras.h"
#include "utility.h"

#include <string.h>

/**
 * List of mime types with magic numbers and typical extensions
 */
struct t_mime_type_entry {
    size_t skip;              //!< starting bytes to skip
    const char *magic_bytes;  //!< bytes to match to get the mime type
    const char *extension;    //!< file extension for the mime type
    const char *mime_type;    //!< mime type
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

/**
 * Gets the mime type by extension
 * @param filename 
 * @return the mime type
 */
const char *get_mime_type_by_ext(const char *filename) {
    const char *ext = get_extension_from_filename(filename);
    if (ext == NULL) {
        return "application/octet-stream";
    }
    const struct t_mime_type_entry *p = NULL;
    for (p = mime_entries; p->extension != NULL; p++) {
        if (strcasecmp(ext, p->extension) == 0) {
            break;
        }
    }
    return p->mime_type;
}

/**
 * Gets the extension by mime type
 * @param mime_type 
 * @return the extension
 */
const char *get_ext_by_mime_type(const char *mime_type) {
    const struct t_mime_type_entry *p = NULL;
    for (p = mime_entries; p->extension != NULL; p++) {
        if (strcasecmp(mime_type, p->mime_type) == 0) {
            break;
        }
    }
    if (p->extension == NULL) {
        MYMPD_LOG_WARN("No extension found for mime_type \"%s\"", mime_type);
    }
    return p->extension;
}

/**
 * Gets the mime type by magic numbers in binary buffer
 * @param stream binary buffer
 * @return the mime type or generic application/octet-stream
 */
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

/**
 * List of image type extionsions
 */
const char *image_extensions[] = {
    "webp",
    "jpg",
    "jpeg",
    "png",
    "avif",
    "svg",
    NULL
};

/**
 * Checks the filetype by extension
 * @param filename 
 * @return true if it is a image extension else false
 */
bool is_image(const char *filename) {
    const char *ext = get_extension_from_filename(filename);
    if (ext == NULL) {
        return false;
    }
    const char **p = image_extensions;
    while(*p != NULL) {
        if (strcasecmp(*p, ext) == 0) {
            return true;
        }
        p++;
    }
    return false;
}
