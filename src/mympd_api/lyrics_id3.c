/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Read lyrics from ID3 tags
 */

#include "compile_time.h"
#include "src/mympd_api/lyrics_id3.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"

#include <id3tag.h>

// private definitions

static sds decode_sylt(const id3_byte_t *binary_data, id3_length_t binary_length, enum id3_field_textencoding encoding);
static const char *mympd_id3_field_getlanguage(union id3_field const *field);

// public functions

/**
 * Extracts unsynced lyrics from a id3 tagged mp3 file
 * @param extracted t_list struct to append found lyrics
 * @param media_file absolute filename to read lyrics from
 */
void lyricsextract_unsynced_id3(struct t_list *extracted, sds media_file) {
    MYMPD_LOG_DEBUG(NULL, "Exctracting unsynced lyrics from %s", media_file);
    struct id3_file *file_struct = id3_file_open(media_file, ID3_FILE_MODE_READONLY);
    if (file_struct == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can't parse id3_file: %s", media_file);
        return;
    }
    struct id3_tag *tags = id3_file_tag(file_struct);
    if (tags == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can't read id3 tags from file: %s", media_file);
        id3_file_close(file_struct);
        return;
    }

    unsigned i = 0;
    unsigned found_lyrics = 0;
    struct id3_frame *frame;
    sds buffer = sdsempty();
    while ((frame = id3_tag_findframe(tags, "USLT", i)) != NULL) {
        //fields of USLT:
        //ID3_FIELD_TYPE_TEXTENCODING -> can be ignored
        //ID3_FIELD_TYPE_LANGUAGE -> lang (3 chars)
        //ID3_FIELD_TYPE_STRING -> desc
        //ID3_FIELD_TYPE_STRINGFULL -> lyrics
        const id3_ucs4_t *uslt_text = id3_field_getfullstring(&frame->fields[3]);
        if (uslt_text != NULL) {
            buffer = sdscat(buffer, "{\"synced\":false,");
            //libid3tag has not get function for language, use own function
            const char *lang = mympd_id3_field_getlanguage(&frame->fields[1]);
            if (lang != NULL) {
                buffer = tojson_char(buffer, "lang", lang, true);
            }
            else {
                buffer = tojson_char_len(buffer, "lang", "", 0, true);
            }

            const id3_ucs4_t *uslt_desc = id3_field_getstring(&frame->fields[2]);
            if (uslt_desc != NULL) {
                id3_utf8_t *uslt_desc_utf8 = id3_ucs4_utf8duplicate(uslt_desc);
                buffer = tojson_char(buffer, "desc", (char *)uslt_desc_utf8, true);
                FREE_PTR(uslt_desc_utf8);
            }
            else {
                buffer = tojson_char_len(buffer, "desc", "", 0, true);
            }

            id3_utf8_t *uslt_text_utf8 = id3_ucs4_utf8duplicate(uslt_text);
            buffer = tojson_char(buffer, "text", (char *)uslt_text_utf8, false);
            FREE_PTR(uslt_text_utf8);
            buffer = sdscatlen(buffer, "}", 1);
            list_push(extracted, buffer, 0 , NULL, NULL);
            sdsclear(buffer);
            found_lyrics++;
            MYMPD_LOG_DEBUG(NULL, "Unsynced lyrics successfully extracted");
        }
        else {
            MYMPD_LOG_DEBUG(NULL, "Can not read embedded unsynced lyrics");
            break;
        }
        i++;
    }
    id3_file_close(file_struct);
    FREE_SDS(buffer);

    if (found_lyrics == 0) {
        MYMPD_LOG_DEBUG(NULL, "No embedded unsynced lyrics found");
    }
}

/**
 * Extracts synced lyrics from a id3 tagged mp3 file
 * @param extracted t_list struct to append found lyrics
 * @param media_file absolute filename to read lyrics from
 */
