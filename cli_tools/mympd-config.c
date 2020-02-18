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
#include <getopt.h>

#include "../dist/src/sds/sds.h"
#include "../src/sds_extras.h"

struct t_config {
    //parsed values
    sds host;
    int port;
    sds pass;
    sds music_directory;
    sds playlist_directory;
    bool stickers;
    bool regex;
    //options
    sds mpd_conf;
    sds mpd_exe;
    sds mympd_conf;
    sds webport;
    sds sslport;
    sds user;
    int loglevel;
    bool ssl;
    bool mixramp;
    bool publish;
    bool webdav;
    bool syscmds;
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

bool parse_mpd_conf(struct t_config *pconfig) {
    FILE *fp = fopen(pconfig->mpd_conf, "r");
    if (fp == NULL) {
        printf("Error parsing %s\n", pconfig->mpd_conf);
        return false;
    }
    printf("Parsing %s\n", pconfig->mpd_conf);
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
                    sds *tokens;
                    int count;
                    tokens = sdssplitlen(value, strlen(value), "@", 1, &count);
                    if (count == 2) {
                        if (sdslen(pconfig->pass) == 0 || strstr(tokens[1], "admin") != NULL) {
                            //use prefered the entry with admin privileges or as fallback the first entry
                            pconfig->pass = sdsreplace(pconfig->pass, tokens[0]);
                        }
                    }
                    sdsfreesplitres(tokens, count);
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
    return true;
}

bool check_ldd(struct t_config *pconfig) {
    printf("Getting mpd linked libraries %s\n", pconfig->mpd_exe);
    sds ldd = sdsnew("/usr/bin/ldd ");
    ldd = sdscatprintf(ldd, "%s", pconfig->mpd_exe);
    FILE *fp = popen(ldd, "r"); /* Flawfinder: ignore */
    sdsfree(ldd);
    if (fp == NULL) {
        return false;
    }
    char *line = NULL;
    size_t n = 0;
    while (getline(&line, &n, fp) > 0) {
        if (strstr(line, "libpcre") != NULL) {
            pconfig->regex = true;
            break;
        }
    }
    if (line != NULL) {
        free(line);
    }
    pclose(fp);
    return true;
}

bool parse_options(struct t_config *pconfig, int argc, char **argv) {
    int n, option_index = 0;
    
    static struct option long_options[] = {
        {"mpdconf",   required_argument, 0, 'c'},
        {"mpdexe",    required_argument, 0, 'e'},
        {"mympdconf", required_argument, 0, 'm'},
        {"webport",   required_argument, 0, 'w'},
        {"sslport",   required_argument, 0, 's'},
        {"loglevel",  required_argument, 0, 'l'},
        {"user",      required_argument, 0, 'u'},
        {"mixramp",   no_argument,       0, 'r'},
        {"publish",   no_argument,       0, 'p'},
        {"webdav",    no_argument,       0, 'd'},
        {"syscmds",   no_argument,       0, 'y'},
        {"help",      no_argument,       0, 'h'},
        {0,           0,                 0,  0 }
    };

    while((n = getopt_long(argc, argv, "c:m:w:s:l:u:e:rpdhy", long_options, &option_index)) != -1) { /* Flawfinder: ignore */
        switch (n) {
            case 'c':
                pconfig->mpd_conf = sdsreplace(pconfig->mpd_conf, optarg);
                break;
            case 'e':
                pconfig->mpd_exe = sdsreplace(pconfig->mpd_exe, optarg);
                break;
            case 'm':
                pconfig->mympd_conf = sdsreplace(pconfig->mympd_conf, optarg);
                break;
            case 'w':
                pconfig->webport = sdsreplace(pconfig->webport, optarg);
                break;
            case 'u':
                pconfig->user = sdsreplace(pconfig->user, optarg);
                break;
            case 'l':
                pconfig->loglevel = strtoimax(optarg, NULL, 10);
                break;
            case 's':
                pconfig->sslport = sdsreplace(pconfig->sslport, optarg);
                pconfig->ssl = strcmp(optarg, "0") == 0 ? false : true;
                break;
            case 'r':
                pconfig->mixramp = true;
                break;
            case 'p':
                pconfig->publish = true;
                break;
            case 'd':
                pconfig->webdav = true;
                pconfig->publish = true;
                break;
            case 'y':
                pconfig->syscmds = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [OPTION]...\n"
                    "myMPD configuration utility, for details look at https://github.com/jcorporation/myMPD\n"
                    "WARNING: this tool overwrites your mympd.conf\n\n"
                    "-c, --mpdconf <path>     path to mpd.conf (default: /etc/mpd.conf)\n"
                    "-e  --mpdexe <path>      path to mpd executable (default: /usr/bin/mpd)\n"
                    "-m, --mympdconf <path>   path to mympd.conf (default: /etc/mympd.conf)\n"
                    "-w, --webport <port>     http port (default: 80)\n"
                    "-s, --sslport <port>     ssl port (default: 443, 0 to disable ssl)\n"
                    "-u, --user <username>    username (default: mympd)\n"
                    "-l, --loglevel <number>  loglevel (default: 2)\n"
                    "-r, --mixramp            enable mixramp settings (default: disabled)\n"
                    "-p, --publish            enable publishing feature (default: disabled)\n"
                    "-d, --webdav             enable webdav support (default: disabled)\n"
                    "-y, --syscmds            enable syscmds (default: disabled)\n"
                    "-h, --help               this help\n\n"
                    , argv[0]);
                return false;
        }
    }
    return true;
}

