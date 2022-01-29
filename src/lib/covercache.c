/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "covercache.h"

#include "log.h"
#include "mimetype.h"
#include "sds_extras.h"
#include "utility.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

bool covercache_write_file(const char *cachedir, const char *uri, const char *mime_type, sds binary) {
    if (mime_type[0] == '\0') {
        MYMPD_LOG_WARN("Covercache file for \"%s\" not written, mime_type is empty", uri);
        return false;
    }
    const char *ext = get_ext_by_mime_type(mime_type);
    if (ext == NULL) {
        MYMPD_LOG_WARN("Covercache file for \"%s\" not written, could not determine file extension", uri);
        return false;
    }
    sds filename = sdsnew(uri);
    sds_sanitize_filename(filename);
    sds filepath = sdscatfmt(sdsempty(), "%s/covercache/%s.%s", cachedir, filename, ext);
    bool rc = write_data_to_file(filepath, binary, sdslen(binary));
    FREE_SDS(filename);
    FREE_SDS(filepath);
    return rc;
}

int covercache_clear(const char *cachedir, int keepdays) {
    int num_deleted = 0;
    bool error = false;
    time_t expire_time = time(NULL) - (time_t)(keepdays * 24 * 60 * 60);

    sds covercache = sdscatfmt(sdsempty(), "%s/covercache", cachedir);
    MYMPD_LOG_NOTICE("Cleaning covercache \"%s\"", covercache);
    MYMPD_LOG_DEBUG("Remove files older than %lld sec", (long long)expire_time);
    errno = 0;
    DIR *covercache_dir = opendir(covercache);
    if (covercache_dir == NULL) {
        MYMPD_LOG_ERROR("Error opening directory \"%s\"", covercache);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(covercache);
        return -1;
    }

    struct dirent *next_file;
    sds filepath = sdsempty();
    while ((next_file = readdir(covercache_dir)) != NULL ) {
        if (next_file->d_type != DT_REG) {
            continue;
        }
        sdsclear(filepath);
        filepath = sdscatfmt(filepath, "%s/%s", covercache, next_file->d_name);
        struct stat status;
        if (stat(filepath, &status) != 0) {
            continue;
        }
        if (status.st_mtime < expire_time) {
            MYMPD_LOG_DEBUG("Deleting \"%s\": %lld", filepath, (long long)status.st_mtime);
            errno = 0;
            if (unlink(filepath) != 0) {
                MYMPD_LOG_ERROR("Error removing file \"%s\"", filepath);
                MYMPD_LOG_ERRNO(errno);
                error = true;
            }
            else {
                num_deleted++;
            }
        }
    }
    closedir(covercache_dir);
    FREE_SDS(filepath);

    MYMPD_LOG_NOTICE("Deleted %d files from covercache", num_deleted);
    FREE_SDS(covercache);
    return error == false ? num_deleted : -1;
}
