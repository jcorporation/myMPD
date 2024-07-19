/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Embedded image functions for ID3
 */

#include "compile_time.h"
#include "src/web_server/albumart_id3.h"

#include "src/lib/cache_disk_images.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"

#include <id3tag.h>
#include <stdlib.h>

/**
 * Extracts albumart from id3v2 tagged files
 * @param cachedir covercache directory
 * @param uri song uri
 * @param media_file full path to the song
 * @param binary pointer to already allocates sds string to hold the image
 * @param covercache true = covercache is enabled
 * @param offset number of embedded image to extract
 * @return true on success, else false
 */
bool handle_coverextract_id3(sds cachedir, const char *uri, const char *media_file,
        sds *binary, bool covercache, int offset)
{
    bool rc = false;
    MYMPD_LOG_DEBUG(NULL, "Exctracting coverimage from %s", media_file);
    struct id3_file *file_struct = id3_file_open(media_file, ID3_FILE_MODE_READONLY);
    if (file_struct == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can't parse id3_file: %s", media_file);
        return false;
    }
    struct id3_tag *tags = id3_file_tag(file_struct);
    if (tags == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can't read id3 tags from file: %s", media_file);
        return false;
    }
    struct id3_frame *frame = id3_tag_findframe(tags, "APIC", (unsigned)offset);
    if (frame != NULL) {
        id3_length_t length = 0;
        const id3_byte_t *pic = id3_field_getbinarydata(id3_frame_field(frame, 4), &length);
        if (length > 0) {
            *binary = sdscatlen(*binary, pic, length);
            const char *mime_type = get_mime_type_by_magic_stream(*binary);
            if (mime_type != NULL) {
                if (covercache == true) {
                    sds filename = cache_disk_images_write_file(cachedir, DIR_CACHE_COVER, uri, mime_type, *binary, offset);
                    FREE_SDS(filename);
                }
                else {
                    MYMPD_LOG_DEBUG(NULL, "Covercache is disabled");
                }
                MYMPD_LOG_DEBUG(NULL, "Coverimage successfully extracted (%lu bytes)", (unsigned long)sdslen(*binary));
                rc = true;
            }
            else {
                MYMPD_LOG_WARN(NULL, "Could not determine mimetype, discarding image");
                sdsclear(*binary);
            }
        }
        else {
            MYMPD_LOG_WARN(NULL, "Embedded picture size is zero");
        }
    }
    else {
        MYMPD_LOG_DEBUG(NULL, "No embedded picture detected");
    }
    id3_file_close(file_struct);
    return rc;
}
