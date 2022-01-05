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
    bool rc = false;
    sds filename = sdsnew(uri);
    sds_sanitize_filename(filename);
    sds tmp_file = sdscatfmt(sdsempty(), "%s/covercache/%s.XXXXXX", cachedir, filename);
    errno = 0;
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", tmp_file);
        MYMPD_LOG_ERRNO(errno);
    }
    else {
        FILE *fp = fdopen(fd, "w");
        fwrite(binary, 1, sdslen(binary), fp);
        fclose(fp);
        sds cover_file = sdscatfmt(sdsempty(), "%s/covercache/%s.%s", cachedir, filename, ext);
        errno = 0;
        if (rename(tmp_file, cover_file) == -1) {
            MYMPD_LOG_ERROR("Rename file from \"%s\" to \"%s\" failed", tmp_file, cover_file);
            MYMPD_LOG_ERRNO(errno);
            errno = 0;
            if (unlink(tmp_file) != 0) {
                MYMPD_LOG_ERROR("Error removing file \"%s\"", tmp_file);
                MYMPD_LOG_ERRNO(errno);
            }
        }
        MYMPD_LOG_DEBUG("Write covercache file \"%s\" for uri \"%s\"", cover_file, uri);
        FREE_SDS(cover_file);
        rc = true;
    }
    FREE_SDS(tmp_file);
    FREE_SDS(filename);
    return rc;
}

int covercache_clear(const char *cachedir, int keepdays) {
    int num_deleted = 0;
    bool error = false;
    time_t expire_time = time(NULL) - (long)(keepdays * 24 * 60 * 60);

    sds covercache = sdscatfmt(sdsempty(), "%s/covercache", cachedir);
    MYMPD_LOG_NOTICE("Cleaning covercache \"%s\"", covercache);
    MYMPD_LOG_DEBUG("Remove files older than %ld sec", expire_time);
    errno = 0;
    DIR *covercache_dir = opendir(covercache);
    if (covercache_dir != NULL) {
        struct dirent *next_file;
        sds filepath = sdsempty();
        while ((next_file = readdir(covercache_dir)) != NULL ) {
            if (next_file->d_type == DT_REG) {
                filepath = sdscatfmt(filepath, "%s/%s", covercache, next_file->d_name);
                struct stat status;
                if (stat(filepath, &status) == 0) {
                    if (status.st_mtime < expire_time) {
                        MYMPD_LOG_DEBUG("Deleting \"%s\": %ld", filepath, status.st_mtime);
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
                sdsclear(filepath);
            }
        }
        closedir(covercache_dir);
        FREE_SDS(filepath);
    }
    else {
        MYMPD_LOG_ERROR("Error opening directory \"%s\"", covercache);
        MYMPD_LOG_ERRNO(errno);
    }
    MYMPD_LOG_NOTICE("Deleted %d files from covercache", num_deleted);
    FREE_SDS(covercache);
    return error == false ? num_deleted : -1;
}
