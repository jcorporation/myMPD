/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of: ympd (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   ympd project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <dirent.h>
#include <stdbool.h>

#include "../dist/src/sds/sds.h"
#include "log.h"
#include "list.h"
#include "config_defs.h"
#include "mympd_api.h"
#include "cert.h"
#include "utility.h"
#include "handle_options.h"

//private function definitions
static bool smartpls_init(t_config *config, const char *name, const char *value);
static void clear_covercache(t_config *config);

//global functions
bool smartpls_default(t_config *config) {
    bool rc = true;
    
    rc = smartpls_init(config, "myMPDsmart-bestRated", 
        "{\"type\": \"sticker\", \"sticker\": \"like\", \"maxentries\": 200}\n");
    if (rc == false) {
        return rc;
    }
    rc = smartpls_init(config, "myMPDsmart-mostPlayed", 
        "{\"type\": \"sticker\", \"sticker\": \"playCount\", \"maxentries\": 200}\n");
    if (rc == false) {
        return rc;
    }
    rc = smartpls_init(config, "myMPDsmart-newestSongs", 
        "{\"type\": \"newest\", \"timerange\": 604800}\n");

    return rc;
}

bool handle_option(t_config *config, char *cmd, sds option) {
    #define MATCH_OPTION(o) strcasecmp(option, o) == 0

    if (MATCH_OPTION("cert_remove")) {
        sds ssldir = sdscatprintf(sdsempty(), "%s/ssl", config->varlibdir);
        bool rc = cleanup_certificates(ssldir, "server");
        sds_free(ssldir);
        return rc;
    }
    else if (MATCH_OPTION("ca_remove")) {
        sds ssldir = sdscatprintf(sdsempty(), "%s/ssl", config->varlibdir);
        bool rc = cleanup_certificates(ssldir, "ca");
        sds_free(ssldir);
        return rc;
    }
    else if (MATCH_OPTION("certs_create")) {
        sds ssldir = sdscatprintf(sdsempty(), "%s/ssl", config->varlibdir);
        int testdir_rc = testdir("SSL certificates", ssldir, true);
        sds_free(ssldir);
        if (testdir_rc < 2) {
            return create_certificates(ssldir, config->ssl_san);
        }
        return true;
    }
    else if (MATCH_OPTION("reset_state")) {
        mympd_api_settings_delete(config);
        return true;
    }
    else if (MATCH_OPTION("reset_smartpls")) {
        return smartpls_default(config);
    }
    else if (MATCH_OPTION("reset_lastplayed")) {
        sds lpfile = sdscatprintf(sdsempty(), "%s/state/last_played", config->varlibdir);
        int rc = unlink(lpfile);
        sds_free(lpfile);
        if (rc == 0) {
            return true;
        }
        else {
            LOG_ERROR("last_played file not exists");
            return false;
        }
    }
    else if (MATCH_OPTION("clear_covercache")) {
        clear_covercache(config);
        return true;
    }
    else {
        printf("myMPD %s\n"
               "Copyright (C) 2018-2019 Juergen Mang <mail@jcgames.de>\n"
               "https://github.com/jcorporation/myMPD\n\n"
               "Usage: %s [/etc/mympd.conf] <command>\n"
               "Commands (you should stop mympd before):\n"
               "  certs_create:     create ssl certificates\n"
               "  cert_remove:      remove server certificates\n"
               "  ca_remove:        remove ca certificates\n"
               "  reset_state:      delete all myMPD settings\n"
               "  reset_smartpls:   create default smart playlists\n"
               "  reset_lastplayed: truncates last played list\n"
               "  clear_covercache: empties the covercache directory\n",
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
    
    sds tmp_file = sdscatprintf(sdsempty(), "%s/smartpls/%s.XXXXXX", config->varlibdir, name);

    int fd;
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sds_free(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    fprintf(fp, "%s", value);
    fclose(fp);
    sds cfg_file = sdscatprintf(sdsempty(), "%s/smartpls/%s", config->varlibdir, name);
    if (rename(tmp_file, cfg_file) == -1) {
        LOG_ERROR("Renaming file from %s to %s failed", tmp_file, cfg_file);
        sds_free(tmp_file);
        sds_free(cfg_file);
        return false;
    }
    sds_free(tmp_file);
    sds_free(cfg_file);
    return true;
}

static void clear_covercache(t_config *config) {
    sds covercache = sdscatprintf(sdsempty(), "%s/covercache", config->varlibdir);
    LOG_INFO("Cleaning covercache %s", covercache);
    DIR *covercache_dir = opendir(covercache);
    if (covercache_dir != NULL) {
        struct dirent *next_file;
        sds filepath = sdsempty();
        while ( (next_file = readdir(covercache_dir)) != NULL ) {
            if (strncmp(next_file->d_name, ".", 1) != 0) {
                filepath = sdscatprintf(sdsempty(), "%s/%s", covercache, next_file->d_name);
                if (unlink(filepath) != 0) {
                    LOG_ERROR("Error deleting %s", filepath);
                }
            }
        }
        closedir(covercache_dir);
    }
    else {
        LOG_ERROR("Error opening directory %s", covercache);
    }
    sds_free(covercache);
}
