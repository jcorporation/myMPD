/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lyrics cache handling
 */

#include "compile_time.h"
#include "src/lib/cache_disk_lyrics.h"

#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/**
 * Returns the path / basename for an uri to save it in the lyrics cache
 * @param cachedir cache directory
 * @param uri uri of the song for the lyrics
 * @return path / basename as newly allocated sds string
 */
sds cache_disk_lyrics_get_name(const char *cachedir, const char *uri) {
    sds filename = sds_hash_sha1(uri);
    sds filepath = sdscatfmt(sdsempty(), "%s/%s/%S.json", cachedir, DIR_CACHE_LYRICS, filename);
    FREE_SDS(filename);
    return filepath;
}

/**
 * Writes the lyrics (as text buffer) to the lyrics cache,
 * filename is the hash of the full path
 * @param cachedir cache directory
 * @param uri uri of the song for the lyrics
 * @param str string to save
 * @return written filename (full path) as newly allocated sds string
 */
sds cache_disk_lyrics_write_file(const char *cachedir, const char *uri, const char *str) {
    MYMPD_LOG_DEBUG(NULL, "Writing lyrics cache for \"%s\"", uri);
    sds filepath = cache_disk_lyrics_get_name(cachedir, uri);
    MYMPD_LOG_DEBUG(NULL, "Writing lyrics cache file \"%s\"", filepath);
    bool rc = write_data_to_file(filepath, str, strlen(str));
    if (rc == false) {
        FREE_SDS(filepath);
    }
    return filepath;
}
