/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/cache_disk_images.h"

#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"

#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

/**
 * Returns the path / basename for an uri to save it in the image cache
 * @param cachedir cache directory
 * @param type image type
 * @param uri uri of the song for the cover
 * @param offset number of the coverimage
 * @return path / basename as newly allocated sds string
 */
sds cache_disk_images_get_basename(const char *cachedir, const char *type, const char *uri, int offset) {
    sds filename = sds_hash_sha1(uri);
    sds filepath = sdscatfmt(sdsempty(), "%s/%s/%S-%i", cachedir, type, filename, offset);
    FREE_SDS(filename);
    return filepath;
}

/**
 * Writes the image (as binary buffer) to the image cache,
 * filename is the hash of the full path
 * @param cachedir cache directory
 * @param type image type
 * @param uri uri of the song for the cover
 * @param mime_type mime_type of binary buffer
 * @param binary binary data to save
 * @param offset number of the coverimage
 * @return written filename (full path) as newly allocated sds string
 */
sds cache_disk_images_write_file(sds cachedir, const char *type, const char *uri, const char *mime_type, sds binary, int offset) {
    if (mime_type[0] == '\0') {
        MYMPD_LOG_WARN(NULL, "Covercache file for \"%s\" not written, mime_type is empty", uri);
        return false;
    }
    const char *ext = get_ext_by_mime_type(mime_type);
    if (ext == NULL) {
        MYMPD_LOG_WARN(NULL, "Image cache file for \"%s\" not written, could not determine file extension", uri);
        return false;
    }
    MYMPD_LOG_DEBUG(NULL, "Writing image cache for \"%s\"", uri);
    sds filepath = cache_disk_images_get_basename(cachedir, type, uri, offset);
    filepath = sdscatfmt(filepath, ".%s", ext);
    MYMPD_LOG_DEBUG(NULL, "Writing image cache file \"%s\"", filepath);
    bool rc = write_data_to_file(filepath, binary, sdslen(binary));
    if (rc == false) {
        FREE_SDS(filepath);
    }
    return filepath;
}
