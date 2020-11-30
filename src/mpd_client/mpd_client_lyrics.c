/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <stdbool.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "mpd_client_utility.h"
#include "mpd_client_lyrics.h"

//optional includes
#ifdef ENABLE_LIBID3TAG
    #include <id3tag.h>
#endif

#ifdef ENABLE_FLAC
    #include <FLAC/metadata.h>
#endif

//privat definitions
static sds handle_lyricsextract(sds buffer, sds method, long request_id, const char *media_file);
static sds handle_lyricsextract_id3(sds buffer, sds method, long request_id, const char *media_file);
static sds handle_lyricsextract_flac(sds buffer, sds method, long request_id, const char *media_file, bool is_ogg);

//public functions

sds mpd_client_handle_lyrics(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri) {
    if (validate_uri(uri) == false) {
        LOG_ERROR("Invalid URI: %s", uri);
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Invalid uri", true);
        return buffer;
    }
    
    //create absolute file
    sds mediafile = sdscatfmt(sdsempty(), "%s/%s", mpd_client_state->music_directory_value, uri);
    LOG_DEBUG("Absolut media_file: %s", mediafile);

    //check music_directory folder
    if (mpd_client_state->feat_library == true && access(mediafile, F_OK) == 0) /* Flawfinder: ignore */
    {
        //try txt file in folder in the music directory
        char *uricpy = strdup(uri);
        strip_extension(uricpy);
        sds lyricsfile = sdscatfmt(sdsempty(), "%s/%s.txt", mpd_client_state->music_directory_value, uricpy);
        FREE_PTR(uricpy);
        FILE *fp = fopen(lyricsfile, "r");
        sdsfree(lyricsfile);
        if (fp != NULL) {
            buffer = jsonrpc_start_result(buffer, method, request_id);
            buffer = sdscat(buffer, ",\"data\":[{");
            buffer = tojson_char(buffer, "lang", "", true);
            buffer = tojson_char(buffer, "desc", "", true);
            char *line = NULL;
            size_t n = 0;
            ssize_t read;
            sds text = sdsempty();
            while ((read = getline(&line, &n, fp)) > 0) {
                text = sdscatlen(text, line, read);
            }
            fclose(fp);
            buffer = tojson_char(buffer, "text", text, false);
            buffer = sdscatlen(buffer, "}],", 3);
            buffer = tojson_long(buffer, "returnedEntities", 1, false);
            buffer = jsonrpc_end_result(buffer);
            sdsfree(mediafile);
            sdsfree(text);
            return buffer;
        }
        LOG_DEBUG("No lyrics file found in music directory");
        
        //try to extract lyrics from media file
        buffer = handle_lyricsextract(buffer, method, request_id, mediafile);
        if (sdslen(buffer) > 0) {
            sdsfree(mediafile);
            return buffer;
        }
    }
    LOG_VERBOSE("No lyrics found for %s", mediafile);
    sdsfree(mediafile);
    buffer = jsonrpc_respond_message(buffer, method, request_id, "No lyrics found", false);
    return buffer;
}

//privat functions
static sds handle_lyricsextract(sds buffer, sds method, long request_id, const char *media_file) {
    sds mime_type_media_file = get_mime_type_by_ext(media_file);
    if (strcmp(mime_type_media_file, "audio/mpeg") == 0) {
        buffer = handle_lyricsextract_id3(buffer, method, request_id, media_file);
    }
    else if (strcmp(mime_type_media_file, "audio/ogg") == 0) {
        buffer = handle_lyricsextract_flac(buffer, method, request_id, media_file, true);
    }
    else if (strcmp(mime_type_media_file, "audio/flac") == 0) {
        buffer = handle_lyricsextract_flac(buffer, method, request_id, media_file, false);
    }
    else {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Unsupported file type", false);
    }
    sdsfree(mime_type_media_file);
    return buffer;
}

