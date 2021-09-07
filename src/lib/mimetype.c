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

struct mime_type_entry {
    const char *extension;
    const char *mime_type;
};

struct magic_byte_entry {
    const char *magic_bytes;
    const char *mime_type;
};

const struct mime_type_entry image_files[] = {
    {"png",  "image/png"},
    {"jpg",  "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"svg",  "image/svg+xml"},
    {"webp", "image/webp"},
    {"tiff", "image/tiff"},
    {"bmp",  "image/x-ms-bmp"},
    {NULL,   "application/octet-stream"}
};

const struct mime_type_entry media_files[] = {
    {"mp3",  "audio/mpeg"},
    {"flac", "audio/flac"},
    {"oga",  "audio/ogg"}, 
    {"ogg",  "audio/ogg"},
    {"opus", "audio/ogg"},
    {"spx",  "audio/ogg"},
    {NULL,   "application/octet-stream"}
};

const struct magic_byte_entry magic_bytes[] = {
    {"89504E470D0A1A0A",  "image/png"},
    {"FFD8FFDB",          "image/jpeg"},
    {"FFD8FFE0",          "image/jpeg"},
    {"FFD8FFEE",          "image/jpeg"},
    {"FFD8FFE1",          "image/jpeg"},
    {"49492A00",          "image/tiff"},
    {"4D4D002A",          "image/tiff"},
    {"424D",              "image/x-ms-bmp"},
    {"52494646",          "image/webp"},
    {NULL,                "application/octet-stream"}
};

sds get_extension_from_filename(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        return sdsempty();
    }
    if (strlen(ext) > 1) {
        //trim starting dot
        ext++;
    }
    else {
        return sdsempty();
    }
    sds extension = sdsnew(ext);
    sdstolower(extension);
    return extension;
}

sds find_image_file(sds basefilename) {
    const struct mime_type_entry *p = NULL;
    for (p = image_files; p->extension != NULL; p++) {
        sds testfilename = sdscatfmt(sdsempty(), "%s.%s", basefilename, p->extension);
        if (access(testfilename, F_OK) == 0) { /* Flawfinder: ignore */
            FREE_SDS(testfilename);
            break;
        }
        FREE_SDS(testfilename);
    }
    if (p->extension != NULL) {
        basefilename = sdscatfmt(basefilename, ".%s", p->extension);
    }
    else {
        sdsclear(basefilename);
    }
    return basefilename;
}

const char *get_mime_type_by_ext(const char *filename) {
    sds ext = get_extension_from_filename(filename);

    const struct mime_type_entry *p = NULL;
    for (p = image_files; p->extension != NULL; p++) {
        if (strcmp(ext, p->extension) == 0) {
            break;
        }
    }
    if (p->extension == NULL) {
        p = NULL;
        for (p = media_files; p->extension != NULL; p++) {
            if (strcmp(ext, p->extension) == 0) {
                break;
            }
        }
    }
    FREE_SDS(ext);
    return p->mime_type;
}

const char *get_ext_by_mime_type(const char *mime_type) {
    const struct mime_type_entry *p = NULL;
    for (p = image_files; p->extension != NULL; p++) {
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
    unsigned char binary_buffer[8];
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
    size_t len = sdslen(stream) < 8 ? sdslen(stream) : 8;
    for (size_t i = 0; i < len; i++) {
        hex_buffer = sdscatprintf(hex_buffer, "%02X", (unsigned char) stream[i]);
    }
    const struct magic_byte_entry *p = NULL;
    for (p = magic_bytes; p->magic_bytes != NULL; p++) {
        if (strncmp(hex_buffer, p->magic_bytes, strlen(p->magic_bytes)) == 0) {
            MYMPD_LOG_DEBUG("Matched magic bytes for mime_type: %s", p->mime_type);
            break;
        }
    }
    if (p->magic_bytes == NULL) {
        MYMPD_LOG_WARN("Could not get mime type from bytes \"%s\"", hex_buffer);
    }
    FREE_SDS(hex_buffer);
    return p->mime_type;
}
