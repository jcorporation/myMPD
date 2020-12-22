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
static sds _mpd_client_lyrics_unsynced(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri);
static sds _mpd_client_lyrics_synced(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri);
static sds lyrics_fromfile(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, sds mediafile, const char *ext, bool synced);
static sds lyricsextract_unsynced(sds buffer, sds method, long request_id, sds media_file, sds vorbis_comment);
static sds lyricsextract_unsynced_id3(sds buffer, sds method, long request_id, sds media_file);
static const char *_id3_field_getlanguage(union id3_field const *field);
static sds lyricsextract_synced(sds buffer, sds method, long request_id, sds media_file, sds vorbis_comment);
static sds lyricsextract_synced_id3(sds buffer, sds method, long request_id, sds media_file);
static sds lyricsextract_flac(sds buffer, sds method, long request_id, sds media_file, bool is_ogg, const char *comment_name, bool synced);

//public functions
sds mpd_client_lyrics_get(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri) {
    //try first synced lyrics
    buffer = _mpd_client_lyrics_synced(config, mpd_client_state, buffer, method, request_id, uri);
    //if not found try unsynced lyrics
    if (sdslen(buffer) == 0) {
        buffer = _mpd_client_lyrics_unsynced(config, mpd_client_state, buffer, method, request_id, uri);
    }
    //if not found print error message
    if (sdslen(buffer) == 0) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No lyrics found", false);
    }
    return buffer;
}

sds mpd_client_lyrics_unsynced(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri) {
    buffer = _mpd_client_lyrics_unsynced(config, mpd_client_state, buffer, method, request_id, uri);
    if (sdslen(buffer) == 0) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No lyrics found", false);
    }
    return buffer;
}

sds mpd_client_lyrics_synced(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri) {
    buffer = _mpd_client_lyrics_synced(config, mpd_client_state, buffer, method, request_id, uri);
    if (sdslen(buffer) == 0) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No lyrics found", false);
    }
    return buffer;
}

//privat functions
static sds _mpd_client_lyrics_unsynced(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri) {
    //create absolute file
    sds mediafile = sdscatfmt(sdsempty(), "%s/%s", mpd_client_state->music_directory_value, uri);
    LOG_DEBUG("Absolut media_file: %s", mediafile);
    //try .txt file in folder in the music directory
    buffer = lyrics_fromfile(mpd_client_state, buffer, method, request_id, mediafile, config->uslt_ext, false);
    if (sdslen(buffer) == 0) {
        LOG_DEBUG("Getting embedded unsynced lyrics from %s", mediafile);
        buffer = lyricsextract_unsynced(buffer, method, request_id, mediafile, config->vorbis_uslt);
        if (sdslen(buffer) > 0) {
            sdsfree(mediafile);
            return buffer;
        }
    }
    else {
        sdsfree(mediafile);
        return buffer;
    }
    LOG_VERBOSE("No unsynced lyrics found for %s", mediafile);
    sdsfree(mediafile);
    return buffer;
}

static sds _mpd_client_lyrics_synced(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri) {
    //create absolute file
    sds mediafile = sdscatfmt(sdsempty(), "%s/%s", mpd_client_state->music_directory_value, uri);
    LOG_DEBUG("Absolut media_file: %s", mediafile);
    //try .lrc file in folder in the music directory
    buffer = lyrics_fromfile(mpd_client_state, buffer, method, request_id, mediafile, config->sylt_ext, true);
    if (sdslen(buffer) == 0) {
        LOG_DEBUG("Getting embedded synced lyrics from %s", mediafile);
        buffer = lyricsextract_synced(buffer, method, request_id, mediafile, config->vorbis_sylt);
        if (sdslen(buffer) > 0) {
            sdsfree(mediafile);
            return buffer;
        }
    }
    else {
        sdsfree(mediafile);
        return buffer;
    }
    LOG_VERBOSE("No synced lyrics found for %s", mediafile);
    sdsfree(mediafile);
    return buffer;
}

