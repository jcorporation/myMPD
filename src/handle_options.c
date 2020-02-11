/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <dirent.h>
#include <stdbool.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "../dist/src/frozen/frozen.h"
#include "log.h"
#include "list.h"
#include "config_defs.h"
#include "config.h"
#include "mympd_api/mympd_api_utility.h"
#include "mympd_api/mympd_api_timer.h"
#include "mympd_api/mympd_api_settings.h"
#ifdef ENABLE_SSL
  #include "cert.h"
#endif
#include "utility.h"
#include "maintenance.h"
#include "handle_options.h"

//private function definitions
static bool smartpls_init(t_config *config, const char *name, const char *value);

//global functions
bool smartpls_default(t_config *config) {
    bool rc = true;
    char *line = NULL;
    size_t n = 0;
    ssize_t read;

    sds prefix = sdsempty();
    sds prefix_file = sdscatfmt(sdsempty(), "%s/state/smartpls_prefix", config->varlibdir);
    FILE *fp = fopen(prefix_file, "r");
    sdsfree(prefix_file);
    if (fp != NULL) {
        read = getline(&line, &n, fp);
        if (read > 0) {
            prefix = sdscat(prefix, line);
            FREE_PTR(line);
        }
        fclose(fp);
    }
    else {
        prefix = sdscat(prefix, "myMPDsmart");
    }
    
    sds smartpls_file = sdscatfmt(sdsempty(), "%s%sbestRated", prefix, (sdslen(prefix) > 0 ? "-" : ""));
    rc = smartpls_init(config, smartpls_file, 
        "{\"type\": \"sticker\", \"sticker\": \"like\", \"maxentries\": 200, \"minvalue\": 2, \"sort\": \"\"}");
    if (rc == false) {
        sdsfree(smartpls_file);
        sdsfree(prefix);
        return rc;
    }
    
    sdscrop(smartpls_file);
    smartpls_file = sdscatfmt(smartpls_file, "%s%smostPlayed", prefix, (sdslen(prefix) > 0 ? "-" : ""));
    rc = smartpls_init(config, smartpls_file, 
        "{\"type\": \"sticker\", \"sticker\": \"playCount\", \"maxentries\": 200, \"minvalue\": 0, \"sort\": \"\"}");
    if (rc == false) {
        sdsfree(smartpls_file);
        sdsfree(prefix);
        return rc;
    }
    
    sdscrop(smartpls_file);
    smartpls_file = sdscatfmt(smartpls_file, "%s%snewestSongs", prefix, (sdslen(prefix) > 0 ? "-" : ""));
    rc = smartpls_init(config, smartpls_file, 
        "{\"type\": \"newest\", \"timerange\": 604800, \"sort\": \"\"}");
    sdsfree(smartpls_file);
    sdsfree(prefix);
    
    return rc;
}

bool handle_option(t_config *config, char *cmd, sds option) {
    #define MATCH_OPTION(o) strcasecmp(option, o) == 0
    
    if (MATCH_OPTION("reset_state")) {
        mympd_api_settings_delete(config);
        return true;
    }
    else if (MATCH_OPTION("reset_smartpls")) {
        return smartpls_default(config);
    }
    else if (MATCH_OPTION("reset_lastplayed")) {
        sds lpfile = sdscatfmt(sdsempty(), "%s/state/last_played", config->varlibdir);
        int rc = unlink(lpfile);
        sdsfree(lpfile);
        if (rc == 0) {
            return true;
        }
        else {
            LOG_ERROR("last_played file does not exist");
            return false;
        }
    }
    #ifdef ENABLE_SSL
    else if (MATCH_OPTION("cert_remove")) {
        sds ssldir = sdscatfmt(sdsempty(), "%s/ssl", config->varlibdir);
        bool rc = cleanup_certificates(ssldir, "server");
        sdsfree(ssldir);
        return rc;
    }
    else if (MATCH_OPTION("ca_remove")) {
        sds ssldir = sdscatfmt(sdsempty(), "%s/ssl", config->varlibdir);
        bool rc = cleanup_certificates(ssldir, "ca");
        sdsfree(ssldir);
        return rc;
    }
    else if (MATCH_OPTION("certs_create")) {
        sds ssldir = sdscatfmt(sdsempty(), "%s/ssl", config->varlibdir);
        int testdir_rc = testdir("SSL certificates", ssldir, true);
        sdsfree(ssldir);
        if (testdir_rc < 2) {
            return create_certificates(ssldir, config->ssl_san);
        }
        return true;
    }
    #endif
    else if (MATCH_OPTION("crop_covercache")) {
        clear_covercache(config, -1);
        return true;
    }
    else if (MATCH_OPTION("clear_covercache")) {
        clear_covercache(config, 0);
        return true;
    }
    else if (MATCH_OPTION("dump_config")) {
        return mympd_dump_config();
    }
    else {
        printf("myMPD %s\n"
               "Copyright (C) 2018-2020 Juergen Mang <mail@jcgames.de>\n"
               "https://github.com/jcorporation/myMPD\n\n"
               "Usage: %s [/etc/mympd.conf] <command>\n"
               "Commands (you should stop mympd before):\n"
             #ifdef ENABLE_SSL
               "  certs_create:     create ssl certificates\n"
               "  cert_remove:      remove server certificates\n"
               "  ca_remove:        remove ca certificates\n"
             #endif
               "  reset_state:      delete all myMPD settings\n"
               "  reset_smartpls:   create default smart playlists\n"
               "  reset_lastplayed: truncates last played list\n"
               "  crop_covercache:  crops the covercache directory\n"
               "  clear_covercache: empties the covercache directory\n"
               "  dump_config:      writes default mympd.conf\n"
               "  help:             display this help\n",
               MYMPD_VERSION,
               cmd
        );
    }
    
    return false;
}

//private functions
static bool smartpls_init(t_config *config, const char *name, const char *value) {
    if (!validate_string(name)) {
        return false;
    }
    
    sds tmp_file = sdscatfmt(sdsempty(), "%s/smartpls/%s.XXXXXX", config->varlibdir, name);
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    fprintf(fp, "%s", value);
    fclose(fp);
    sds cfg_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", config->varlibdir, name);
    if (rename(tmp_file, cfg_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, cfg_file);
        sdsfree(tmp_file);
        sdsfree(cfg_file);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(cfg_file);
    return true;
}
