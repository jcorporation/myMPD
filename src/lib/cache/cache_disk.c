/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief General disk cache handling
 */

#include "compile_time.h"
#include "src/lib/cache/cache_disk.h"

#include "src/lib/datetime.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/sds/sds_extras.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// private definitions

static int crop_dir(sds cache_basedir, const char *type, int keepdays);

// public functions

/**
 * Clears the caches unconditionally
 * @param config pointer to static config
 */
void cache_disk_clear(struct t_config *config) {
    crop_dir(config->cachedir, DIR_CACHE_COVER, 0);
    crop_dir(config->cachedir, DIR_CACHE_HTTP, 0);
    crop_dir(config->cachedir, DIR_CACHE_LYRICS, 0);
    crop_dir(config->cachedir, DIR_CACHE_THUMBS, 0);
    crop_dir(config->cachedir, DIR_CACHE_MISC, 0);
}

/**
 * Crops the caches respecting the keep_days settings
 * @param config pointer to static config
 */
void cache_disk_crop(struct t_config *config) {
    if (config->cache_cover_keep_days > CACHE_DISK_DISABLED) {
        crop_dir(config->cachedir, DIR_CACHE_COVER, config->cache_cover_keep_days);
    }
    if (config->cache_lyrics_keep_days > CACHE_DISK_DISABLED) {
        crop_dir(config->cachedir, DIR_CACHE_LYRICS, config->cache_lyrics_keep_days);
    }
    if (config->cache_thumbs_keep_days > CACHE_DISK_DISABLED) {
        crop_dir(config->cachedir, DIR_CACHE_THUMBS, config->cache_thumbs_keep_days);
    }
    if (config->cache_http_keep_days > CACHE_DISK_DISABLED) {
        crop_dir(config->cachedir, DIR_CACHE_HTTP, config->cache_http_keep_days);
    }
    crop_dir(config->cachedir, DIR_CACHE_MISC, config->cache_misc_keep_days);
}

/**
 * Crops a specific cache dir
 * @param cache_basedir cache basedir
 * @param type cache subdir
 * @param keepdays expiration in days
 * @return deleted file count on success, else -1
 */
static int crop_dir(sds cache_basedir, const char *type, int keepdays) {
    assert(keepdays >= 0);
    int num_deleted = 0;
    bool rc = true;
    const time_t now = time(NULL);
    const time_t expire_time = now - ((time_t)keepdays * 24LL * 60LL * 60LL);

    sds cache_path = sdscatfmt(sdsempty(), "%S/%s", cache_basedir, type);
    char fmt_time[32];
    readable_time(fmt_time, expire_time);
    MYMPD_LOG_INFO(NULL, "Cleaning %s cache \"%s\", removing files older than %s ", type, cache_path, fmt_time);
    errno = 0;
    DIR *cache_dir = opendir(cache_path);
    if (cache_dir == NULL) {
        MYMPD_LOG_ERROR(NULL, "Error opening directory \"%s\"", cache_path);
        MYMPD_LOG_ERRNO(NULL, errno);
        FREE_SDS(cache_path);
        return -1;
    }
    errno = 0;
    int dir_fd = dirfd(cache_dir);
    if (dir_fd == -1) {
        MYMPD_LOG_ERROR(NULL, "Error getting directory file descriptor for \"%s\"", cache_path);
        MYMPD_LOG_ERRNO(NULL, errno);
        closedir(cache_dir);
        FREE_SDS(cache_path);
        return -1;
    }

    struct dirent *next_file;
    while ((next_file = readdir(cache_dir)) != NULL ) {
        struct stat st;
        if (fstatat(dir_fd, next_file->d_name, &st, AT_SYMLINK_NOFOLLOW) != 0) {
            continue;
        }
        if (S_ISREG(st.st_mode) == false) {
            continue;
        }
        time_t mtime = st.st_mtime;
        if (mtime < expire_time) {
            MYMPD_LOG_DEBUG(NULL, "Deleting \"%s/%s\"", cache_path, next_file->d_name);
            if (rm_file_at(dir_fd, next_file->d_name) == true) {
                num_deleted++;
            }
        }
    }
    closedir(cache_dir);

    MYMPD_LOG_NOTICE(NULL, "Deleted %d files from %s cache", num_deleted, type);
    FREE_SDS(cache_path);
    return rc == true ? num_deleted : -1;
}
