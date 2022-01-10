/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_autoconf.h"

#include "../lib/log.h"
#include "../lib/sds_extras.h"
#include "../lib/validate.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

//private definitions
static sds _find_mpd_conf(void);
static sds _get_mpd_conf(const char *key, const char *default_value, validate_callback vcb);
static int _sdssplit_whitespace(sds line, sds *name, sds *value);

//public functions

void mpd_client_autoconf(struct t_mympd_state *mympd_state) {
    //try reading mpd.conf
    sds mpd_conf = _find_mpd_conf();
    if (sdslen(mpd_conf) > 0) {
        MYMPD_LOG_NOTICE("Found %s", mpd_conf);
        //get config from mpd configuration file
        sds mpd_host = _get_mpd_conf("bind_to_address", mympd_state->mpd_state->mpd_host, vcb_isname);
        mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, mpd_host);
        FREE_SDS(mpd_host);

        sds mpd_pass = _get_mpd_conf("password", mympd_state->mpd_state->mpd_pass, vcb_isname);
        mympd_state->mpd_state->mpd_pass = sds_replace(mympd_state->mpd_state->mpd_pass, mpd_pass);
        FREE_SDS(mpd_pass);

        sds mpd_port = _get_mpd_conf("port", mympd_state->mpd_state->mpd_host, vcb_isdigit);
        int port = (int)strtoimax(mpd_port, NULL, 10);
        if (port > 1024 && port <= 65534) {
            mympd_state->mpd_state->mpd_port = port;
        }
        FREE_SDS(mpd_port);

        sds music_directory = _get_mpd_conf("music_directory", mympd_state->music_directory, vcb_isfilepath);
        mympd_state->music_directory = sds_replace(mympd_state->music_directory, music_directory);
        FREE_SDS(music_directory);

        sds playlist_directory = _get_mpd_conf("playlist_directory", mympd_state->playlist_directory, vcb_isfilepath);
        mympd_state->playlist_directory = sds_replace(mympd_state->playlist_directory, playlist_directory);
        FREE_SDS(playlist_directory);
        FREE_SDS(mpd_conf);
        return;
    }
    FREE_SDS(mpd_conf);

    //try environment
    //https://mpd.readthedocs.io/en/latest/client.html#environment-variables
    MYMPD_LOG_NOTICE("Reading environment");
    bool mpd_configured = false;
    const char *mpd_host_env = getenv("MPD_HOST"); /* Flawfinder: ignore */
    if (mpd_host_env != NULL && strlen(mpd_host_env) <= 100) {
        sds mpd_host = sdsnew(mpd_host_env);
        if (vcb_isname(mpd_host) == true) {
            if (mpd_host[0] != '@' && strstr(mpd_host, "@") != NULL) {
                int count = 0;
                sds *tokens = sdssplitlen(mpd_host, (ssize_t)sdslen(mpd_host), "@", 1, &count);
                mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, tokens[1]);
                mympd_state->mpd_state->mpd_pass = sds_replace(mympd_state->mpd_state->mpd_pass, tokens[0]);
                sdsfreesplitres(tokens,count);
            }
            else {
                //no password
                mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, mpd_host);
            }
            MYMPD_LOG_NOTICE("Setting mpd host to \"%s\"", mympd_state->mpd_state->mpd_host);
            mpd_configured = true;
        }
        FREE_SDS(mpd_host);
    }
    const char *mpd_port_env = getenv("MPD_PORT"); /* Flawfinder: ignore */
    if (mpd_port_env != NULL && strlen(mpd_port_env) <= 5) {
        sds mpd_port = sdsnew(mpd_port_env);
        if (vcb_isdigit(mpd_port) == true) {
            int port = (int)strtoimax(mpd_port, NULL, 10);
            if (port == 0) {
                mympd_state->mpd_state->mpd_port = 6600;
                MYMPD_LOG_NOTICE("Setting mpd port to \"%d\"", mympd_state->mpd_state->mpd_port);
            }
            else if (port > MPD_PORT_MIN && port <= MPD_PORT_MAX) {
                mympd_state->mpd_state->mpd_port = port;
                MYMPD_LOG_NOTICE("Setting mpd port to \"%d\"", mympd_state->mpd_state->mpd_port);
            }
            else {
                MYMPD_LOG_WARN("MPD port must be between 1024 and 65534, default is 6600");
            }
        }
        FREE_SDS(mpd_port);
    }
    const char *mpd_timeout_env = getenv("MPD_TIMEOUT"); /* Flawfinder: ignore */
    if (mpd_timeout_env != NULL && strlen(mpd_timeout_env) <= 5) {
        sds mpd_timeout = sdsnew(mpd_timeout_env);
        if (vcb_isdigit(mpd_timeout) == true) {
            int timeout = (int)strtoimax(mpd_timeout, NULL, 10);
            timeout = timeout * 1000; //convert to ms
            if (timeout >= MPD_TIMEOUT_MIN && timeout < MPD_TIMEOUT_MAX) {
                mympd_state->mpd_state->mpd_timeout = timeout;
                MYMPD_LOG_NOTICE("Setting mpd timeout to \"%d\"", mympd_state->mpd_state->mpd_timeout);
            }
            else {
                MYMPD_LOG_WARN("MPD timeout must be between %d and %d", MPD_TIMEOUT_MIN, MPD_TIMEOUT_MAX);
            }
        }
        FREE_SDS(mpd_timeout);
    }
    if (mpd_configured == true) {
        return;
    }

    //check for socket
    const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR"); /* Flawfinder: ignore */
    if (xdg_runtime_dir != NULL && strlen(xdg_runtime_dir) <= 100) {
        sds socket = sdscatfmt(sdsempty(), "%s/mpd/socket", xdg_runtime_dir);
        if (access(socket, F_OK ) == 0) { /* Flawfinder: ignore */
            mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, socket);
            sdsfree(socket);
            return;
        }
        sdsfree(socket);
    }
    if (access("/run/mpd/socket", F_OK ) == 0) { /* Flawfinder: ignore */
        mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, "/run/mpd/socket");
        return;
    }
    if (access("/var/run/mpd/socket", F_OK ) == 0) { /* Flawfinder: ignore */
        mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, "/var/run/mpd/socket");
        return;
    }
    if (access("/var/lib/mpd/socket", F_OK ) == 0) { /* Flawfinder: ignore */
        //gentoo default 
        mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, "/var/lib/mpd/socket");
        return;
    }
    //fallback to localhost:6600
    mympd_state->mpd_state->mpd_host = sds_replace(mympd_state->mpd_state->mpd_host, "localhost");
    mympd_state->mpd_state->mpd_port = 6600;
}

