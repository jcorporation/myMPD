/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
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
static sds lyrics_fromfile(sds buffer, sds method, long request_id, sds mediafile, const char *ext, bool synced);
static sds lyricsextract_unsynced(sds buffer, sds method, long request_id, sds media_file, sds vorbis_comment);
static sds lyricsextract_unsynced_id3(sds buffer, sds method, long request_id, sds media_file);
static sds lyricsextract_synced(sds buffer, sds method, long request_id, sds media_file, sds vorbis_comment);
static sds lyricsextract_synced_id3(sds buffer, sds method, long request_id, sds media_file);
static sds lyricsextract_flac(sds buffer, sds method, long request_id, sds media_file, bool is_ogg, const char *comment_name, bool synced);

#ifdef ENABLE_LIBID3TAG
static sds decode_sylt(const id3_byte_t *binary_data, id3_length_t binary_length);
static const char *_id3_field_getlanguage(union id3_field const *field);
#endif

//public functions
sds mpd_client_lyrics_get(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri) {
    if (is_streamuri(uri) == true) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "lyrics", "error", "Can not get lyrics for stream uri");
        return buffer;
    }
    if (mpd_client_state->feat_library == false) {
        MYMPD_LOG_DEBUG("No lyrics file found, no access to music directory");
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "lyrics", "info", "No lyrics found");
        return buffer;
    }
    //try first synced lyrics
    MYMPD_LOG_DEBUG("Get synced lyrics for uri: %s", uri);
    buffer = _mpd_client_lyrics_synced(config, mpd_client_state, buffer, method, request_id, uri);
    //if not found try unsynced lyrics
    if (sdslen(buffer) == 0) {
        MYMPD_LOG_DEBUG("Get unsynced lyrics for uri: %s", uri);
        buffer = _mpd_client_lyrics_unsynced(config, mpd_client_state, buffer, method, request_id, uri);
    }
    //if not found print error message
    if (sdslen(buffer) == 0) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "lyrics", "info", "No lyrics found");
    }
    return buffer;
}

sds mpd_client_lyrics_unsynced(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri) {
    if (is_streamuri(uri) == true) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "lyrics", "error", "Can not get lyrics for stream uri");
        return buffer;
    }
    if (mpd_client_state->feat_library == false) {
        MYMPD_LOG_DEBUG("No lyrics file found, no access to music directory");
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "lyrics", "info", "No lyrics found");
        return buffer;
    }
    buffer = _mpd_client_lyrics_unsynced(config, mpd_client_state, buffer, method, request_id, uri);
    if (sdslen(buffer) == 0) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "lyrics", "info", "No lyrics found");
    }
    return buffer;
}

sds mpd_client_lyrics_synced(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri) {
    if (is_streamuri(uri) == true) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "lyrics", "error", "Can not get lyrics for stream uri");
        return buffer;
    }
    if (mpd_client_state->feat_library == false) {
        MYMPD_LOG_DEBUG("No lyrics file found, no access to music directory");
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "lyrics", "info", "No lyrics found");
        return buffer;
    }
    buffer = _mpd_client_lyrics_synced(config, mpd_client_state, buffer, method, request_id, uri);
    if (sdslen(buffer) == 0) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "lyrics", "info", "No lyrics found");
    }
    return buffer;
}

//private functions
static sds _mpd_client_lyrics_unsynced(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri) {
    //create absolute file
    sds mediafile = sdscatfmt(sdsempty(), "%s/%s", mpd_client_state->music_directory_value, uri);
    MYMPD_LOG_DEBUG("Absolut media_file: %s", mediafile);
    //try .txt file in folder in the music directory
    buffer = lyrics_fromfile(buffer, method, request_id, mediafile, config->uslt_ext, false);
    if (sdslen(buffer) == 0) {
        MYMPD_LOG_DEBUG("Getting embedded unsynced lyrics from %s", mediafile);
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
    MYMPD_LOG_INFO("No unsynced lyrics found for %s", mediafile);
    sdsfree(mediafile);
    return buffer;
}

static sds _mpd_client_lyrics_synced(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, const char *uri) {
    //create absolute file
    sds mediafile = sdscatfmt(sdsempty(), "%s/%s", mpd_client_state->music_directory_value, uri);
    MYMPD_LOG_DEBUG("Absolut media_file: %s", mediafile);
    //try .lrc file in folder in the music directory
    buffer = lyrics_fromfile(buffer, method, request_id, mediafile, config->sylt_ext, true);
    if (sdslen(buffer) == 0) {
        MYMPD_LOG_DEBUG("Getting embedded synced lyrics from %s", mediafile);
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
    MYMPD_LOG_INFO("No synced lyrics found for %s", mediafile);
    sdsfree(mediafile);
    return buffer;
}

static sds lyrics_fromfile(sds buffer, sds method, long request_id, sds mediafile, const char *ext, bool synced) {
    //try file in folder in the music directory
    sds filename_cpy = sdsnew(mediafile);
    strip_extension(filename_cpy);
    sds lyricsfile = sdscatfmt(sdsempty(), "%s.%s", filename_cpy, ext);
    MYMPD_LOG_DEBUG("Trying to open lyrics file: %s", lyricsfile);
    sdsfree(filename_cpy);
    FILE *fp = fopen(lyricsfile, "r");
    sdsfree(lyricsfile);
    if (fp != NULL) {
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = sdscat(buffer, "\"data\":[{");
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
        buffer = jsonrpc_result_end(buffer);
        sdsfree(text);
        return buffer;
    }
    MYMPD_LOG_DEBUG("No lyrics file found in music directory");
    sdsclear(buffer);
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
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "lyrics", "warn", "Unsupported file type");
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
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "lyrics", "warn", "Unsupported file type");
    }
    sdsfree(mime_type_media_file);
    return buffer;
}

