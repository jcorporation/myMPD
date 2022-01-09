/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_lyrics.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mimetype.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

//optional includes
#ifdef ENABLE_LIBID3TAG
    #include <id3tag.h>
#endif

#ifdef ENABLE_FLAC
    #include <FLAC/metadata.h>
#endif

//privat definitions
static int _mympd_api_lyrics_unsynced(struct t_mympd_state *mympd_state, sds *buffer, int returned_entities, sds mediafile, const char *mime_type_mediafile);
static int _mympd_api_lyrics_synced(struct t_mympd_state *mympd_state, sds *buffer, int returned_entities, sds mediafile, const char *mime_type_mediafile);
static int lyrics_fromfile(sds *buffer, sds mediafile, const char *ext, bool synced, int returned_entities);
static int lyricsextract_unsynced_id3(sds *buffer, sds media_file, int returned_entities);
static int lyricsextract_synced_id3(sds *buffer, sds media_file, int returned_entities);
static int lyricsextract_flac(sds *buffer, sds media_file, bool is_ogg, const char *comment_name, bool synced, int returned_entities);

#ifdef ENABLE_LIBID3TAG
static sds decode_sylt(const id3_byte_t *binary_data, id3_length_t binary_length, enum id3_field_textencoding encoding);
static const char *_id3_field_getlanguage(union id3_field const *field);
#endif

//public functions
sds mympd_api_lyrics_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, sds uri) {
    if (is_streamuri(uri) == true) {
        MYMPD_LOG_ERROR("Can not get lyrics for stream uri");
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "lyrics", "error", "Can not get lyrics for stream uri");
        return buffer;
    }
    if (mympd_state->mpd_state->feat_mpd_library == false) {
        MYMPD_LOG_DEBUG("Can not get lyrics, no access to music directory");
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "lyrics", "info", "No lyrics found");
        return buffer;
    }
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    sds mediafile = sdscatfmt(sdsempty(), "%s/%s", mympd_state->music_directory_value, uri);
    const char *mime_type_mediafile = get_mime_type_by_ext(mediafile);
    int returned_entities = _mympd_api_lyrics_synced(mympd_state, &buffer, 0, mediafile, mime_type_mediafile);
    returned_entities = _mympd_api_lyrics_unsynced(mympd_state, &buffer, returned_entities, mediafile, mime_type_mediafile);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "returnedEntities", returned_entities, false);
    buffer = jsonrpc_result_end(buffer);
    FREE_SDS(mediafile);
    return buffer;
}

//private functions
static int _mympd_api_lyrics_unsynced(struct t_mympd_state *mympd_state, sds *buffer, int returned_entities, sds mediafile, const char *mime_type_mediafile) {
    //try .txt file in folder in the music directory
    returned_entities = lyrics_fromfile(buffer, mediafile, mympd_state->lyrics_uslt_ext, false, returned_entities);
    //get embedded lyrics
    if (strcmp(mime_type_mediafile, "audio/mpeg") == 0) {
        returned_entities = lyricsextract_unsynced_id3(buffer, mediafile, returned_entities);
    }
    else if (strcmp(mime_type_mediafile, "audio/ogg") == 0) {
        returned_entities = lyricsextract_flac(buffer, mediafile, true, mympd_state->lyrics_vorbis_uslt, false, returned_entities);
    }
    else if (strcmp(mime_type_mediafile, "audio/flac") == 0) {
        returned_entities = lyricsextract_flac(buffer, mediafile, false, mympd_state->lyrics_vorbis_uslt, false, returned_entities);
    }
    return returned_entities;
}

static int _mympd_api_lyrics_synced(struct t_mympd_state *mympd_state, sds *buffer, int returned_entities, sds mediafile, const char *mime_type_mediafile) {
    //try .lrc file in folder in the music directory
    returned_entities = lyrics_fromfile(buffer, mediafile, mympd_state->lyrics_sylt_ext, true, returned_entities);
    //get embedded lyrics
    if (strcmp(mime_type_mediafile, "audio/mpeg") == 0) {
        returned_entities = lyricsextract_synced_id3(buffer, mediafile, returned_entities);
    }
    else if (strcmp(mime_type_mediafile, "audio/ogg") == 0) {
        returned_entities = lyricsextract_flac(buffer, mediafile, true, mympd_state->lyrics_vorbis_sylt, true, returned_entities);
    }
    else if (strcmp(mime_type_mediafile, "audio/flac") == 0) {
        returned_entities = lyricsextract_flac(buffer, mediafile, false, mympd_state->lyrics_vorbis_sylt, true, returned_entities);
    }
    return returned_entities;
}