//private functions
static sds _find_mpd_conf(void) {
    const char *filenames[] = {
        "/etc/mpd.conf",
        "/usr/local/etc/mpd.conf",
        "/etc/opt/mpd/mpd.conf",
        "/etc/opt/mpd.conf",
        NULL
    };

    sds filename = sdsempty();
    for (const char **p = filenames; *p != NULL; p++) {
        filename = sds_replace(filename, *p);
        FILE *fp = fopen(filename, OPEN_FLAGS_READ);
        if (fp != NULL) { /* Flawfinder: ignore */
            fclose(fp);
            return filename;
        }
    }
    MYMPD_LOG_WARN("No readable mpd.conf found");
    sdsclear(filename);
    return filename;
}

static sds _get_mpd_conf(const char *key, const char *default_value, validate_callback vcb) {
    sds last_value = sdsnew(default_value);
    sds mpd_conf = _find_mpd_conf();
    errno = 0;
    FILE *fp = fopen(mpd_conf, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_WARN("Error opening MPD configuration file \"%s\": ", mpd_conf);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(mpd_conf);
        return last_value;
    }
    FREE_SDS(mpd_conf);
    sds line = sdsempty();
    sds name;
    sds value;
    while (sds_getline(&line, fp, 1000) == 0) {
        if (sdslen(line) > 0) {
            int tokens = _sdssplit_whitespace(line, &name, &value);
            if (tokens == 2) {
                if (strcasecmp(name, key) == 0 && strcasecmp(name, "bind_to_address") == 0) {
                    if (sdslen(last_value) == 0 || strncmp(value, "/", 1) == 0) {
                        //prefer socket connection
                        MYMPD_LOG_NOTICE("Found mpd host: %s", value);
                        if (vcb(value) == true) {
                            last_value = sds_replace(last_value, value);
                        }
                    }
                }
                else if (strcasecmp(name, key) == 0 && strcasecmp(name, "password") == 0) {
                    sds *pwtokens;
                    int count = 0;
                    pwtokens = sdssplitlen(value, (int)sdslen(value), "@", 1, &count);
                    if (count == 2) {
                        if (sdslen(last_value) == 0 || strstr(pwtokens[1], "admin") != NULL) {
                            //prefer the entry with admin privileges or as fallback the first entry
                            MYMPD_LOG_NOTICE("Found mpd password\n");
                            if (vcb(pwtokens[0]) == true) {
                                last_value = sds_replace(last_value, pwtokens[0]);
                            }
                        }
                    }
                    sdsfreesplitres(pwtokens, count);
                }
                else if (strcasecmp(name, key) == 0) {
                    MYMPD_LOG_NOTICE("Found %s: %s", key, value);
                    if (vcb(value) == true) {
                        last_value = sds_replace(last_value, value);
                    }
                }
            }
            FREE_SDS(name);
            FREE_SDS(value);
        }
    }
    fclose(fp);
    FREE_SDS(line);
    return last_value;
}

static int _sdssplit_whitespace(sds line, sds *name, sds *value) {
    *name = sdsempty();
    *value = sdsempty();
    int tokens = 0;
    size_t i = 0;
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
