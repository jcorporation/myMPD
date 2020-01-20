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

int clear_covercache(t_config *config, int keepdays) {
    int num_deleted = 0;
    if (config->covercache == false) {
        LOG_WARN("Covercache is disabled");
        return 0;
    }
    if (keepdays == -1) {
        keepdays = config->covercache_keep_days;
    }
    time_t now = time(NULL) - keepdays * 24 * 60 * 60;
    
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
                    if (status.st_mtime < now) {
                        LOG_DEBUG("Deleting %s: %ld", filepath, status.st_mtime);
                        if (unlink(filepath) != 0) {
                            LOG_ERROR("Error deleting %s", filepath);
                        }
                        else {
                            num_deleted++;
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
    LOG_INFO("Deleted %d files from covercache", num_deleted);
    sdsfree(covercache);
    return num_deleted;
}
