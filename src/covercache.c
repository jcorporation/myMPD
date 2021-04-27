/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "list.h"
#include "mympd_config_defs.h"
#include "utility.h"
#include "log.h"
#include "covercache.h"

bool write_covercache_file(const char *workdir, const char *uri, const char *mime_type, sds binary) {
    bool rc = false;
    sds filename = sdsnew(uri);
    uri_to_filename(filename);
    sds tmp_file = sdscatfmt(sdsempty(), "%s/covercache/%s.XXXXXX", workdir, filename);
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write: %s", tmp_file, strerror(errno));
    }
    else {
        FILE *fp = fdopen(fd, "w");
        fwrite(binary, 1, sdslen(binary), fp);
        fclose(fp);
        sds ext = get_ext_by_mime_type(mime_type);
        sds cover_file = sdscatfmt(sdsempty(), "%s/covercache/%s.%s", workdir, filename, ext);
        if (rename(tmp_file, cover_file) == -1) {
            MYMPD_LOG_ERROR("Rename file from \"%s\" to \"%s\" failed: %s", tmp_file, cover_file, strerror(errno));
            if (unlink(tmp_file) != 0) {
                MYMPD_LOG_ERROR("Error removing file \"%s\": %s", tmp_file, strerror(errno));
            }
        }
        MYMPD_LOG_DEBUG("Write covercache file \"%s\" for uri \"%s\"", cover_file, uri);
        sdsfree(ext);
        sdsfree(cover_file);
        rc = true;
    }
    sdsfree(tmp_file);
    sdsfree(filename);
    return rc;
}

int clear_covercache(const char *workdir, int keepdays) {
    int num_deleted = 0;
    time_t now = time(NULL) - keepdays * 24 * 60 * 60;
    
    sds covercache = sdscatfmt(sdsempty(), "%s/covercache", workdir);
    MYMPD_LOG_NOTICE("Cleaning covercache %s", covercache);
    MYMPD_LOG_DEBUG("Remove files older than %ld sec", now);
    DIR *covercache_dir = opendir(covercache);
    if (covercache_dir != NULL) {
        struct dirent *next_file;
        while ( (next_file = readdir(covercache_dir)) != NULL ) {
            if (strncmp(next_file->d_name, ".", 1) != 0) {
                sds filepath = sdscatfmt(sdsempty(), "%s/%s", covercache, next_file->d_name);
                struct stat status;
                if (stat(filepath, &status) == 0) {
                    if (status.st_mtime < now) {
                        MYMPD_LOG_DEBUG("Deleting %s: %ld", filepath, status.st_mtime);
                        if (unlink(filepath) != 0) {
                            MYMPD_LOG_ERROR("Error removing file \"%s\": %s", filepath, strerror(errno));
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
        MYMPD_LOG_ERROR("Error opening directory %s: %s", covercache, strerror(errno));
    }
    MYMPD_LOG_NOTICE("Deleted %d files from covercache", num_deleted);
    sdsfree(covercache);
    return num_deleted;
}