static sds lyricsextract_unsynced_id3(sds buffer, sds method, long request_id, sds media_file) {
    #ifdef ENABLE_LIBID3TAG
    MYMPD_LOG_DEBUG("Exctracting lyrics from %s", media_file);
    struct id3_file *file_struct = id3_file_open(media_file, ID3_FILE_MODE_READONLY);
    if (file_struct == NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "lyrics", "error", "Error reading metadata");
        MYMPD_LOG_ERROR("Can't parse id3_file: %s", media_file);
        return buffer;
    }
    struct id3_tag *tags = id3_file_tag(file_struct);
    if (tags == NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "lyrics", "error", "Error reading metadata");
        MYMPD_LOG_ERROR("Can't read id3 tags from file: %s", media_file);
        return buffer;
    }

    struct id3_frame *frame;
    unsigned i = 0;
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
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
            MYMPD_LOG_DEBUG("Lyrics successfully extracted");
        }
        else {
            MYMPD_LOG_DEBUG("Can not read embedded lyrics");
            break;
        }
        i++;
    }
    id3_file_close(file_struct);
    
    if (i == 0) {
        MYMPD_LOG_DEBUG("No embedded lyrics detected");
        sdsclear(buffer);
        return buffer;
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_bool(buffer, "synced", false, true);
    buffer = tojson_long(buffer, "returnedEntities", i, false);
    buffer = jsonrpc_result_end(buffer);
    #else
    (void) media_file;
    (void) method;
    (void) request_id;
    #endif
    return buffer;
}

static sds lyricsextract_synced_id3(sds buffer, sds method, long request_id, sds media_file) {
    #ifdef ENABLE_LIBID3TAG
    MYMPD_LOG_DEBUG("Exctracting lyrics from %s", media_file);
    struct id3_file *file_struct = id3_file_open(media_file, ID3_FILE_MODE_READONLY);
    if (file_struct == NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "lyrics", "error", "Error reading metadata");
        MYMPD_LOG_ERROR("Can't parse id3_file: %s", media_file);
        return buffer;
    }
    struct id3_tag *tags = id3_file_tag(file_struct);
    if (tags == NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "lyrics", "error", "Error reading metadata");
        MYMPD_LOG_ERROR("Can't read id3 tags from file: %s", media_file);
        return buffer;
    }

    struct id3_frame *frame;
    unsigned i = 0;
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    while ((frame = id3_tag_findframe(tags, "SYLT", i)) != NULL) {
        //fields of SYLT:
        //ID3_FIELD_TYPE_TEXTENCODING -> can be ignored
        //ID3_FIELD_TYPE_LANGUAGE -> lang (3 chars)
        //ID3_FIELD_TYPE_INT8 -> time stamp
        //ID3_FIELD_TYPE_INT8 -> content type
        //ID3_FIELD_TYPE_STRING -> desc
        //ID3_FIELD_TYPE_BINARYDATA -> lyrics
        id3_length_t sylt_data_len;
        const id3_byte_t *sylt_data = id3_field_getbinarydata(id3_frame_field(frame, 5), &sylt_data_len);
        if (sylt_data != NULL) {
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
            sds text = decode_sylt(sylt_data, sylt_data_len);
            buffer = tojson_char(buffer, "text", text, false);
            buffer = sdscatlen(buffer, "}", 1);
            MYMPD_LOG_DEBUG("Lyrics successfully extracted");
        }
        else {
            MYMPD_LOG_DEBUG("Can not read embedded lyrics");
            break;
        }
        i++;
    }
    id3_file_close(file_struct);
    
    if (i == 0) {
        MYMPD_LOG_DEBUG("No embedded lyrics detected");
        sdsclear(buffer);
        return buffer;
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_bool(buffer, "synced", true, true);
    buffer = tojson_long(buffer, "returnedEntities", i, false);
    buffer = jsonrpc_result_end(buffer);
    #else
    (void) media_file;
    (void) method;
    (void) request_id;
    #endif
    return buffer;
}