static int lyrics_fromfile(sds *buffer, sds mediafile, const char *ext, bool synced, int returned_entities) {
    //try file in folder in the music directory
    sds filename_cpy = sdsdup(mediafile);
    sds_strip_file_extension(filename_cpy);
    sds lyricsfile = sdscatfmt(sdsempty(), "%s.%s", filename_cpy, ext);
    MYMPD_LOG_DEBUG("Trying to open lyrics file: %s", lyricsfile);
    FREE_SDS(filename_cpy);
    errno = 0;
    FILE *fp = fopen(lyricsfile, OPEN_FLAGS_READ);
    if (fp != NULL) {
        if (returned_entities > 0) {
            *buffer = sdscatlen(*buffer, ",", 1);
        }
        *buffer = sdscatlen(*buffer, "{", 1);
        *buffer = tojson_bool(*buffer, "synced", synced, true);
        *buffer = tojson_char(*buffer, "lang", "", true);
        *buffer = tojson_char(*buffer, "desc", "", true);
        sds text = sdsempty();
        sds_getfile(&text, fp, 10000);
        fclose(fp);
        *buffer = tojson_char(*buffer, "text", text, false);
        *buffer = sdscatlen(*buffer, "}", 1);
        FREE_SDS(text);
        returned_entities++;
    }
    else {
        if (errno == ENOENT) {
            MYMPD_LOG_DEBUG("No lyrics file found in music directory");
        }
        else {
            MYMPD_LOG_ERROR("Error opening lyrics file \"%s\"", lyricsfile);
            MYMPD_LOG_ERRNO(errno);
        }
    }
    FREE_SDS(lyricsfile);
    return returned_entities;
}

static int lyricsextract_unsynced_id3(sds *buffer, sds media_file, int returned_entities) {
    #ifdef ENABLE_LIBID3TAG
    MYMPD_LOG_DEBUG("Exctracting unsynced lyrics from %s", media_file);
    struct id3_file *file_struct = id3_file_open(media_file, ID3_FILE_MODE_READONLY);
    if (file_struct == NULL) {
        MYMPD_LOG_ERROR("Can't parse id3_file: %s", media_file);
        return returned_entities;
    }
    struct id3_tag *tags = id3_file_tag(file_struct);
    if (tags == NULL) {
        MYMPD_LOG_ERROR("Can't read id3 tags from file: %s", media_file);
        id3_file_close(file_struct);
        return returned_entities;
    }

    int i = 0;
    struct id3_frame *frame;
    while ((frame = id3_tag_findframe(tags, "USLT", i)) != NULL) {
        //fields of USLT:
        //ID3_FIELD_TYPE_TEXTENCODING -> can be ignored
        //ID3_FIELD_TYPE_LANGUAGE -> lang (3 chars)
        //ID3_FIELD_TYPE_STRING -> desc
        //ID3_FIELD_TYPE_STRINGFULL -> lyrics
        const id3_ucs4_t *uslt_text = id3_field_getfullstring(&frame->fields[3]);
        if (uslt_text != NULL) {
            if (returned_entities > 0) {
                *buffer = sdscatlen(*buffer, ",", 1);
            }
            *buffer = sdscat(*buffer, "{\"synced\":false,");
            //libid3tag has not get function for language, use own function
            const char *lang = _id3_field_getlanguage(&frame->fields[1]);
            if (lang != NULL) {
                *buffer = tojson_char(*buffer, "lang", lang, true);
            }
            else {
                *buffer = tojson_char(*buffer, "lang", "", true);
            }

            const id3_ucs4_t *uslt_desc = id3_field_getstring(&frame->fields[2]);
            if (uslt_desc != NULL) {
                id3_utf8_t *uslt_desc_utf8 = id3_ucs4_utf8duplicate(uslt_desc);
                *buffer = tojson_char(*buffer, "desc", (char *)uslt_desc_utf8, true);
                FREE_PTR(uslt_desc_utf8);
            }
            else {
                *buffer = tojson_char(*buffer, "desc", "", true);
            }

            id3_utf8_t *uslt_text_utf8 = id3_ucs4_utf8duplicate(uslt_text);
            *buffer = tojson_char(*buffer, "text", (char *)uslt_text_utf8, false);
            FREE_PTR(uslt_text_utf8);
            *buffer = sdscatlen(*buffer, "}", 1);
            returned_entities++;
            MYMPD_LOG_DEBUG("Unsynced lyrics successfully extracted");
        }
        else {
            MYMPD_LOG_DEBUG("Can not read embedded unsynced lyrics");
            break;
        }
        i++;
    }
    id3_file_close(file_struct);

    if (i == 0) {
        MYMPD_LOG_DEBUG("No embedded unsynced lyrics detected");
    }
    #else
    (void) media_file;
    (void) buffer;
    #endif
    return returned_entities;
}

