/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "m3u.h"

#include "jsonrpc.h"
#include "log.h"
#include "sds_extras.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
    while (sds_getline(&line, fp, 1000) == 0) {
        if (sdslen(line) > min_line_len &&
            strncmp(line, field, field_len) == 0)
        {
            const char *ptr = line;
            ptr+= field_len + 1;
            buffer = sdscat(buffer, ptr);
        }
    }
    FREE_SDS(line);
    fclose(fp);
    return buffer;
}

sds m3u_to_json(sds buffer, const char *filename, sds *plname) {
    errno = 0;
    FILE *fp = fopen(filename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Can not open file \"%s\"", filename);
        MYMPD_LOG_ERRNO(errno);
        return buffer;
    }
    sds line = sdsempty();
    //check ext m3u header
    sds_getline(&line, fp, 1000);
    if (strcmp(line, "#EXTM3U") != 0) {
        MYMPD_LOG_WARN("Invalid ext m3u file");
        sdsfree(line);
        fclose(fp);
        return buffer;
    }
    int line_count = 0;
    while (sds_getline(&line, fp, 1000) == 0) {
        if (line[0] == '\0') {
            //skip blank lines
            continue;
        }
        if (line_count++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        if (line[0] != '#') {
            //stream uri
            buffer = tojson_char(buffer, "streamUri", line, false);
            continue;
        }
        buffer = sdscatlen(buffer, "\"", 1);
        int i = 1;
        while (line[i] != '\0' &&
               line[i] != ':')
        {
            buffer = sds_catjsonchar(buffer, line[i]);
            i++;
        }
        buffer = sdscatlen(buffer, "\":\"", 3);
        i++;
        while (line[i] != '\0') {
            buffer = sds_catjsonchar(buffer, line[i]);
            if (plname != NULL) {
                *plname = sdscatprintf(*plname, "%c", line[i]);
            }
            i++;
        }
        buffer = sdscatlen(buffer, "\"", 1);
    }
    FREE_SDS(line);
    fclose(fp);
    return buffer;
}
