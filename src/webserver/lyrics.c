/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Lyrics functions
 */

#include "compile_time.h"
#include "src/webserver/lyrics.h"

#include "src/lib/cache/cache_disk_lyrics.h"
#include "src/lib/filehandler.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_query.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"
#include "src/webserver/response.h"

#include <string.h>

//optional includes
#ifdef MYMPD_ENABLE_LIBID3TAG
    #include "src/webserver/lyrics_id3.h"
#endif

#ifdef MYMPD_ENABLE_FLAC
    #include "src/webserver/lyrics_flac.h"
#endif

/**
 * Privat definitions
 */
static void lyrics_get(struct t_lyrics *lyrics, struct t_list *extracted,
        sds mediafile, const char *mime_type_mediafile);
static void lyrics_fromfile(struct t_list *extracted, sds mediafile, const char *ext, bool synced);

/**
 * Gets synced and unsynced lyrics from filesystem and embedded
 * @param nc Mongoose connection
 * @param request_id Jsonrpc id
 * @param uri Song uri 
 * @return true if a response was send, else false
 */
bool webserver_lyrics_get(struct mg_connection *nc, unsigned request_id, sds body) {
    sds uri = NULL;
    struct t_json_parse_error parse_error;
    enum mympd_cmd_ids cmd_id = MYMPD_API_LYRICS_GET;
    json_parse_error_init(&parse_error);
    if (json_get_string(body, "$.params.uri", 1, FILEPATH_LEN_MAX, &uri, vcb_isfilepath, &parse_error) == false) {
        sds buffer = jsonrpc_respond_message_phrase(sdsempty(), cmd_id, request_id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, parse_error.message, 2, "path", parse_error.path);
        webserver_send_data(nc, buffer, sdslen(buffer), EXTRA_HEADERS_JSON_CONTENT);
        json_parse_error_clear(&parse_error);
        return true;
    }
    json_parse_error_clear(&parse_error);

    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    struct t_config *config = mg_user_data->config;

    struct t_list extracted;
    list_init(&extracted);

    // check cache
    sds cache_file = cache_disk_lyrics_get_name(config->cachedir, uri);
    int nread = 0;
    sds content = sds_getfile(sdsempty(), cache_file, CONTENT_LEN_MAX, true, false, &nread);
    if (nread > 0) {
        if (validate_json_object(content) == true) {
            MYMPD_LOG_DEBUG(NULL, "Found cached lyrics");
            list_push(&extracted, content, 0, NULL, NULL);
        }
        else {
            MYMPD_LOG_WARN(NULL, "Invalid cached lyrics found, removing file");
            rm_file(cache_file);
        }
    }
    FREE_SDS(cache_file);
    FREE_SDS(content);

    // get lyrics only for local uri and if we have access to the mpd music directory
    if (is_streamuri(uri) == false &&
        sdslen(mg_user_data->music_directory) > 0)
    {
        sds mediafile = sdscatfmt(sdsempty(), "%S/%S", mg_user_data->music_directory, uri);
        const char *mime_type_mediafile = get_mime_type_by_ext(mediafile);
        lyrics_get(&mg_user_data->lyrics, &extracted, mediafile, mime_type_mediafile);
        FREE_SDS(mediafile);
    }
    FREE_SDS(uri);

    if (extracted.length > 0) {
        // lyrics found
        sds buffer = jsonrpc_respond_start(sdsempty(), cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        struct t_list_node *current = NULL;
        unsigned entity_count = 0;
        while ((current = list_shift_first(&extracted)) != NULL) {
            if (entity_count++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatsds(buffer, current->key);
            list_node_free(current);
        }
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "totalEntities", entity_count, true);
        buffer = tojson_uint(buffer, "returnedEntities", entity_count, false);
        buffer = jsonrpc_end(buffer);
        webserver_send_data(nc, buffer, sdslen(buffer), EXTRA_HEADERS_JSON_CONTENT);
        FREE_SDS(buffer);
        return true;
    }
    // No lyrics found
    return false;
}

/**
 * Private functions
 */

/**
 * Retrieves lyrics and appends it to extracted list
 * @param lyrics pointer to lyrics configuration
 * @param extracted t_list struct to append found lyrics
 * @param mediafile absolute filepath of song uri
 * @param mime_type_mediafile mime type of the song uri
 */
static void lyrics_get(struct t_lyrics *lyrics, struct t_list *extracted,
        sds mediafile, const char *mime_type_mediafile)
{
    //try unsynced lyrics file in folder of the song
    lyrics_fromfile(extracted, mediafile, lyrics->uslt_ext, false);
    //get embedded unsynced lyrics
    if (strcmp(mime_type_mediafile, "audio/mpeg") == 0) {
        #ifdef MYMPD_ENABLE_LIBID3TAG
            lyricsextract_unsynced_id3(extracted, mediafile);
        #endif
    }
    else if (strcmp(mime_type_mediafile, "audio/ogg") == 0) {
        #ifdef MYMPD_ENABLE_FLAC
            lyricsextract_flac(extracted, mediafile, true, lyrics->vorbis_uslt, false);
        #endif
    }
    else if (strcmp(mime_type_mediafile, "audio/flac") == 0) {
        #ifdef MYMPD_ENABLE_FLAC
            lyricsextract_flac(extracted, mediafile, false, lyrics->vorbis_uslt, false);
        #endif
    }
    //try synced lyrics file in folder of the song
    lyrics_fromfile(extracted, mediafile, lyrics->sylt_ext, true);
    //get embedded synced lyrics
    if (strcmp(mime_type_mediafile, "audio/mpeg") == 0) {
        #ifdef MYMPD_ENABLE_LIBID3TAG
            lyricsextract_synced_id3(extracted, mediafile);
        #endif
    }
    else if (strcmp(mime_type_mediafile, "audio/ogg") == 0) {
        #ifdef MYMPD_ENABLE_FLAC
            lyricsextract_flac(extracted, mediafile, true, lyrics->vorbis_sylt, true);
        #endif
    }
    else if (strcmp(mime_type_mediafile, "audio/flac") == 0) {
        #ifdef MYMPD_ENABLE_FLAC
            lyricsextract_flac(extracted, mediafile, false, lyrics->vorbis_sylt, true);
        #endif
    }
}

/**
 * Reads lyrics from a textfiles
 * @param extracted t_list struct to append found lyrics
 * @param mediafile absolute filepath of song uri
 * @param ext file extension
 * @param synced true for synced lyrics else false
 */
static void lyrics_fromfile(struct t_list *extracted, sds mediafile, const char *ext, bool synced) {
    //try file in folder in the music directory
    sds lyricsfile = replace_file_extension(mediafile, ext);
    MYMPD_LOG_DEBUG(NULL, "Trying to open lyrics file: %s", lyricsfile);
    int nread = 0;
    sds text = sds_getfile(sdsempty(), lyricsfile, LYRICS_SIZE_MAX, false, false, &nread);
    if (nread > 0) {
        sds buffer = sdsempty();
        buffer = sdscatlen(buffer, "{", 1);
        buffer = tojson_bool(buffer, "synced", synced, true);
        buffer = tojson_char_len(buffer, "lang", "", 0, true);
        buffer = tojson_char_len(buffer, "desc", "", 0, true);
        buffer = tojson_sds(buffer, "text", text, false);
        buffer = sdscatlen(buffer, "}", 1);
        list_push(extracted, buffer, 0 , NULL, NULL);
        FREE_SDS(buffer);
    }
    FREE_SDS(text);
    FREE_SDS(lyricsfile);
}