static int lyricsextract_synced_id3(sds *buffer, sds media_file, int returned_entities) {
    #ifdef ENABLE_LIBID3TAG
    MYMPD_LOG_DEBUG("Exctracting synced lyrics from %s", media_file);
    struct id3_file *file_struct = id3_file_open(media_file, ID3_FILE_MODE_READONLY);
    if (file_struct == NULL) {
        MYMPD_LOG_ERROR("Can't parse id3_file: %s", media_file);
        return 0;
    }
    struct id3_tag *tags = id3_file_tag(file_struct);
    if (tags == NULL) {
        MYMPD_LOG_ERROR("Can't read id3 tags from file: %s", media_file);
        id3_file_close(file_struct);
        return 0;
    }

    int i = 0;
    struct id3_frame *frame;
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
            if (returned_entities > 0) {
                *buffer = sdscatlen(*buffer, ",", 1);
            }
            *buffer = sdscat(*buffer, "{\"synced\":true,");

            enum id3_field_textencoding encoding = id3_field_gettextencoding(&frame->fields[0]);
            *buffer = tojson_long(*buffer, "encoding", encoding, true);

            //libid3tag has not get function for language, use own function
            const char *lang = _id3_field_getlanguage(&frame->fields[1]);
            if (lang != NULL) {
                *buffer = tojson_char(*buffer, "lang", lang, true);
            }
            else {
                *buffer = tojson_char(*buffer, "lang", "", true);
            }

            long time_stamp = id3_field_getint(&frame->fields[2]);
            *buffer = tojson_long(*buffer, "timestamp", time_stamp, true);

            long content_type = id3_field_getint(&frame->fields[3]);
            *buffer = tojson_long(*buffer, "contenttype", content_type, true);

            const id3_ucs4_t *uslt_desc = id3_field_getstring(&frame->fields[4]);
            if (uslt_desc != NULL) {
                id3_utf8_t *uslt_desc_utf8 = id3_ucs4_utf8duplicate(uslt_desc);
                *buffer = tojson_char(*buffer, "desc", (char *)uslt_desc_utf8, true);
                FREE_PTR(uslt_desc_utf8);
            }
            else {
                *buffer = tojson_char(*buffer, "desc", "", true);
            }
            sds text = decode_sylt(sylt_data, sylt_data_len, encoding);
            //sylt data is already encoded
            *buffer = sdscatfmt(*buffer, "\"text\":\"%s\"", text);

            //*buffer = tojson_char(*buffer, "text", text, false);
            FREE_SDS(text);
            *buffer = sdscatlen(*buffer, "}", 1);
            returned_entities++;
            MYMPD_LOG_DEBUG("Synced lyrics successfully extracted");
        }
        else {
            MYMPD_LOG_DEBUG("Can not read embedded synced lyrics");
            break;
        }
        i++;
    }
    id3_file_close(file_struct);
    if (i == 0) {
        MYMPD_LOG_DEBUG("No embedded synced lyrics detected");
    }
    #else
    (void) media_file;
    (void) buffer;
    #endif
    return returned_entities;
}

#ifdef ENABLE_LIBID3TAG
static const char *_id3_field_getlanguage(union id3_field const *field) {
    assert(field);

    if (field->type != ID3_FIELD_TYPE_LANGUAGE) {
        return NULL;
    }

    return field->immediate.value;
}