void lyricsextract_synced_id3(struct t_list *extracted, sds media_file) {
    MYMPD_LOG_DEBUG(NULL, "Exctracting synced lyrics from \"%s\"", media_file);
    struct id3_file *file_struct = id3_file_open(media_file, ID3_FILE_MODE_READONLY);
    if (file_struct == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can't parse id3_file: %s", media_file);
        return;
    }
    struct id3_tag *tags = id3_file_tag(file_struct);
    if (tags == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can't read id3 tags from file \"%s\"", media_file);
        id3_file_close(file_struct);
        return;
    }

    unsigned i = 0;
    unsigned found_lyrics = 0;
    struct id3_frame *frame;
    sds buffer = sdsempty();
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
            buffer = sdscat(buffer, "{\"synced\":true,");

            enum id3_field_textencoding encoding = id3_field_gettextencoding(&frame->fields[0]);
            buffer = tojson_uint(buffer, "encoding", encoding, true);

            const char *lang = mympd_id3_field_getlanguage(&frame->fields[1]);
            if (lang != NULL) {
                buffer = tojson_char(buffer, "lang", lang, true);
            }
            else {
                buffer = tojson_char_len(buffer, "lang", "", 0, true);
            }

            int64_t time_stamp = id3_field_getint(&frame->fields[2]);
            buffer = tojson_int64(buffer, "timestamp", time_stamp, true);

            int64_t content_type = id3_field_getint(&frame->fields[3]);
            buffer = tojson_int64(buffer, "contenttype", content_type, true);

            const id3_ucs4_t *uslt_desc = id3_field_getstring(&frame->fields[4]);
            if (uslt_desc != NULL) {
                id3_utf8_t *uslt_desc_utf8 = id3_ucs4_utf8duplicate(uslt_desc);
                buffer = tojson_char(buffer, "desc", (char *)uslt_desc_utf8, true);
                FREE_PTR(uslt_desc_utf8);
            }
            else {
                buffer = tojson_char_len(buffer, "desc", "", 0, true);
            }
            sds text = decode_sylt(sylt_data, sylt_data_len, encoding);
            //sylt data is already encoded as json value
            buffer = sdscatprintf(buffer, "\"text\":\"%s\"", text);
            FREE_SDS(text);
            buffer = sdscatlen(buffer, "}", 1);
            list_push(extracted, buffer, 0 , NULL, NULL);
            sdsclear(buffer);
            found_lyrics++;
            MYMPD_LOG_DEBUG(NULL, "Synced lyrics successfully extracted");
        }
        else {
            MYMPD_LOG_DEBUG(NULL, "Can not read embedded synced lyrics");
            break;
        }
        i++;
    }
    id3_file_close(file_struct);
    FREE_SDS(buffer);
    if (found_lyrics == 0) {
        MYMPD_LOG_DEBUG(NULL, "No embedded synced lyrics found");
    }
}

// private functions

/**
 * Custom function to get the id3 language field
 * libid3tag has not get function for language
 * @param field pointer to frame field
 * @return language
 */
static const char *mympd_id3_field_getlanguage(union id3_field const *field) {
    assert(field);
    if (field->type != ID3_FIELD_TYPE_LANGUAGE) {
        return NULL;
    }
    return field->immediate.value;
}

/**
 * Decodes the binary sylt tag from id3 tagged files
 * @param binary_data the binary sylt tag
 * @param binary_length length of the binary data
 * @param encoding text encoding
 * @return decoded sylt tag
 */
static sds decode_sylt(const id3_byte_t *binary_data, id3_length_t binary_length, enum id3_field_textencoding encoding) {
    sds sylt_text = sdsempty();
    //text buffer
    sds text_buf = sdsempty();
    id3_length_t sep_len = encoding == 0 || encoding == 3 ? 1 : 2;
    id3_length_t i = 0;

    MYMPD_LOG_DEBUG(NULL, "Sylt encoding: %u", encoding);

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
                    unsigned c = (unsigned)((binary_data[i + 1] << 8) | binary_data[i]);
                    if (c <= 0xd7ff || c >= 0xe000) {
                        text_buf = sdscatprintf(text_buf, "\\u%04x", c);
                    }
                    else {
                        //surrogate pair
                        c = (unsigned)((binary_data[i + 1] << 24) | (binary_data[i] << 16) | (binary_data[i + 3] << 8) | binary_data[i + 2]);
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
                    unsigned c = (unsigned)((binary_data[i] << 8) | binary_data[i + 1]);
                    if (c <= 0xd7ff || c >= 0xe000) {
                        text_buf = sdscatprintf(text_buf, "\\u%04x", c);
                    }
                    else if (i + 4 < binary_length) {
                        //surrogate pair
                        c = (unsigned)((binary_data[i] << 24) | (binary_data[i + 1] << 16) | (binary_data[i + 2] << 8) | binary_data[i + 3]);
                        c = c - 0x10000;
                        if (c <= 0x10ffff) {
                            text_buf = sdscatprintf(text_buf, "\\u%04x%04x", 0xd800 + (c >> 10), 0xdc00 + (c & 0x3ff));
                        }
                    }
                    else {
                        MYMPD_LOG_ERROR(NULL, "Premature end of data");
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
                    text_buf = sds_catchar(text_buf, (char)binary_data[i]);
                }
                i++;
            }
        }
        else {
            MYMPD_LOG_ERROR(NULL, "Unknown text encoding");
            break;
        }
        //skip separator
        if (i + sep_len < binary_length) {
            i = i + sep_len;
        }
        else {
            MYMPD_LOG_ERROR(NULL, "Premature end of data");
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
            MYMPD_LOG_ERROR(NULL, "No timestamp found");
            break;
        }
    }
    FREE_SDS(text_buf);
    return sylt_text;
}
