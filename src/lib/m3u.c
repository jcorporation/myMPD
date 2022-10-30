/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/m3u.h"

#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

#include <errno.h>
#include <string.h>

/**
 * Private definitions
 */
static const char *m3ufields_map(sds field);

/**
 * Public functions
 */

/**
 * Appends the extm3u field value to buffer
 * @param buffer already allocated sds string to append
 * @param field extm3u field to read
 * @param filename m3u file to open
 * @return pointer to buffer
 */
sds m3u_get_field(sds buffer, const char *field, const char *filename) {
    errno = 0;
    FILE *fp = fopen(filename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Can not open file \"%s\"", filename);
        MYMPD_LOG_ERRNO(errno);
        return buffer;
    }
    size_t field_len = strlen(field);
    size_t min_line_len = field_len + 2;
    sds line = sdsempty();
    while (sds_getline(&line, fp, LINE_LENGTH_MAX) == 0) {
        if (sdslen(line) > min_line_len &&
            strncmp(line, field, field_len) == 0)
        {
            const char *ptr = line;
            ptr+= field_len + 1;
            buffer = sdscat(buffer, ptr);
        }
    }
    FREE_SDS(line);
    (void) fclose(fp);
    return buffer;
}

/**
 * Converts the m3u to json and appends it to buffer
 * @param buffer already allocated sds string to append
 * @param filename m3u file to open
 * @param m3ufields appends all fields values (lower case) to this sds string if not NULL
 * @return pointer to buffer
 */
sds m3u_to_json(sds buffer, const char *filename, sds *m3ufields) {
    errno = 0;
    FILE *fp = fopen(filename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Can not open file \"%s\"", filename);
        MYMPD_LOG_ERRNO(errno);
        sdsclear(buffer);
        return buffer;
    }
    sds line = sdsempty();
    //check ext m3u header
    sds_getline(&line, fp, LINE_LENGTH_MAX);
    if (strcmp(line, "#EXTM3U") != 0) {
        MYMPD_LOG_WARN("Invalid ext m3u file");
        FREE_SDS(line);
        (void) fclose(fp);
        sdsclear(buffer);
        return buffer;
    }
    int line_count = 0;
    sds field = sdsempty();
    while (sds_getline(&line, fp, LINE_LENGTH_MAX) == 0) {
        if (line[0] == '\0') {
            //skip blank lines
            continue;
        }
        if (line_count++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        if (line[0] != '#') {
            //stream uri
            buffer = tojson_char(buffer, "StreamUri", line, false);
            continue;
        }
        //skip # char
        int i = 1;
        sdsclear(field);
        while (line[i] != '\0' &&
               line[i] != ':')
        {
            field = sds_catjsonchar(field, line[i]);
            i++;
        }
        const char *key = m3ufields_map(field);
        if (key[0] == '\0') {
            key = field;
        }
        buffer = sdscatfmt(buffer, "\"%s\":\"", key);
        i++;
        while (line[i] != '\0') {
            buffer = sds_catjsonchar(buffer, line[i]);
            if (m3ufields != NULL) {
                *m3ufields = sds_catchar(*m3ufields, line[i]);
            }
            i++;
        }
        buffer = sdscatlen(buffer, "\"", 1);
    }
    FREE_SDS(line);
    FREE_SDS(field);
    (void) fclose(fp);
    if (m3ufields != NULL) {
        sds_utf8_tolower(*m3ufields);
    }
    return buffer;
}

/**
 * Private functions
 */

/**
 * Converts the extm3u field name in a better readable name
 * @param field extm3u field name
 * @return readable name
 */
static const char *m3ufields_map(sds field) {
    if (strcmp(field, "EXTGENRE") == 0)    { return "Genre"; }
    if (strcmp(field, "EXTIMG") == 0)      { return "Image"; }
    if (strcmp(field, "HOMEPAGE") == 0)    { return "Homepage"; }
    if (strcmp(field, "COUNTRY") == 0)     { return "Country"; }
    if (strcmp(field, "LANGUAGE") == 0)    { return "Language"; }
    if (strcmp(field, "DESCRIPTION") == 0) { return "Description"; }
    if (strcmp(field, "PLAYLIST") == 0)    { return "Name"; }
    if (strcmp(field, "CODEC") == 0)       { return "Codec"; }
    if (strcmp(field, "BITRATE") == 0)     { return "Bitrate"; }
    return "";
}