static sds decode_sylt(const id3_byte_t *binary_data, id3_length_t binary_length, enum id3_field_textencoding encoding) {
    sds sylt_text = sdsempty();
    //text buffer
    sds text_buf = sdsempty();
    unsigned sep_len = encoding == 0 || encoding == 3 ? 1 : 2;
    unsigned i = 0;

    MYMPD_LOG_DEBUG("Sylt encoding: %u", encoding);

    while (i + sep_len + 4 < binary_length) {
        //look for bom and skip it
        if ((encoding == 1 && binary_data[i] == 0xff && binary_data[i + 1] == 0xfe) ||
            (encoding == 2 && binary_data[i] == 0xfe && binary_data[i + 1] == 0xff)) {
            //utf-16 le or be
            i = i + 2;
        }
        else if (encoding == 3 && binary_data[i] == 0xef && binary_data[i + 1] == 0xbb && binary_data[i + 2] == 0xbf) {
            //utf-8
            i = i + 3;
        }
        //skip newline char
        if ((encoding == 0 || encoding == 3) && binary_data[i] == '\n') {
            i++;
        }
        else if ((encoding == 1 && binary_data[i] == '\n' && binary_data[i + 1] == '\0') ||
                 (encoding == 2 && binary_data[i] == '\0' && binary_data[i + 1] == '\n'))
        {
            i = i + 2;
        }
        //read text
        if (encoding == 0) {
            //latin - read text until \0 separator
            while (i < binary_length && binary_data[i] != '\0') {
                text_buf = sds_catjsonchar(text_buf, (char)binary_data[i]);
                i++;
            }
        }
        else if (encoding == 1) {
            //utf16le - read text until \0\0 separator
            while (i + 2 < binary_length && (binary_data[i] != '\0' || binary_data[i + 1] != '\0')) {
                if ((binary_data[i] & 0x80) == 0x00 && binary_data[i + 1] == '\0') {
                    //printable ascii char
                    text_buf = sds_catjsonchar(text_buf, (char)binary_data[i]);
                }
                else {
                    unsigned c = (binary_data[i + 1] << 8) | binary_data[i];
                    if (c <= 0xd7ff || c >= 0xe000) {
                        text_buf = sdscatprintf(text_buf, "\\u%04x", c);
                    }
                    else {
                        //surrogate pair
                        c = (binary_data[i + 1] << 24) | (binary_data[i] << 16) | (binary_data[i + 3] << 8) | binary_data[i + 2];
                        c = c - 0x10000;
                        if (c <= 0x10ffff) {
                            text_buf = sdscatprintf(text_buf, "\\u%04x%04x", 0xd800 + (c >> 10), 0xdc00 + (c & 0x3ff));
                        }
                    }
                }
                i = i + 2;
            }
        }
        else if (encoding == 2) {
            //utf16be - read text until \0\0 separator
            while (i + 2 < binary_length && (binary_data[i] != '\0' || binary_data[i + 1] != '\0')) {
                if ((binary_data[i + 1] & 0x80) == 0x00 && binary_data[i] == '\0') {
                    //printable ascii char
                    text_buf = sds_catjsonchar(text_buf, (char)binary_data[i + 1]);
                }
                else {
                    unsigned c = (binary_data[i] << 8) | binary_data[i + 1];
                    if (c <= 0xd7ff || c >= 0xe000) {
                        text_buf = sdscatprintf(text_buf, "\\u%04x", c);
                    }
                    else if (i + 4 < binary_length) {
                        //surrogate pair
                        c = (binary_data[i] << 24) | (binary_data[i + 1] << 16) | (binary_data[i + 2] << 8) | binary_data[i + 3];
                        c = c - 0x10000;
                        if (c <= 0x10ffff) {
                            text_buf = sdscatprintf(text_buf, "\\u%04x%04x", 0xd800 + (c >> 10), 0xdc00 + (c & 0x3ff));
                        }
                    }
                    else {
                        MYMPD_LOG_ERROR("Premature end of data");
                        break;
                    }
                }
                i = i + 2;
            }
        }
        else if (encoding == 3) {
            //utf8 - read text until \0 separator
            while (i < binary_length && binary_data[i] != '\0') {
                if ((binary_data[i] & 0x80) == 0x00) {
                    //ascii char
                    text_buf = sds_catjsonchar(text_buf, (char)binary_data[i]);
                }
                else {
                    text_buf = sdscatfmt(text_buf, "%c", binary_data[i]);
                }
                i++;
            }
        }
        else {
            MYMPD_LOG_ERROR("Unknown text encoding");
            break;
        }
        //skip separator
        if (i + sep_len < binary_length) {
            i = i + sep_len;
        }
        else {
            MYMPD_LOG_ERROR("Premature end of data");
            break;
        }
        //read timestamp - 4 bytes
        if (i + 3 < binary_length) {
            int ms = (binary_data[i] << 24) | (binary_data[i + 1] << 16) | (binary_data[i + 2] << 8) | binary_data[i + 3];
            int min = ms / 60000;
            ms = ms - min * 60000;
            int sec = ms / 1000;
            ms = ms - sec * 1000;
            sylt_text = sdscatprintf(sylt_text, "[%02d:%02d.%02d]%s\\n", min, sec, ms, text_buf);
            sdsclear(text_buf);
            i = i + 4;
        }
        else {
            MYMPD_LOG_ERROR("No timestamp found");
            break;
        }
    }
    FREE_SDS(text_buf);
    return sylt_text;
}
#endif

