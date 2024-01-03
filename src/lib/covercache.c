/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/covercache.h"

#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>


/**
 * Writes the coverimage (as binary buffer) to the covercache,
 * filename is the hash of the full path
 * @param cachedir covercache directory
 * @param uri uri of the song for the cover
 * @param mime_type mime_type of binary buffer
 * @param binary binary data to save
 * @param offset number of the coverimage
 * @return true on success else false
 */
bool covercache_write_file(sds cachedir, const char *uri, const char *mime_type, sds binary, int offset) {
    if (mime_type[0] == '\0') {
        MYMPD_LOG_WARN(NULL, "Covercache file for \"%s\" not written, mime_type is empty", uri);
        return false;
    }
    const char *ext = get_ext_by_mime_type(mime_type);
    if (ext == NULL) {
        MYMPD_LOG_WARN(NULL, "Covercache file for \"%s\" not written, could not determine file extension", uri);
        return false;
    }
    MYMPD_LOG_DEBUG(NULL, "Writing covercache for \"%s\"", uri);
    sds filename = sds_hash_sha1(uri);
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%S-%i.%s", cachedir, DIR_CACHE_COVER, filename, offset, ext);
    MYMPD_LOG_DEBUG(NULL, "Writing covercache file \"%s\"", filepath);
    bool rc = write_data_to_file(filepath, binary, sdslen(binary));
    FREE_SDS(filename);
    FREE_SDS(filepath);
    return rc;
}

/**
 * Crops the covercache
 * @param cachedir covercache directory
 * @param keepdays delete files older than days
 * @return deleted filecount on success else -1
 */
int covercache_clear(sds cachedir, int keepdays) {
    int num_deleted = 0;
    bool rc = true;
    time_t expire_time = time(NULL) - (time_t)(keepdays * 24 * 60 * 60);

    sds covercache = sdscatfmt(sdsempty(), "%S/%s", cachedir, DIR_CACHE_COVER);
    char fmt_time[32];
    readable_time(fmt_time, expire_time);
    MYMPD_LOG_INFO(NULL, "Cleaning covercache \"%s\", removing files older than %s ", covercache, fmt_time);
    errno = 0;
    DIR *covercache_dir = opendir(covercache);
    if (covercache_dir == NULL) {
        MYMPD_LOG_ERROR(NULL, "Error opening directory \"%s\"", covercache);
        MYMPD_LOG_ERRNO(NULL, errno);
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
        filepath = sdscatfmt(filepath, "%S/%s", covercache, next_file->d_name);
        time_t mtime = get_mtime(filepath);
        if (mtime < expire_time) {
            MYMPD_LOG_DEBUG(NULL, "Deleting \"%s\"", filepath);
            rc = rm_file(filepath);
            if (rc == true) {
                num_deleted++;
            }
        }
    }
    closedir(covercache_dir);
    FREE_SDS(filepath);

    MYMPD_LOG_NOTICE(NULL, "Deleted %d files from covercache", num_deleted);
    FREE_SDS(covercache);
    return rc == true ? num_deleted : -1;
}