static sds lyrics_fromfile(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, sds mediafile, const char *ext, bool synced) {
    //check music_directory folder
    if (mpd_client_state->feat_library == false || access(mediafile, F_OK) != 0) /* Flawfinder: ignore */
    {
        LOG_DEBUG("No lyrics file found, no access to music directory");
        return buffer;
    }
    //try file in folder in the music directory
    sds filename_cpy = sdsnew(mediafile);
    strip_extension(filename_cpy);
    sds lyricsfile = sdscatfmt(sdsempty(), "%s.%s", filename_cpy, ext);
    LOG_DEBUG("Trying to open lyrics file: %s", lyricsfile);
    sdsfree(filename_cpy);
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
        FREE_PTR(line);
        buffer = tojson_char(buffer, "text", text, false);
        buffer = sdscatlen(buffer, "}],", 3);
        buffer = tojson_bool(buffer, "synced", synced, true);
        buffer = tojson_long(buffer, "returnedEntities", 1, false);
        buffer = jsonrpc_end_result(buffer);
        sdsfree(text);
        return buffer;
    }
    LOG_DEBUG("No lyrics file found in music directory");
    return buffer;
}

static sds lyricsextract_unsynced(sds buffer, sds method, long request_id, sds media_file, sds vorbis_comment) {
    sds mime_type_media_file = get_mime_type_by_ext(media_file);
    if (strcmp(mime_type_media_file, "audio/mpeg") == 0) {
        buffer = lyricsextract_unsynced_id3(buffer, method, request_id, media_file);
    }
    else if (strcmp(mime_type_media_file, "audio/ogg") == 0) {
        buffer = lyricsextract_flac(buffer, method, request_id, media_file, true, vorbis_comment, false);
    }
    else if (strcmp(mime_type_media_file, "audio/flac") == 0) {
        buffer = lyricsextract_flac(buffer, method, request_id, media_file, false, vorbis_comment, false);
    }
    else {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Unsupported file type", false);
    }
    sdsfree(mime_type_media_file);
    return buffer;
}

static sds lyricsextract_synced(sds buffer, sds method, long request_id, sds media_file, sds vorbis_comment) {
    sds mime_type_media_file = get_mime_type_by_ext(media_file);
    if (strcmp(mime_type_media_file, "audio/mpeg") == 0) {
        buffer = lyricsextract_synced_id3(buffer, method, request_id, media_file);
    }
    else if (strcmp(mime_type_media_file, "audio/ogg") == 0) {
        buffer = lyricsextract_flac(buffer, method, request_id, media_file, true, vorbis_comment, true);
    }
    else if (strcmp(mime_type_media_file, "audio/flac") == 0) {
        buffer = lyricsextract_flac(buffer, method, request_id, media_file, false, vorbis_comment, true);
    }
    else {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Unsupported file type", false);
    }
    sdsfree(mime_type_media_file);
    return buffer;
}