void set_defaults(struct t_config *pconfig) {
    pconfig->host = sdsnew("/run/mpd/socket");
    pconfig->port = 6600;
    pconfig->pass = sdsempty();
    pconfig->music_directory = sdsempty();
    pconfig->playlist_directory = sdsempty();
    pconfig->stickers = false;
    pconfig->regex = false;
    pconfig->mpd_conf = sdsnew("/etc/mpd.conf");
    pconfig->mpd_exe = sdsnew("/usr/bin/mpd");
    pconfig->mympd_conf = sdsnew("/etc/mympd.conf");
    pconfig->webport = sdsnew("80");
    pconfig->sslport = sdsnew("443");
    pconfig->user = sdsnew("mympd");
    pconfig->loglevel = 2;
    pconfig->ssl = true;
    pconfig->mixramp = false;
    pconfig->publish = false;
    pconfig->webdav = false;
    pconfig->syscmds = false;
}

bool write_mympd_conf(struct t_config *pconfig) {
    printf("Writing %s\n", pconfig->mympd_conf);
    sds tmp_file = sdscatfmt(sdsempty(), "%s.XXXXXX", pconfig->mympd_conf);
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        fprintf(stderr, "Can't open %s for write\n", tmp_file);
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    fprintf(fp, "# myMPD configuration file\n"
        "#\n"
        "# SPDX-License-Identifier: GPL-2.0-or-later\n"
        "# myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>\n"
        "# https://github.com/jcorporation/mympd\n"
        "#\n\n"
        "[mpd]\n"
        "#Connection to mpd, unix socket or host/port, socket preferred\n"
        "#host = /run/mpd/socket\n"
        "#host = 127.0.0.1\n"
        "#port = 6600\n"
        "#pass = \n"
        "host = %s\n",
        pconfig->host);
    if (strncmp(pconfig->host, "/", 1) != 0) {
        fprintf(fp, "port = %d\n", pconfig->port);
    }
    if (sdslen(pconfig->pass) > 0) {
        fprintf(fp, "pass = %s\n", pconfig->pass);
    }
    fprintf(fp, "\n#absolut path of music_directory of mpd\n"
        "#none = no local music_directory\n"
        "#auto = get music_directory from mpd (only supported, if connected to mpd socket)\n");
    if (strncmp(pconfig->host, "/", 1) == 0) {
        fprintf(fp, "musicdirectory = auto\n");
    }
    else {
        fprintf(fp, "musicdirectory = %s\n", pconfig->music_directory);
    }
    fprintf(fp, "\n#absolut path of mpd playlist_directory\n"
        "playlistdirectory = %s\n\n", pconfig->playlist_directory);
    fprintf(fp, "#MPD compiled with regex support\n"
        "regex = %s\n", (pconfig->regex == true ? "true" : "false"));

    fprintf(fp, "\n\n[webserver]\n"
        "#Webserver options\n"
        "webport = %s\n\n"
        "#Enable ssl\n"
        "#Certificates are generated under /var/lib/mympd/ssl/\n"
        "ssl = %s\n",
        pconfig->webport,
        (pconfig->ssl == true ? "true" : "false"));
    if (pconfig->ssl == true) {
        fprintf(fp, "sslport = %s\n", pconfig->sslport);
    }
    else {
        fprintf(fp, "#sslport = 443\n");
    }
    fprintf(fp, "\n#Publishes some mpd and myMPD directories\n"
        "publish = %s\n\n"
        "#Webdav support, publish must be set to true\n"
        "webdav = %s\n",
        (pconfig->publish == true ? "true" : "false"),
        (pconfig->webdav == true ? "true" : "false"));

    fprintf(fp, "\n\n[mympd]\n"
        "Loglevel\n"
        "#0 = error\n"
        "#1 = warn\n"
        "#2 = info\n"
        "#3 = verbose\n"
        "#4 = debug\n"
        "loglevel = %d\n\n"
        "#myMPD user\n"
        "#group is the primary group of this user\n"
        "user = %s\n\n"
        "#Usage of stickers for play statistics\n"
        "stickers = %s\n\n"
        "#Mixrampdb settings in gui\n"
        "mixramp = %s\n\n"
        "#Enable system commands defined in syscmds section\n"
        "syscmds = %s\n",
        pconfig->loglevel,
        pconfig->user,
        (pconfig->stickers == true ? "true" : "false"),
        (pconfig->mixramp == true ? "true" : "false"),
        (pconfig->syscmds == true ? "true" : "false"));
    
    fprintf(fp, "\n\n[syscmds]\n"
        "Shutdown = sudo /sbin/halt\n"
        "#To use this command add following lines to /etc/sudoers (without #)\n"
        "#Cmnd_Alias MYMPD_CMDS = /sbin/halt\n"
        "#mympd ALL=NOPASSWD: MYMPD_CMDS\n");
    
    fclose(fp);
    sds conf_file = sdscatfmt(sdsempty(), "%s", pconfig->mympd_conf);
    int rc = rename(tmp_file, conf_file);
    if (rc == -1) {
        fprintf(stderr, "Renaming file from %s to %s failed\n", tmp_file, conf_file);
    }
    sdsfree(tmp_file);
    sdsfree(conf_file);
    return true;
}

int main(int argc, char **argv) {
    int rc = EXIT_SUCCESS;
    
    struct t_config mpd_config;
    set_defaults(&mpd_config);

    if (parse_options(&mpd_config, argc, argv) == false) {
        rc = EXIT_FAILURE;
        goto cleanup;
    }

    if (parse_mpd_conf(&mpd_config) == false) {
        fprintf(stderr, "Error parsing %s\n", mpd_config.mpd_conf);
        rc = EXIT_FAILURE;
        goto cleanup;
    }
 
    if (check_ldd(&mpd_config) == false) {
        fprintf(stderr, "Error executing ldd\n");
        //Expect that mpd is compiled with regex support
        mpd_config.regex = true;
    }

    write_mympd_conf(&mpd_config);

    cleanup:
    sdsfree(mpd_config.host);
    sdsfree(mpd_config.pass);
    sdsfree(mpd_config.music_directory);
    sdsfree(mpd_config.playlist_directory);
    sdsfree(mpd_config.webport);
    sdsfree(mpd_config.sslport);
    sdsfree(mpd_config.user);
    sdsfree(mpd_config.mpd_conf);
    sdsfree(mpd_config.mpd_exe);
    sdsfree(mpd_config.mympd_conf);
    return rc;
}
