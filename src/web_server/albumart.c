/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/albumart.h"

#include "src/lib/api.h"
#include "src/lib/cache_disk.h"
#include "src/lib/convert.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"

#include <libgen.h>

//optional includes
#ifdef MYMPD_ENABLE_LIBID3TAG
    #include "src/web_server/albumart_id3.h"
#endif

#ifdef MYMPD_ENABLE_FLAC
    #include "src/web_server/albumart_flac.h"
#endif

/**
 * Privat definitions
 */
static bool handle_coverextract(struct mg_connection *nc, sds cachedir, const char *uri, const char *media_file, bool covercache, int offset);

/**
 * Public functions
 */

/**
 * Sends the albumart redirect response from the albumart_by_albumid handler
 * @param nc mongoose connection
 * @param data jsonrpc response
 */
void webserver_send_albumart_redirect(struct mg_connection *nc, sds data) {
    sds uri = NULL;
    unsigned size;
    if (json_get_string_max(data, "$.result.uri", &uri, vcb_isuri, NULL) == true &&
        json_get_uint(data, "$.result.size", 0, 1, &size, NULL) == true)
    {
        sds redirect_uri = size == ALBUMART_THUMBNAIL
            ? sdscatfmt(sdsempty(),"/albumart-thumb?offset=0&uri=")
            : sdscatfmt(sdsempty(),"/albumart?offset=0&uri=");
        redirect_uri = sds_urlencode(redirect_uri, uri, sdslen(uri));
        MYMPD_LOG_DEBUG(NULL, "Sending redirect to: %s", redirect_uri);
        webserver_send_header_found(nc, redirect_uri, "");
        FREE_SDS(redirect_uri);
    }
    else {
        webserver_serve_placeholder_image(nc, PLACEHOLDER_NA);
    }
    FREE_SDS(uri);
}

/**
 * Sends the albumart response from mpd to the client
 * @param nc mongoose connection
 * @param data jsonrpc response
 * @param binary the image
 */
void webserver_send_albumart(struct mg_connection *nc, sds data, sds binary) {
    size_t len = sdslen(binary);
    sds mime_type = NULL;
    if (len > 0 &&
        json_get_string(data, "$.result.mime_type", 1, 200, &mime_type, vcb_isname, NULL) == true &&
        strncmp(mime_type, "image/", 6) == 0)
    {
        MYMPD_LOG_DEBUG(NULL, "Serving albumart from memory (%s - %lu bytes) (%lu)", mime_type, (unsigned long)len, nc->id);
        sds headers = sdscatfmt(sdsempty(), "Content-Type: %S\r\n", mime_type);
        headers = sdscat(headers, EXTRA_HEADERS_IMAGE);
        webserver_send_data(nc, binary, len, headers);
        FREE_SDS(headers);
    }
    else {
        webserver_serve_placeholder_image(nc, PLACEHOLDER_NA);
    }
    FREE_SDS(mime_type);
}

/**
 * Request handler for /albumart/albumid and /albumart-thumb/albumid.
 * Sends the request to the mympd_api thread.
 * @param hm http message
 * @param conn_id connection id
 * @param size size of the albumart
 */
void request_handler_albumart_by_album_id(struct mg_http_message *hm, unsigned long conn_id, enum albumart_sizes size) {
    sds albumid = sdsnewlen(hm->uri.buf, hm->uri.len);
    basename_uri(albumid);
    MYMPD_LOG_DEBUG(NULL, "Sending getalbumart to mpd_client_queue");
    struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, conn_id, 0, INTERNAL_API_ALBUMART_BY_ALBUMID, NULL, MPD_PARTITION_DEFAULT);
    request->data = tojson_sds(request->data, "albumid", albumid, true);
    request->data = tojson_uint(request->data, "size", size, false);
    request->data = jsonrpc_end(request->data);
    mympd_queue_push(mympd_api_queue, request, 0);
    FREE_SDS(albumid);
}

/**
 * Request handler for /albumart and /albumart-thumb
 * @param nc mongoose connection
 * @param hm http message
 * @param mg_user_data pointer to mongoose configuration
 * @param conn_id connection id
 * @param size albumart size
 * @return true: an image was served,
 *         false: request was sent to the mympd_api thread to get the image by MPD
 */
