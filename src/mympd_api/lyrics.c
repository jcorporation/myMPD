/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/lyrics.h"

#include "src/lib/cache_disk_lyrics.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mympd_api/trigger.h"

#include <string.h>

//optional includes
#ifdef MYMPD_ENABLE_LIBID3TAG
    #include "src/mympd_api/lyrics_id3.h"
#endif

#ifdef MYMPD_ENABLE_FLAC
    #include "src/mympd_api/lyrics_flac.h"
#endif

/**
 * Privat definitions
 */
static void lyrics_get(struct t_lyrics *lyrics, struct t_list *extracted,
        sds mediafile, const char *mime_type_mediafile);
static void lyrics_fromfile(struct t_list *extracted, sds mediafile, const char *ext, bool synced);

/**
 * Public functions
 */

/**
 * Gets synced and unsynced lyrics from filesystem and embedded
 * @param mympd_state pointer to mympd_state
 * @param buffer buffer to write the response
 * @param uri song uri 
 * @param partition mpd partition
 * @param conn_id mongoose connection id
 * @param request_id jsonrpc id
 * @return pointer to buffer
 */
sds mympd_api_lyrics_get(struct t_mympd_state *mympd_state, sds buffer,
        sds uri, sds partition, unsigned long conn_id, unsigned request_id)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_LYRICS_GET;
    struct t_list extracted;
    list_init(&extracted);

    // check cache
    sds cache_file = cache_disk_lyrics_get_name(mympd_state->config->cachedir, uri);
    int nread = 0;
    sds content = sds_getfile(sdsempty(), cache_file, CONTENT_LEN_MAX, true, false, &nread);
    if (nread > 0) {
        if (validate_json_object(content) == true) {
            MYMPD_LOG_DEBUG(partition, "Found cached lyrics");
            list_push(&extracted, content, 0, NULL, NULL);
        }
        else {
            MYMPD_LOG_WARN(partition, "Invalid cached lyrics found, removing file");
            rm_file(cache_file);
        }
    }
    FREE_SDS(cache_file);
    FREE_SDS(content);

    // get lyrics only for local uri and if we have access to the mpd music directory
    if (is_streamuri(uri) == false &&
        sdslen(mympd_state->mpd_state->music_directory_value) > 0)
    {
        sds mediafile = sdscatfmt(sdsempty(), "%S/%S", mympd_state->mpd_state->music_directory_value, uri);
        const char *mime_type_mediafile = get_mime_type_by_ext(mediafile);
        lyrics_get(&mympd_state->lyrics, &extracted, mediafile, mime_type_mediafile);
        FREE_SDS(mediafile);
    }

    if (extracted.length == 0) {
        #ifdef MYMPD_ENABLE_LUA
            // no lyrics found, check if there is a trigger to fetch lyrics
            int n = mympd_api_trigger_execute_http(&mympd_state->trigger_list, TRIGGER_MYMPD_LYRICS, uri, partition, conn_id, request_id);
            if (n > 0) {
                // return empty buffer, response must be send by triggered script
                if (n > 1) {
                    MYMPD_LOG_WARN(partition, "More than one script triggered for lyrics.");
                }
                return buffer;
            }
        #else
            (void)conn_id;
        #endif
        // no trigger
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_LYRICS, JSONRPC_SEVERITY_INFO, "No lyrics found");
    }
    else {
        // lyrics found
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
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
    }

    return buffer;
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
