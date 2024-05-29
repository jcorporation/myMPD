/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/albumart_flac.h"

#include "src/lib/cache_disk_cover.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"

#include <FLAC/metadata.h>

/**
 * Extracts albumart from vorbis tagged files
 * @param cachedir covercache directory
 * @param uri song uri
 * @param media_file full path to the song
 * @param binary pointer to already allocates sds string to hold the image
 * @param is_ogg true if it is a ogg file, false if it is a flac file
 * @param covercache true = covercache is enabled
 * @param offset number of embedded image to extract
 * @return true on success, else false
 */
bool handle_coverextract_flac(sds cachedir, const char *uri, const char *media_file,
        sds *binary, bool is_ogg, bool covercache, int offset)
{
    bool rc = false;
    MYMPD_LOG_DEBUG(NULL, "Exctracting coverimage from %s", media_file);
    FLAC__StreamMetadata *metadata = NULL;

    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();

    if(! (is_ogg? FLAC__metadata_chain_read_ogg(chain, media_file) : FLAC__metadata_chain_read(chain, media_file)) ) {
        MYMPD_LOG_ERROR(NULL, "Error reading metadata from \"%s\"", media_file);
        FLAC__metadata_chain_delete(chain);
        return false;
    }

    FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
    FLAC__metadata_iterator_init(iterator, chain);
    if (iterator == NULL) {
        MYMPD_LOG_ERROR(NULL, "Error initializing iterator for \"%s\"", media_file);
        FLAC__metadata_chain_delete(chain);
        return false;
    }
    int i = 0;
    do {
        FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
        if (block->type == FLAC__METADATA_TYPE_PICTURE) {
            if (i == offset) {
                metadata = block;
                break;
            }
            i++;
        }
    } while (FLAC__metadata_iterator_next(iterator) && metadata == NULL);

    if (metadata == NULL) {
        MYMPD_LOG_DEBUG(NULL, "No embedded picture detected");
    }
    else if (metadata->data.picture.data_length > 0) {
        *binary = sdscatlen(*binary, metadata->data.picture.data, metadata->data.picture.data_length);
        const char *mime_type = get_mime_type_by_magic_stream(*binary);
        if (mime_type != NULL) {
            if (covercache == true) {
                cache_disk_cover_write_file(cachedir, uri, mime_type, *binary, offset);
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
    FLAC__metadata_iterator_delete(iterator);
    FLAC__metadata_chain_delete(chain);
    return rc;
}