#ifdef ENABLE_LIBID3TAG
static const char *_id3_field_getlanguage(union id3_field const *field) {
    assert(field);

    if (field->type != ID3_FIELD_TYPE_LANGUAGE) {
        return NULL;
    }

    return field->immediate.value;
}

static sds decode_sylt(const id3_byte_t *binary_data, id3_length_t binary_length) {
    sds sylt_text = sdsempty();
    int sep = 0;
    int text = 1;
    int ts = 0;
    unsigned char ts_buf[1] = "\0";
    sds text_buf = sdsempty();
    for (unsigned i = 0; i < binary_length; i++) {
        if (ts == 1) {
            //timestamp has 2 bytes, save first one
            ts_buf[0] = binary_data[i];
            ts = 2;
        }
        else if (ts == 2) {
           int cc = 0;
           if (binary_data[i] != '\0' && ts_buf[0] != '\0') {
               cc = (ts_buf[0] << 8) | binary_data[i]; 
           }
           //convert hundredths of a seconds to lrc time format
           int mm = cc / 60000;
           cc = cc - mm * 60000;
           int ss = cc / 1000;
           cc = cc - ss * 1000;
           sylt_text = sdscatprintf(sylt_text, "[%02d:%02d.%02d]", mm, ss, cc);
           ts = 3;
        }
        if (binary_data[i] == '\n' || ts == 3) {
            //print text and empty text buffer
            if (sdslen(text_buf) > 0) {
                sylt_text = sdscatfmt(sylt_text, "%s\n", text_buf);
                sdsclear(text_buf);
            }
            //reset all states on line ending
            sep = 0;
            text = 1;
            ts = 0;
        }
        else if (binary_data[i] == '\0') {
            //count separators
            sep++;
            //end of text
            text = 0;
            //start of timestamps
            if (sep == 3) {
                ts = 1;
            }
        }
        else if (text == 1) {
            //add char to text buffer
            text_buf = sdscatprintf(text_buf, "%c", binary_data[i]);
        }
    }

    return sylt_text;
}
#endif

static sds lyricsextract_flac(sds buffer, sds method, long request_id, sds media_file, bool is_ogg, const char *comment_name, bool synced) {
    #ifdef ENABLE_FLAC
    MYMPD_LOG_DEBUG("Exctracting lyrics from %s", media_file);
    FLAC__StreamMetadata *metadata = NULL;

    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();
    
    if(! (is_ogg? FLAC__metadata_chain_read_ogg(chain, media_file) : FLAC__metadata_chain_read(chain, media_file)) ) {
        MYMPD_LOG_DEBUG("%s: ERROR: reading metadata", media_file);
        FLAC__metadata_chain_delete(chain);
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "lyrics", "error", "Error reading metadata");
        return buffer;
    }

    FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
    assert(iterator);
    FLAC__metadata_iterator_init(iterator, chain);
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int field_num = 0;
    int found_lyrics = 0;
    FLAC__StreamMetadata *block;
    FLAC__bool ok = true;
    do {
        block = FLAC__metadata_iterator_get_block(iterator);
        ok &= (0 != block);
        if (!ok) {
            MYMPD_LOG_ERROR("Could not get block from chain: %s", media_file);
        }
        else if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
            field_num = 0;
            while ((field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block, field_num, comment_name)) > -1) {
                MYMPD_LOG_DEBUG("Found embedded lyrics");
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
                    MYMPD_LOG_DEBUG("Invalid vorbis comment");
                }
            }
        }
    } while (ok && FLAC__metadata_iterator_next(iterator));
    
    if (found_lyrics == 0) {
        MYMPD_LOG_DEBUG("No embedded lyrics detected");
        sdsclear(buffer);
    }
    else {
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_bool(buffer, "synced", synced, true);
        buffer = tojson_long(buffer, "returnedEntities", found_lyrics, false);
        buffer = jsonrpc_result_end(buffer);
    }
    FLAC__metadata_iterator_delete(iterator);
    FLAC__metadata_chain_delete(chain);
    #else
    (void) media_file;
    (void) is_ogg;
    (void) method;
    (void) request_id;
    (void) comment_name;
    (void) synced;
    #endif
    return buffer;
}
