/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <limits.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <libgen.h>

#include "../../dist/src/sds/sds.h"
#include "../../dist/src/mongoose/mongoose.h"
#include "../../dist/src/frozen/frozen.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../log.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "web_server_utility.h"
#include "web_server_lyrics.h"

//optional includes
#ifdef ENABLE_LIBID3TAG
    #include <id3tag.h>
#endif

#ifdef ENABLE_FLAC
    #include <FLAC/metadata.h>
#endif

//privat definitions
static bool handle_lyricsextract(struct mg_connection *nc, const char *media_file);
static bool handle_lyricsextract_id3(const char *media_file, sds *text);
//static bool handle_lyricsextract_flac(const char *media_file, sds *text, bool is_ogg);

//public functions

//returns true if an image is served
//returns false if waiting for mpd_client to handle request - not implemented yet
bool handle_lyrics(struct mg_connection *nc, struct http_message *hm, t_mg_user_data *mg_user_data, t_config *config, int conn_id) {
    //decode uri
    sds uri_decoded = sdsurldecode(sdsempty(), hm->uri.p, (int)hm->uri.len, 0);
    if (sdslen(uri_decoded) == 0) {
        LOG_ERROR("Failed to decode uri");
        serve_plaintext(nc, "Failed to decode uri");
        sdsfree(uri_decoded);
        return true;
    }
    if (validate_uri(uri_decoded) == false) {
        LOG_ERROR("Invalid URI: %s", uri_decoded);
        serve_plaintext(nc, "Invalid URI");
        sdsfree(uri_decoded);
        return true;
    }
    //remove /lyrics/
    sdsrange(uri_decoded, 8, -1);
    //create absolute file
    sds mediafile = sdscatfmt(sdsempty(), "%s/%s", mg_user_data->music_directory, uri_decoded);
    LOG_DEBUG("Absolut media_file: %s", mediafile);

    //check music_directory folder
    if (mg_user_data->feat_library == true && access(mediafile, F_OK) == 0) /* Flawfinder: ignore */
    {
        //try txt file in folder under music_directory
        char *uricpy = strdup(uri_decoded);
        strip_extension(uricpy);
        sds lyricsfile = sdscatfmt(sdsempty(), "%s/%s.txt", mg_user_data->music_directory, uricpy);
        FREE_PTR(uricpy);
        
        if (access(lyricsfile, F_OK ) == 0) { /* Flawfinder: ignore */
            LOG_DEBUG("Serving file %s (text/plain)", lyricsfile);
            mg_http_serve_file(nc, hm, lyricsfile, mg_mk_str("text/plain"), mg_mk_str(EXTRA_HEADERS_CACHE));
            sdsfree(uri_decoded);
            sdsfree(lyricsfile);
            sdsfree(mediafile);
            return true;
        }
        sdsfree(lyricsfile);
        LOG_DEBUG("No lyrics file found in music directory");
        //try to extract lyrics from media file
        bool rc = handle_lyricsextract(nc, mediafile);
        if (rc == true) {
            sdsfree(uri_decoded);
            sdsfree(mediafile);
            return true;
        }
    }
    LOG_VERBOSE("No lyrics found for %s", mediafile);
    sdsfree(mediafile);
    sdsfree(uri_decoded);
    serve_plaintext(nc, "No lyrics found");

    (void) config;    
    (void) conn_id;
    return true;
}

//privat functions
static bool handle_lyricsextract(struct mg_connection *nc, const char *media_file) {
    bool rc = false;
    sds text = sdsempty();
    sds mime_type_media_file = get_mime_type_by_ext(media_file);
    if (strcmp(mime_type_media_file, "audio/mpeg") == 0) {
        rc = handle_lyricsextract_id3(media_file, &text);
    }
    /*
    else if (strcmp(mime_type_media_file, "audio/ogg") == 0) {
        rc = handle_lyricsextract_flac(media_file, &text, true);
    }
    else if (strcmp(mime_type_media_file, "audio/flac") == 0) {
        rc = handle_lyricsextract_flac(media_file, &text, false);
    }
    */
    sdsfree(mime_type_media_file);
    if (rc == true) {
        sds header = sdscatfmt(sdsempty(), "Content-Type: text/plain\r\n");
        header = sdscat(header, EXTRA_HEADERS_CACHE);
        mg_send_head(nc, 200, sdslen(text), header);
        mg_send(nc, text, sdslen(text));
        sdsfree(header);
    }
    sdsfree(text);
    return rc;
}

static bool handle_lyricsextract_id3(const char *media_file, sds *text) {
    bool rc = false;
    #ifdef ENABLE_LIBID3TAG
    LOG_DEBUG("Exctracting lyrics from %s", media_file);
    struct id3_file *file_struct = id3_file_open(media_file, ID3_FILE_MODE_READONLY);
    if (file_struct == NULL) {
        LOG_ERROR("Can't parse id3_file: %s", media_file);
        return false;
    }
    struct id3_tag *tags = id3_file_tag(file_struct);
    if (tags == NULL) {
        LOG_ERROR("Can't read id3 tags from file: %s", media_file);
        return false;
    }
    struct id3_frame *frame = id3_tag_findframe(tags, "USLT", 0);
    if (frame != NULL) {
        const id3_ucs4_t *ulst_u = id3_field_getfullstring(&frame->fields[3]);
        if (ulst_u != NULL) {
            id3_utf8_t *ulst = id3_ucs4_utf8duplicate(ulst_u);
            *text = sdscat(*text, (char *)ulst);
            FREE_PTR(ulst);
            LOG_DEBUG("Lyrics successfully extracted");
            rc = true;        
        }
        else {
            LOG_DEBUG("Can not read embedded lyrics");
        }
    }
    else {
        LOG_DEBUG("No embedded lyrics detected");
    }
    id3_file_close(file_struct);
    #else
    (void) media_file;
    (void) text;
    #endif
    return rc;
}

/*
static bool handle_lyricsextract_flac(const char *media_file, sds *text, bool is_ogg) {
    bool rc = false;
    #ifdef ENABLE_FLAC
    LOG_DEBUG("Exctracting lyrics from %s", media_file);
    FLAC__StreamMetadata *metadata = NULL;

    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();
    
    if(! (is_ogg? FLAC__metadata_chain_read_ogg(chain, media_file) : FLAC__metadata_chain_read(chain, media_file)) ) {
        LOG_DEBUG("%s: ERROR: reading metadata", media_file);
        FLAC__metadata_chain_delete(chain);
        return false;
    }

    FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
    FLAC__metadata_iterator_init(iterator, chain);
    assert(iterator);
    
    do {
        FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
        if (block->type == FLAC__METADATA_TYPE_PICTURE) {
            metadata = block;
        }
    } while (FLAC__metadata_iterator_next(iterator) && metadata == NULL);
    
    if (metadata == NULL) {
        LOG_DEBUG("No embedded lyrics detected");
    }
    else {
        *text = sdscatlen(*text, metadata->data.picture.data, metadata->data.picture.data_length);
        LOG_DEBUG("Lyrics successfully extracted");
        rc = true;
    }
    FLAC__metadata_iterator_delete(iterator);
    FLAC__metadata_chain_delete(chain);
    #else
    (void) media_file;
    (void) text;
    (void) is_ogg;
    #endif
    return rc;
}
*/