static sds handle_lyricsextract_id3(sds buffer, sds method, long request_id, const char *media_file) {
    #ifdef ENABLE_LIBID3TAG
    LOG_DEBUG("Exctracting lyrics from %s", media_file);
    struct id3_file *file_struct = id3_file_open(media_file, ID3_FILE_MODE_READONLY);
    if (file_struct == NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Error reading metadata", true);
        LOG_ERROR("Can't parse id3_file: %s", media_file);
        return buffer;
    }
    struct id3_tag *tags = id3_file_tag(file_struct);
    if (tags == NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Error reading metadata", true);
        LOG_ERROR("Can't read id3 tags from file: %s", media_file);
        return buffer;
    }

    struct id3_frame *frame;
    unsigned i = 0;
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    while ((frame = id3_tag_findframe(tags, "USLT", i)) != NULL) {
        const id3_ucs4_t *uslt_text = id3_field_getfullstring(&frame->fields[3]);
        if (uslt_text != NULL) {
            if (i > 0) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);

            const id3_ucs4_t *uslt_lang = id3_field_getstring(&frame->fields[1]);
            if (uslt_lang != NULL) {
                id3_utf8_t *uslt_lang_utf8 = id3_ucs4_utf8duplicate(uslt_lang);
                buffer = tojson_char(buffer, "lang", (char *)uslt_lang_utf8, true);
                FREE_PTR(uslt_lang_utf8);
            }
            else {
                buffer = tojson_char(buffer, "lang", "", true);
            }
            
            const id3_ucs4_t *uslt_desc = id3_field_getstring(&frame->fields[2]);
            if (uslt_desc != NULL) {
                id3_utf8_t *uslt_desc_utf8 = id3_ucs4_utf8duplicate(uslt_desc);
                buffer = tojson_char(buffer, "desc", (char *)uslt_desc_utf8, true);
                FREE_PTR(uslt_desc_utf8);
            }
            else {
                buffer = tojson_char(buffer, "desc", "", true);
            }
            
            id3_utf8_t *uslt_text_utf8 = id3_ucs4_utf8duplicate(uslt_text);
            buffer = tojson_char(buffer, "text", (char *)uslt_text_utf8, false);
            FREE_PTR(uslt_text_utf8);
            buffer = sdscatlen(buffer, "}", 1);
            LOG_DEBUG("Lyrics successfully extracted");
        }
        else {
            LOG_DEBUG("Can not read embedded lyrics");
            break;
        }
        i++;
    }
    id3_file_close(file_struct);
    
    if (i == 0) {
        LOG_DEBUG("No embedded lyrics detected");
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No lyrics found", false);
        return buffer;
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "returnedEntities", i, false);
    buffer = jsonrpc_end_result(buffer);
    #else
    (void) media_file;
    #endif
    return buffer;
}

static sds handle_lyricsextract_flac(sds buffer, sds method, long request_id, const char *media_file, bool is_ogg) {
    #ifdef ENABLE_FLAC
    LOG_DEBUG("Exctracting lyrics from %s", media_file);
    FLAC__StreamMetadata *metadata = NULL;

    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();
    
    if(! (is_ogg? FLAC__metadata_chain_read_ogg(chain, media_file) : FLAC__metadata_chain_read(chain, media_file)) ) {
        LOG_DEBUG("%s: ERROR: reading metadata", media_file);
        FLAC__metadata_chain_delete(chain);
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Error reading metadata", false);
        return buffer;
    }

    FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
    FLAC__metadata_iterator_init(iterator, chain);
    assert(iterator);
    
    int field_num = 0;
    do {
        FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
        if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
            field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block, 0, "LYRICS");
            if (field_num == -1) {
                field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block, 0, "UNSYNCEDLYRICS");
            }
            if (field_num > -1) {
                metadata = block;
            }
        }
    } while (FLAC__metadata_iterator_next(iterator) && metadata == NULL);
    
    if (metadata == NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No lyrics found", false);
        LOG_DEBUG("No embedded lyrics detected");
    }
    else {
        FLAC__StreamMetadata_VorbisComment *vc = &metadata->data.vorbis_comment;
        FLAC__StreamMetadata_VorbisComment_Entry *field = &vc->comments[field_num++];

        char *field_value = memchr(field->entry, '=', field->length);
        if (field_value != NULL) {
            if (strlen(field_value) > 1) {
                field_value++;
                buffer = jsonrpc_start_result(buffer, method, request_id);
                buffer = sdscat(buffer, ",\"data\":[{");
                buffer = tojson_char(buffer, "lang", "", true);
                buffer = tojson_char(buffer, "desc", "", true);
                buffer = tojson_char(buffer, "text", field_value, false);
                buffer = sdscatlen(buffer, "}],", 3);
                buffer = tojson_long(buffer, "returnedEntities", 1, false);
                buffer = jsonrpc_end_result(buffer);
            }
            else {
                buffer = jsonrpc_respond_message(buffer, method, request_id, "No lyrics found", false);
            }
        }
        else {
            LOG_DEBUG("Invalid vorbis comment");
            buffer = jsonrpc_respond_message(buffer, method, request_id, "Error reading metadata", false);
        }
    }
    FLAC__metadata_iterator_delete(iterator);
    FLAC__metadata_chain_delete(chain);
    #else
    (void) media_file;
    (void) is_ogg;
    #endif
    return buffer;
}