static int lyricsextract_flac(sds *buffer, sds media_file, bool is_ogg, const char *comment_name, bool synced, int returned_entities) {
    #ifdef ENABLE_FLAC
    MYMPD_LOG_DEBUG("Exctracting lyrics from %s", media_file);
    FLAC__StreamMetadata *metadata = NULL;
    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();

    if(! (is_ogg? FLAC__metadata_chain_read_ogg(chain, media_file) : FLAC__metadata_chain_read(chain, media_file)) ) {
        MYMPD_LOG_ERROR("%s: ERROR: reading metadata", media_file);
        FLAC__metadata_chain_delete(chain);
        return 0;
    }

    FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
    assert(iterator);
    FLAC__metadata_iterator_init(iterator, chain);
    int field_num = 0;
    FLAC__StreamMetadata *block;
    FLAC__bool ok = true;
    int found_lyrics = 0;
    do {
        block = FLAC__metadata_iterator_get_block(iterator);
        ok &= (0 != block);
        if (!ok) {
            MYMPD_LOG_ERROR("Could not get block from chain: %s", media_file);
        }
        else if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
            field_num = 0;
            while ((field_num = FLAC__metadata_object_vorbiscomment_find_entry_from(block, field_num, comment_name)) > -1) {
                metadata = block;
                FLAC__StreamMetadata_VorbisComment *vc = &metadata->data.vorbis_comment;
                FLAC__StreamMetadata_VorbisComment_Entry *field = &vc->comments[field_num++];

                char *field_value = memchr(field->entry, '=', field->length);
                if (field_value != NULL && strlen(field_value) > 1) {
                    MYMPD_LOG_DEBUG("Found embedded lyrics");
                    field_value++;
                    found_lyrics++;
                    if (returned_entities > 0) {
                       *buffer = sdscatlen(*buffer, ",", 1);
                    }
                    *buffer = sdscatlen(*buffer, "{", 1);
                    *buffer = tojson_bool(*buffer, "synced", synced, true);
                    *buffer = tojson_char(*buffer, "lang", "", true);
                    *buffer = tojson_char(*buffer, "desc", "", true);
                    *buffer = tojson_char(*buffer, "text", field_value, false);
                    *buffer = sdscatlen(*buffer, "}", 1);
                    returned_entities++;
                }
                else {
                    MYMPD_LOG_DEBUG("Invalid vorbis comment");
                }
            }
        }
    } while (ok && FLAC__metadata_iterator_next(iterator));

    if (found_lyrics == 0) {
        MYMPD_LOG_DEBUG("No embedded lyrics detected");
    }
    FLAC__metadata_iterator_delete(iterator);
    FLAC__metadata_chain_delete(chain);
    #else
    (void) media_file;
    (void) is_ogg;
    (void) comment_name;
    (void) synced;
    (void) buffer;
    #endif
    return returned_entities;
}
