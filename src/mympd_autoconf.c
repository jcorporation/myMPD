/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_autoconf.h"

#include "lib/log.h"
#include "lib/sds_extras.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//private definitions
static int sdssplit_whitespace(sds line, sds *name, sds *value);

//public functions
sds find_mpd_conf(void) {
    const char *filenames[] = { 
        "/etc/mpd.conf",
        "/usr/local/etc/mpd.conf",
        "/etc/opt/mpd/mpd.conf",
        "/etc/opt/mpd.conf",
        NULL
    };

    sds filename = sdsempty();
    for (const char **p = filenames; *p != NULL; p++) {
        filename = sdsreplace(filename, *p);
        FILE *fp = fopen(filename, OPEN_FLAGS_READ);
        if (fp != NULL) { /* Flawfinder: ignore */
            fclose(fp);
            return filename;
        }
    }
    MYMPD_LOG_WARN("No readable mpd.conf found");
    filename = sdscrop(filename);
    return filename;
}

sds get_mpd_conf(const char *key, const char *default_value) {
    sds last_value = sdsnew(default_value);
    sds mpd_conf = find_mpd_conf();
    errno = 0;
    FILE *fp = fopen(mpd_conf, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_WARN("Error opening MPD configuration file \"%s\": ", mpd_conf);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(mpd_conf);
        return last_value;
    }
    sdsfree(mpd_conf);
    char *line = NULL;
    size_t n = 0;
    sds sds_line = sdsempty();
    sds name;
    sds value;
    while (getline(&line, &n, fp) > 0) {
        sds_line = sdsreplace(sds_line, line);
        sdstrim(sds_line, " \n\r\t");
        if (sdslen(sds_line) > 0) {
            int tokens = sdssplit_whitespace(sds_line, &name, &value);
            if (tokens == 2) {
                if (strcasecmp(name, key) == 0 && strcasecmp(name, "bind_to_address") == 0) {
                    if (sdslen(last_value) == 0 || strncmp(value, "/", 1) == 0) {
                        //prefer socket connection
                        MYMPD_LOG_NOTICE("Found mpd host: %s", value);
                        last_value = sdsreplace(last_value, value);
                    }
                }
                else if (strcasecmp(name, key) == 0 && strcasecmp(name, "password") == 0) {
                    sds *pwtokens;
                    int count;
                    pwtokens = sdssplitlen(value, (int)strlen(value), "@", 1, &count);
                    if (count == 2) {
                        if (sdslen(last_value) == 0 || strstr(pwtokens[1], "admin") != NULL) {
                            //use prefered the entry with admin privileges or as fallback the first entry
                            last_value = sdsreplace(last_value, pwtokens[0]);
                            MYMPD_LOG_NOTICE("Found mpd password\n");
                        }
                    }
                    sdsfreesplitres(pwtokens, count);
                }
                else if (strcasecmp(name, key) == 0) {
                    MYMPD_LOG_NOTICE("Found %s: %s", key, value);
                    last_value = sdsreplace(last_value, value);
                }
            }
            sdsfree(name);
            sdsfree(value);
        }
    }
    if (line != NULL) {
        free(line);
    }
    fclose(fp);
    sdsfree(sds_line);
    return last_value;
}

//private functions
static int sdssplit_whitespace(sds line, sds *name, sds *value) {
    *name = sdsempty();
    *value = sdsempty();
    int tokens = 0;
    unsigned i = 0;
    const char *p = line;
    
    if (*p == '#') {
        return tokens;
    }
    //get name
    for (; i < sdslen(line); i++) {
        if (isspace(*p) != 0) {
            break;
        }
        *name = sdscatlen(*name, p, 1);
        p++;
    }
    if (sdslen(*name) == 0) {
        return tokens;
    }
    tokens++;
    //remove whitespace
    for (; i < sdslen(line); i++) {
        if (isspace(*p) == 0) {
            *value = sdscatlen(*value, p, 1);
        }
        p++;
    }
    //get value
    for (; i < sdslen(line); i++) {
        *value = sdscatlen(*value, p, 1);
        p++;
    }
    if (sdslen(*value) > 0) { tokens++; }
    sdstrim(*value, "\"");
    return tokens;
}
