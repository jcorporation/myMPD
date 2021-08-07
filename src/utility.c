/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include "utility.h"

#include "lib/log.h"
#include "lib/sds_extras.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int testdir(const char *name, const char *dirname, bool create) {
    DIR* dir = opendir(dirname);
    if (dir != NULL) {
        closedir(dir);
        MYMPD_LOG_NOTICE("%s: \"%s\"", name, dirname);
        //directory exists
        return 0;
    }

    if (create == true) {
        errno = 0;
        if (mkdir(dirname, 0700) != 0) {
            MYMPD_LOG_ERROR("%s: creating \"%s\" failed", name, dirname);
            MYMPD_LOG_ERRNO(errno);
            //directory does not exist and creating it failed
            return 2;
        }
        MYMPD_LOG_NOTICE("%s: \"%s\" created", name, dirname);
        //directory successfully created
        return 1;
    }

    MYMPD_LOG_ERROR("%s: \"%s\" does not exist", name, dirname);
    //directory does not exist
    return 3;
}

void strip_slash(sds s) {
    int len = sdslen(s);
    if (len > 1 && s[len - 1] == '/') {
        sdsrange(s, 0, len - 2);
    }
}

int strip_extension(char *s) {
    for (ssize_t i = strlen(s) - 1 ; i > 0; i--) {
        if (s[i] == '.') {
            s[i] = '\0';
            return i;
        }
        if (s[i] == '/') {
            return -1;
        }
    }
    return -1;
}

int replacechar(char *str, const char orig, const char rep) {
    char *ix = str;
    int n = 0;
    while ((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}

bool strtobool(const char *value) {
    return strncmp(value, "true", 4) == 0 ? true : false;
}

int uri_to_filename(char *str) {
    int n = replacechar(str, '/', '_');
    n+= replacechar(str, '.', '_');
    n+= replacechar(str, ':', '_');
    return n;
}

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

sds find_image_file(sds basefilename) {
    const struct mime_type_entry *p = NULL;
    for (p = image_files; p->extension != NULL; p++) {
        sds testfilename = sdscatfmt(sdsempty(), "%s.%s", basefilename, p->extension);
        if (access(testfilename, F_OK) == 0) { /* Flawfinder: ignore */
            sdsfree(testfilename);
            break;
        }
        sdsfree(testfilename);
    }
    if (p->extension != NULL) {
        basefilename = sdscatfmt(basefilename, ".%s", p->extension);
    }
    else {
        basefilename = sdscrop(basefilename);
    }
    return basefilename;
}

const struct mime_type_entry media_files[] = {
    {"mp3",  "audio/mpeg"},
    {"flac", "audio/flac"},
    {"oga",  "audio/ogg"},
    {"ogg",  "audio/ogg"},
    {"opus",  "audio/ogg"},
    {"spx",  "audio/ogg"},
    {NULL,   "application/octet-stream"}
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

sds get_mime_type_by_ext(const char *filename) {
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
    sds mime_type = sdsnew(p->mime_type);
    sdsfree(ext);
    return mime_type;
}

sds get_ext_by_mime_type(const char *mime_type) {
    const struct mime_type_entry *p = NULL;
    for (p = image_files; p->extension != NULL; p++) {
        if (strcmp(mime_type, p->mime_type) == 0) {
            break;
        }
    }
    sds ext = sdsnew(p->extension);
    return ext;
}

const struct magic_byte_entry magic_bytes[] = {
    {"89504E470D0A1A0A",  "image/png"},
    {"FFD8FFDB",  "image/jpeg"},
    {"FFD8FFE0",  "image/jpeg"},
    {"FFD8FFEE", "image/jpeg"},
    {"FFD8FFE1", "image/jpeg"},
    {"49492A00", "image/tiff"},
    {"4D4D002A", "image/tiff"},
    {"424D",  "image/x-ms-bmp"},
    {"52494646", "image/webp"},
    {NULL,   "application/octet-stream"}
};

sds get_mime_type_by_magic(const char *filename) {
    errno = 0;
    FILE *fp = fopen(filename, "rb");
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
    sds mime_type = get_mime_type_by_magic_stream(stream);
    sdsfree(stream);
    return mime_type;
}

sds get_mime_type_by_magic_stream(sds stream) {
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
    sdsfree(hex_buffer);
    sds mime_type = sdsnew(p->mime_type);
    return mime_type;
}

void my_usleep(time_t usec) {
    struct timespec ts = {
        .tv_sec = (usec / 1000) / 1000,
        .tv_nsec = (usec % 1000000000L) * 1000
    };
    nanosleep(&ts, NULL);
}

unsigned long substractUnsigned(unsigned long num1, unsigned long num2) {
    if (num1 > num2) {
        return num1 - num2;
    }
    return 0;
}

char *basename_uri(char *uri) {
    //filename
    if (strstr(uri, "://") == NULL) {
        char *b = basename(uri);
        return b;
    }
    //uri, remove query and hash
    char *b = uri;
    for (size_t i = 0;  i < strlen(b); i++) {
        if (b[i] == '#' || b[i] == '?') {
            b[i] = '\0';
            return b;
        }
    }
    return b;
}

//converts unsigned to int and prevents wrap arround
int unsigned_to_int(unsigned x) {
    return x < INT_MAX ? (int) x : INT_MAX;
}