bool request_handler_albumart_by_uri(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data, unsigned long conn_id, enum albumart_sizes size)
{
    struct t_config *config = mg_user_data->config;

    sds offset_s = get_uri_param(&hm->query, "offset=");
    sds uri = get_uri_param(&hm->query, "uri=");
    int offset;

    if (uri == NULL ||
        offset_s == NULL ||
        str2int(&offset, offset_s) != STR2INT_SUCCESS ||
        vcb_isuri(uri) == false ||
        sdslen(uri) == 0)
    {
        MYMPD_LOG_ERROR(NULL, "Failed to decode query");
        webserver_serve_placeholder_image(nc, PLACEHOLDER_NA);
        FREE_SDS(offset_s);
        FREE_SDS(uri);
        return true;
    }
    FREE_SDS(offset_s);

    MYMPD_LOG_DEBUG(NULL, "Handle albumart for uri \"%s\", offset %d", uri, offset);

    //check covercache and serve image from it if found
    if (check_imagescache(nc, hm, mg_user_data, DIR_CACHE_COVER, uri, offset) == true) {
        FREE_SDS(uri);
        return true;
    }

    //uri too long
    if (sdslen(uri) > FILEPATH_LEN_MAX) {
        FREE_SDS(uri);
        MYMPD_LOG_WARN(NULL, "Uri is too long, max len is %d", FILEPATH_LEN_MAX);
        webserver_serve_placeholder_image(nc, PLACEHOLDER_NA);
        return true;
    }

    //check for cover in /pics/thumbs/ and webradios
    if (is_streamuri(uri) == true) {
        if (sdslen(uri) > FILENAME_LEN_MAX) {
            FREE_SDS(uri);
            MYMPD_LOG_DEBUG(NULL, "Uri is too long, max len is %d", FILENAME_LEN_MAX);
            webserver_serve_placeholder_image(nc, PLACEHOLDER_STREAM);
            return true;
        }

        sds sanitized_uri = sdsdup(uri);
        sanitize_filename(sanitized_uri);
        sds coverfile = sdscatfmt(sdsempty(), "%S/%s/%S", config->workdir, DIR_WORK_PICS_THUMBS, sanitized_uri);
        FREE_SDS(sanitized_uri);
        MYMPD_LOG_DEBUG(NULL, "Check for stream cover \"%s\"", coverfile);
        coverfile = webserver_find_image_file(coverfile);
        if (sdslen(coverfile) > 0) {
            //found a local coverfile
            webserver_serve_file(nc, hm, mg_user_data->browse_directory, coverfile);
            FREE_SDS(uri);
            FREE_SDS(coverfile);
            return true;
        }
        // TODO: get the webradio image
        webserver_serve_placeholder_image(nc, PLACEHOLDER_STREAM);
        FREE_SDS(uri);
        return true;
    }

    if (sdslen(mg_user_data->music_directory) > 0) {
        //create absolute file
        sds mediafile = sdscatfmt(sdsempty(), "%S/%S", mg_user_data->music_directory, uri);
        MYMPD_LOG_DEBUG(NULL, "Absolut media_file: %s", mediafile);
        //try image in folder under music_directory
        if (mg_user_data->coverimage_names_len > 0 &&
            offset == 0)
        {
            sds path = sdsdup(uri);
            path = sds_dirname(path);
            if (is_virtual_cuedir(mg_user_data->music_directory, path) == true) {
                //fix virtual cue sheet directories
                path = sds_dirname(path);
            }
            bool found = false;
            sds coverfile = sdsempty();
            if (size == ALBUMART_THUMBNAIL) {
                found = find_image_in_folder(&coverfile, mg_user_data->music_directory, path, mg_user_data->thumbnail_names, mg_user_data->thumbnail_names_len);
            }
            if (found == false) {
                found = find_image_in_folder(&coverfile, mg_user_data->music_directory, path, mg_user_data->coverimage_names, mg_user_data->coverimage_names_len);
            }
            if (found == true) {
                webserver_serve_file(nc, hm, mg_user_data->browse_directory, coverfile);
                FREE_SDS(uri);
                FREE_SDS(coverfile);
                FREE_SDS(mediafile);
                FREE_SDS(path);
                return true;
            }

            FREE_SDS(coverfile);
            MYMPD_LOG_DEBUG(NULL, "No cover file found in music directory");
            FREE_SDS(path);
        }

        if (testfile_read(mediafile) == true) {
            //try to extract albumart from media file
            bool covercache = mg_user_data->config->cache_cover_keep_days != CACHE_DISK_DISABLED
                ? true
                : false;
            bool rc = handle_coverextract(nc, config->cachedir, uri, mediafile, covercache, offset);
            if (rc == true) {
                FREE_SDS(uri);
                FREE_SDS(mediafile);
                return true;
            }
        }
        FREE_SDS(mediafile);
    }

    //ask mpd - mpd can read only first image
    if (mg_user_data->feat_albumart == true &&
        offset == 0)
    {
        MYMPD_LOG_DEBUG(NULL, "Sending INTERNAL_API_ALBUMART_BY_URI to mympdapi_queue");
        struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, conn_id, 0, INTERNAL_API_ALBUMART_BY_URI, NULL, MPD_PARTITION_DEFAULT);
        request->data = tojson_sds(request->data, "uri", uri, false);
        request->data = jsonrpc_end(request->data);
        mympd_queue_push(mympd_api_queue, request, 0);
        FREE_SDS(uri);
        return false;
    }

    MYMPD_LOG_INFO(NULL, "No coverimage found for \"%s\"", uri);
    FREE_SDS(uri);
    webserver_serve_placeholder_image(nc, PLACEHOLDER_NA);
    return true;
}

