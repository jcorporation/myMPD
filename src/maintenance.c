/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "log.h"
#include "list.h"
#include "config_defs.h"
#include "maintenance.h"

void clear_covercache(t_config *config) {
    time_t now = time(NULL) - config->covercache_keep_days * 24 * 60 * 60;
    
    sds covercache = sdscatfmt(sdsempty(), "%s/covercache", config->varlibdir);
    LOG_INFO("Cleaning covercache %s", covercache);
    LOG_DEBUG("Remove files older than %ld sec", now);
    DIR *covercache_dir = opendir(covercache);
    if (covercache_dir != NULL) {
        struct dirent *next_file;
        while ( (next_file = readdir(covercache_dir)) != NULL ) {
            if (strncmp(next_file->d_name, ".", 1) != 0) {
                sds filepath = sdscatfmt(sdsempty(), "%s/%s", covercache, next_file->d_name);
                struct stat status;
                if (stat(filepath, &status) == 0) {
                    LOG_DEBUG("%s: %ld\n", filepath, status.st_mtime);
                    if (status.st_mtime < now) {
                        if (unlink(filepath) != 0) {
                            LOG_ERROR("Error deleting %s", filepath);
                        }
                    }
                }
                sdsfree(filepath);
            }
        }
        closedir(covercache_dir);
    }
    else {
        LOG_ERROR("Error opening directory %s", covercache);
    }
    sdsfree(covercache);
}