static sds lyricsextract_unsynced_id3(sds buffer, sds method, long request_id, sds media_file) {
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
        //fields of USLT:
        //ID3_FIELD_TYPE_TEXTENCODING -> can be ignored
        //ID3_FIELD_TYPE_LANGUAGE -> lang (3 chars)
        //ID3_FIELD_TYPE_STRING -> desc
        //ID3_FIELD_TYPE_STRINGFULL -> lyrics
        const id3_ucs4_t *uslt_text = id3_field_getfullstring(&frame->fields[3]);
        if (uslt_text != NULL) {
            if (i > 0) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);

            //libid3tag has not get function for language, use own function
            const char *lang = _id3_field_getlanguage(&frame->fields[1]);
            if (lang != NULL) {
                buffer = tojson_char(buffer, "lang", lang, true);
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
        buffer = tojson_bool(buffer, "synced", false, true);
    buffer = tojson_long(buffer, "returnedEntities", i, false);
    buffer = jsonrpc_end_result(buffer);
    #else
    (void) media_file;
    #endif
    return buffer;
}

static sds lyricsextract_synced_id3(sds buffer, sds method, long request_id, sds media_file) {
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
    while ((frame = id3_tag_findframe(tags, "SYLT", i)) != NULL) {
        //fields of SYLT:
        //ID3_FIELD_TYPE_TEXTENCODING -> can be ignored
        //ID3_FIELD_TYPE_LANGUAGE -> lang (3 chars)
        //ID3_FIELD_TYPE_INT8 -> time stamp
        //ID3_FIELD_TYPE_INT8 -> content type
        //ID3_FIELD_TYPE_STRING -> desc
        //ID3_FIELD_TYPE_BINARYDATA -> lyrics
        id3_length_t length;
        const id3_byte_t *text = id3_field_getbinarydata(id3_frame_field(frame, 5), &length);
        if (text != NULL) {
            if (i > 0) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            
            enum id3_field_textencoding encoding = id3_field_gettextencoding(&frame->fields[0]);
            buffer = tojson_long(buffer, "encoding", encoding, true);

            //libid3tag has not get function for language, use own function
            const char *lang = _id3_field_getlanguage(&frame->fields[1]);
            if (lang != NULL) {
                buffer = tojson_char(buffer, "lang", lang, true);
            }
            else {
                buffer = tojson_char(buffer, "lang", "", true);
            }
            
            long time_stamp = id3_field_getint(&frame->fields[2]);
            buffer = tojson_long(buffer, "timestamp", time_stamp, true);
            
            long content_type = id3_field_getint(&frame->fields[3]);
            buffer = tojson_long(buffer, "contenttype", content_type, true);
            
            const id3_ucs4_t *uslt_desc = id3_field_getstring(&frame->fields[4]);
            if (uslt_desc != NULL) {
                id3_utf8_t *uslt_desc_utf8 = id3_ucs4_utf8duplicate(uslt_desc);
                buffer = tojson_char(buffer, "desc", (char *)uslt_desc_utf8, true);
                FREE_PTR(uslt_desc_utf8);
            }
            else {
                buffer = tojson_char(buffer, "desc", "", true);
            }
            buffer = tojson_char_len(buffer, "text", (char *)text, length, false);
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
        buffer = tojson_bool(buffer, "synced", true, true);
    buffer = tojson_long(buffer, "returnedEntities", i, false);
    buffer = jsonrpc_end_result(buffer);
    #else
    (void) media_file;
    #endif
    return buffer;
}

static const char *_id3_field_getlanguage(union id3_field const *field) {
    assert(field);

    if (field->type != ID3_FIELD_TYPE_LANGUAGE) {
        return NULL;
    }

    return field->immediate.value;
}

static sds lyricsextract_flac(sds buffer, sds method, long request_id, sds media_file, bool is_ogg, const char *comment_name, bool synced) {
    #ifdef ENABLE_FLAC
    LOG_DEBUG("Exctracting lyrics from %s", media_file);
    FLAC__StreamMetadata *metadata = NULL;

    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();
    
    if(! (is_ogg? FLAC__metadata_chain_read_ogg(chain, media_file) : FLAC__metadata_chain_read(chain, media_file)) ) {
        LOG_DEBUG("%s: ERROR: reading metadata", media_file);
        FLAC__metadata_chain_delete(chain);
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Error reading metadata", true);
        return buffer;
    }

    FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
    assert(iterator);
    FLAC__metadata_iterator_init(iterator, chain);
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    int field_num = 0;
    int found_lyrics = 0;
    FLAC__StreamMetadata *block;
    FLAC__bool ok = true;
    do {
        block = FLAC__metadata_iterator_get_block(iterator);
        ok &= (0 != block);
        if (!ok) {
            LOG_ERROR("Could not get block from chain: %s", media_file);
        }
        else if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
            field_num = 0;
            while ((field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block, field_num, comment_name)) > -1) {
                LOG_DEBUG("Found embedded lyrics");
                metadata = block;
                FLAC__StreamMetadata_VorbisComment *vc = &metadata->data.vorbis_comment;
                FLAC__StreamMetadata_VorbisComment_Entry *field = &vc->comments[field_num++];

                char *field_value = memchr(field->entry, '=', field->length);
                if (field_value != NULL && strlen(field_value) > 1) {
                    field_value++;
                    if (found_lyrics++) {
                        buffer = sdscatlen(buffer, ",", 1);
                    }
                    buffer = sdscatlen(buffer, "{", 1);
                    buffer = tojson_char(buffer, "lang", "", true);
                    buffer = tojson_char(buffer, "desc", "", true);
                    buffer = tojson_char(buffer, "text", field_value, false);
                    buffer = sdscatlen(buffer, "}", 1);
                }
                else {
                    LOG_DEBUG("Invalid vorbis comment");
                }
            }
        }
    } while (ok && FLAC__metadata_iterator_next(iterator));
    
    if (found_lyrics == 0) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No lyrics found", false);
    }
    else {
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_bool(buffer, "synced", synced, true);
        buffer = tojson_long(buffer, "returnedEntities", found_lyrics, false);
        buffer = jsonrpc_end_result(buffer);
    }
    FLAC__metadata_iterator_delete(iterator);
    FLAC__metadata_chain_delete(chain);
    #else
    (void) media_file;
    (void) is_ogg;
    #endif
    return buffer;
}