/**
 * Extracts albumart from media files
 * @param nc mongoose connection
 * @param cachedir covercache directory
 * @param uri song uri
 * @param media_file full path to the song
 * @param covercache true = covercache is enabled
 * @param offset number of embedded image to extract
 * @return true on success, else false
 */
static bool handle_coverextract(struct mg_connection *nc, sds cachedir,
        const char *uri, const char *media_file, bool covercache, int offset)
{
    #if !defined MYMPD_ENABLE_LIBID3TAG && !defined MYMPD_ENABLE_FLAC
        (void) covercache;
        (void) cachedir;
        (void) offset;
        return false;
    #endif

    bool rc = false;
    const char *mime_type_media_file = get_mime_type_by_ext(media_file);
    MYMPD_LOG_DEBUG(NULL, "Handle coverextract for uri \"%s\"", uri);
    MYMPD_LOG_DEBUG(NULL, "Mimetype of %s is %s", media_file, mime_type_media_file);
    sds binary = sdsempty();
    if (strcmp(mime_type_media_file, "audio/mpeg") == 0) {
        #ifdef MYMPD_ENABLE_LIBID3TAG
            rc = handle_coverextract_id3(cachedir, uri, media_file, &binary, covercache, offset);
        #endif
    }
    else if (strcmp(mime_type_media_file, "audio/ogg") == 0) {
        #ifdef MYMPD_ENABLE_FLAC
            rc = handle_coverextract_flac(cachedir, uri, media_file, &binary, true, covercache, offset);
        #endif
    }
    else if (strcmp(mime_type_media_file, "audio/flac") == 0) {
        #ifdef MYMPD_ENABLE_FLAC
            rc = handle_coverextract_flac(cachedir, uri, media_file, &binary, false, covercache, offset);
        #endif
    }
    if (rc == true) {
        const char *mime_type = get_mime_type_by_magic_stream(binary);
        MYMPD_LOG_DEBUG(NULL, "Serving coverimage for \"%s\" (%s)", media_file, mime_type);
        sds headers = sdscatfmt(sdsempty(), "Content-Type: %s\r\n", mime_type);
        headers = sdscat(headers, EXTRA_HEADERS_IMAGE);
        webserver_send_data(nc, binary,  sdslen(binary), headers);
        FREE_SDS(headers);
    }
    FREE_SDS(binary);
    return rc;
}
