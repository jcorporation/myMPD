/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Read lyrics from vorbis comments
 */

#include "compile_time.h"
#include "src/mympd_api/lyrics_flac.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

#include <FLAC/metadata.h>
#include <assert.h>
#include <string.h>

/**
 * Extracts unsynced lyrics from a vorbis comment
 * @param extracted t_list struct to append found lyrics
 * @param media_file absolute filename to read lyrics from
 * @param is_ogg true if is a ogg file else false (flac)
 * @param comment_name name of vorbis comment with the lyrics
 * @param synced true for synced lyrics else false
 */
void lyricsextract_flac(struct t_list *extracted, sds media_file, bool is_ogg, const char *comment_name, bool synced) {
    #ifdef MYMPD_ENABLE_FLAC
    MYMPD_LOG_DEBUG(NULL, "Exctracting lyrics from \"%s\"", media_file);
    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();

    if (! (is_ogg? FLAC__metadata_chain_read_ogg(chain, media_file) : FLAC__metadata_chain_read(chain, media_file)) ) {
        MYMPD_LOG_ERROR(NULL, "Can't read metadata from file \"%s\"", media_file);
        FLAC__metadata_chain_delete(chain);
        return;
    }

    FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
    assert(iterator);
    FLAC__metadata_iterator_init(iterator, chain);
    int found_lyrics = 0;
    sds buffer = sdsempty();
    do {
        FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
        if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
            int field_num = 0;
            while ((field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block, (unsigned)field_num, comment_name)) > -1) {
                FLAC__StreamMetadata_VorbisComment *vc = &block->data.vorbis_comment;
                FLAC__StreamMetadata_VorbisComment_Entry *field = &vc->comments[field_num++];

                char *field_value = memchr(field->entry, '=', field->length);
                if (field_value != NULL &&
                    strlen(field_value) > 1)
                {
                    field_value++;
                    buffer = sdscatlen(buffer, "{", 1);
                    buffer = tojson_bool(buffer, "synced", synced, true);
                    buffer = tojson_char_len(buffer, "lang", "", 0, true);
                    buffer = tojson_char_len(buffer, "desc", "", 0, true);
                    buffer = tojson_char(buffer, "text", field_value, false);
                    buffer = sdscatlen(buffer, "}", 1);
                    list_push(extracted, buffer, 0 , NULL, NULL);
                    sdsclear(buffer);
                    found_lyrics++;
                    MYMPD_LOG_DEBUG(NULL, "Found embedded lyrics");
                    field_value++;
                }
                else {
                    MYMPD_LOG_DEBUG(NULL, "Invalid vorbis comment");
                }
            }
        }
    } while (FLAC__metadata_iterator_next(iterator));

    if (found_lyrics == 0) {
        MYMPD_LOG_DEBUG(NULL, "No embedded lyrics found");
    }
    FLAC__metadata_iterator_delete(iterator);
    FLAC__metadata_chain_delete(chain);
    FREE_SDS(buffer);
    #else
    (void) media_file;
    (void) is_ogg;
    (void) comment_name;
    (void) synced;
    (void) extracted;
    #endif
}
