/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "../dist/src/sds/sds.h"
#include "../src/sds_extras.h"

struct t_config {
    sds host;
    int port;
    sds pass;
    sds music_directory;
    sds playlist_directory;
    bool stickers;
    bool regex;
};

int sdssplit_whitespace(sds line, sds *name, sds *value) {
    *name = sdsempty();
    *value = sdsempty();
    int tokens = 0;
    int i = 0;
    const char *p = line;
    
    if (*p == '#') {
        return tokens;
    }
    //get name
    for (; i < sdslen(line); i++) {
        if (isspace(*p) != 0) {
            break;
        }
        else {
            *name = sdscatlen(*name, p, 1);
        }
        p++;
    }
    if (sdslen(*name) == 0) {
        return tokens;
    }
    else {
        tokens++;
    }
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

bool parse_mpd_conf(const char *mpd_conf, struct t_config *pconfig) {
    pconfig->host = sdsempty();
    pconfig->port = 6600;
    pconfig->pass = sdsempty();
    pconfig->music_directory = sdsempty();
    pconfig->playlist_directory = sdsempty();
    pconfig->stickers = false;
    pconfig->regex = true;
    
    FILE *fp = fopen(mpd_conf, "r");
    if (fp == NULL) {
        printf("Error parsing %s\n", mpd_conf);
        return false;
    }
    printf("Parsing %s\n", mpd_conf);
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
                if (strcasecmp(name, "music_directory") == 0) {
                    pconfig->music_directory = sdsreplace(pconfig->music_directory, value);
                }
                else if (strcasecmp(name, "playlist_directory") == 0) {
                    pconfig->playlist_directory = sdsreplace(pconfig->playlist_directory, value);
                }
                else if (strcasecmp(name, "sticker_file") == 0) {
                    if (sdslen(value) > 0) {
                        pconfig->stickers = true;
                    }
                }
                else if (strcasecmp(name, "port") == 0) {
                    pconfig->port = strtoimax(value, NULL, 10);
                }
                else if (strcasecmp(name, "bind_to_address") == 0) {
                    if (sdslen(pconfig->host) == 0 || strncmp(value, "/", 1) == 0) {
                        //prefer socket connection
                        pconfig->host = sdsreplace(pconfig->host, value);
                    }
                }
                else if (strcasecmp(name, "password") == 0) {
                    //todo: get account with highest privileges
                    pconfig->pass = sdsreplace(pconfig->pass, value);
                }
            }
            sdsfree(name);
            sdsfree(value);
        }
    }
    fclose(fp);
    sdsfree(sds_line);
    return true;
}

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;
    int rc = EXIT_SUCCESS;
    //todo: get mpd.conf from parameter or check some locations
    sds mpd_conf = sdsnew("/etc/mpd.conf");

    struct t_config mpd_config;
    if (parse_mpd_conf(mpd_conf, &mpd_config) == false) {
        printf("Error parsing %s\n", mpd_conf);
        rc = EXIT_FAILURE;
    }
    
    printf("\tHost: %s\n", mpd_config.host);
    printf("\tPort: %d\n", mpd_config.port);
    printf("\tPassword: %s\n", mpd_config.pass);
    printf("\tMusic directory: %s\n", mpd_config.music_directory);
    printf("\tPlaylist directory: %s\n", mpd_config.playlist_directory);
    printf("\tStickers: %s\n", (mpd_config.stickers == true ? "true" : "false"));
    //todo: check ldd state?
    //printf("\tRegex: %s\n", (mpd_config.regex == true ? "true" : "false"));

    sdsfree(mpd_config.host);
    sdsfree(mpd_config.pass);
    sdsfree(mpd_config.music_directory);
    sdsfree(mpd_config.playlist_directory);
    sdsfree(mpd_conf);
    return rc;
}
