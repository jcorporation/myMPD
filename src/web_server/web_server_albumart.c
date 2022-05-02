/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "web_server_albumart.h"

#include "../lib/api.h"
#include "../lib/covercache.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/m3u.h"
#include "../lib/mimetype.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mympd_api/mympd_api_utility.h"

#include <assert.h>
#include <libgen.h>

//optional includes
#ifdef ENABLE_LIBID3TAG
    #include <id3tag.h>
#endif

#ifdef ENABLE_FLAC
    #include <FLAC/metadata.h>
#endif

//privat definitions
static bool handle_coverextract(struct mg_connection *nc, struct t_config *config, const char *uri, const char *media_file, bool covercache, int offset);
static bool handle_coverextract_id3(struct t_config *config, const char *uri, const char *media_file, sds *binary, bool covercache, int offset);
static bool handle_coverextract_flac(struct t_config *config, const char *uri, const char *media_file, sds *binary, bool is_ogg, bool covercache, int offset);

//public functions
void webserver_albumart_send(struct mg_connection *nc, sds data, sds binary) {
    size_t len = sdslen(binary);
    sds mime_type = NULL;
    if (len > 0 &&
        json_get_string(data, "$.result.mime_type", 1, 200, &mime_type, vcb_isname, NULL) == true &&
        strncmp(mime_type, "image/", 6) == 0)
    {
        MYMPD_LOG_DEBUG("Serving albumart from memory (%s - %lu bytes) (%lu)", mime_type, (unsigned long)len, nc->id);
        sds headers = sdscatfmt(sdsempty(), "Content-Type: %S\r\n", mime_type);
        headers = sdscat(headers, EXTRA_HEADERS_CACHE);
        webserver_send_data(nc, binary, len, headers);
        FREE_SDS(headers);
    }
    else {
        //create dummy http message and serve not available image
        struct mg_http_message hm;
        webserver_populate_dummy_hm(nc, &hm);
        webserver_serve_na_image(nc, &hm);
    }
    FREE_SDS(mime_type);
}

//returns true if an image is served
//returns false if waiting for mpd_client to handle request
bool webserver_albumart_handler(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data, struct t_config *config, long long conn_id,
        enum albumart_sizes size)
{
    sds query = sdsnewlen(hm->query.ptr, hm->query.len);
    sds uri_decoded = sdsempty();
    int offset = 0;
    if (sdslen(query) > 13 &&
        strncmp(query, "offset=", 7) == 0)
    {
        //remove offset=
        sdsrange(query, 7, -1);
        offset = sds_toimax(query);
        if (sdslen(query) > 5 &&
            strncmp(query, "&uri=", 5) == 0)
        {
            //remove &uri=
            sdsrange(query, 5, -1);
            //decode uri
            uri_decoded = sds_urldecode(uri_decoded, query, sdslen(query), 0);
        }
    }
    sdsfree(query);
    if (sdslen(uri_decoded) == 0) {
        MYMPD_LOG_ERROR("Failed to decode query");
        webserver_serve_na_image(nc, hm);
        FREE_SDS(uri_decoded);
        return true;
    }

    if (is_streamuri(uri_decoded) == false &&
        vcb_isfilepath(uri_decoded) == false)
    {
        MYMPD_LOG_ERROR("Invalid URI: %s", uri_decoded);
        webserver_serve_na_image(nc, hm);
        FREE_SDS(uri_decoded);
        return true;
    }

    MYMPD_LOG_DEBUG("Handle albumart for uri \"%s\", offset %d", uri_decoded, offset);
    //check for cover in /pics/thumbs/ and webradio m3u
    if (is_streamuri(uri_decoded) == true) {
        if (sdslen(uri_decoded) == 0) {
            MYMPD_LOG_ERROR("Uri to short");
            webserver_serve_na_image(nc, hm);
            FREE_SDS(uri_decoded);
            return true;
        }
        sds_sanitize_filename(uri_decoded);

        sds coverfile = sdscatfmt(sdsempty(), "%S/pics/thumbs/%S", config->workdir, uri_decoded);
        MYMPD_LOG_DEBUG("Check for stream cover \"%s\"", coverfile);
        coverfile = webserver_find_image_file(coverfile);

        if (sdslen(coverfile) == 0) {
            //no coverfile found, next try to find a webradio m3u
            sds webradio_file = sdscatfmt(sdsempty(), "%S/webradios/%S.m3u", config->workdir, uri_decoded);
            MYMPD_LOG_DEBUG("Check for webradio playlist \"%s\"", webradio_file);
            if (access(webradio_file, F_OK) == 0) { /* Flawfinder: ignore */
                sds extimg = m3u_get_field(sdsempty(), "#EXTIMG", webradio_file);
                if (is_streamuri(extimg) == true) {
                    //full uri, send a temporary redirect
                    webserver_send_header_found(nc, extimg);
                    FREE_SDS(uri_decoded);
                    FREE_SDS(coverfile);
                    FREE_SDS(extimg);
                    FREE_SDS(webradio_file);
                    return true;
                }
                if (sdslen(extimg) > 0) {
                    coverfile = sdscatfmt(sdsempty(), "%S/pics/thumbs/%S", config->workdir, extimg);
                }
                FREE_SDS(extimg);
            }
            FREE_SDS(webradio_file);
        }
        if (sdslen(coverfile) > 0) {
            //found a local coverfile
            const char *mime_type = get_mime_type_by_ext(coverfile);
            MYMPD_LOG_DEBUG("Serving file \"%s\" (%s)", coverfile, mime_type);
            static struct mg_http_serve_opts s_http_server_opts;
            s_http_server_opts.root_dir = mg_user_data->browse_directory;
            s_http_server_opts.extra_headers = EXTRA_HEADERS_CACHE;
            s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
            mg_http_serve_file(nc, hm, coverfile, &s_http_server_opts);
            webserver_handle_connection_close(nc);
        }
        else {
            //serve fallback image
            webserver_serve_stream_image(nc, hm);
        }
        FREE_SDS(uri_decoded);
        FREE_SDS(coverfile);
        return true;
    }

    //check covercache
    if (mg_user_data->covercache == true) {
        sds filename = sds_hash(uri_decoded);
        sds covercachefile = sdscatprintf(sdsempty(), "%s/covercache/%s-%d", config->cachedir, filename, offset);
        FREE_SDS(filename);
        covercachefile = webserver_find_image_file(covercachefile);
        if (sdslen(covercachefile) > 0) {
            const char *mime_type = get_mime_type_by_ext(covercachefile);
            MYMPD_LOG_DEBUG("Serving file %s (%s)", covercachefile, mime_type);
            static struct mg_http_serve_opts s_http_server_opts;
            s_http_server_opts.root_dir = mg_user_data->browse_directory;
            s_http_server_opts.extra_headers = EXTRA_HEADERS_CACHE;
            s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
            mg_http_serve_file(nc, hm, covercachefile, &s_http_server_opts);
            webserver_handle_connection_close(nc);
            FREE_SDS(uri_decoded);
            FREE_SDS(covercachefile);
            return true;
        }
        MYMPD_LOG_DEBUG("No covercache file found");
        FREE_SDS(covercachefile);
    }

    if (sdslen(mg_user_data->music_directory) > 0) {
        //create absolute file
        sds mediafile = sdscatfmt(sdsempty(), "%S/%S", mg_user_data->music_directory, uri_decoded);
        MYMPD_LOG_DEBUG("Absolut media_file: %s", mediafile);
        //try image in folder under music_directory
        if (mg_user_data->coverimage_names_len > 0 &&
            offset == 0)
        {
            sds path = sdsdup(uri_decoded);
            dirname(path);
            sdsupdatelen(path);
            if (is_virtual_cuedir(mg_user_data->music_directory, path) == true) {
                //fix virtual cue sheet directories
                dirname(path);
                sdsupdatelen(path);
            }
            bool found = false;
            sds coverfile = sdsempty();
            if (size == ALBUMART_THUMBNAIL) {
                //thumbnail images
                for (int j = 0; j < mg_user_data->thumbnail_names_len; j++) {
                    coverfile = sdscatfmt(coverfile, "%S/%S/%S", mg_user_data->music_directory, path, mg_user_data->thumbnail_names[j]);
                    if (strchr(mg_user_data->thumbnail_names[j], '.') == NULL) {
                        //basename, try extensions
                        coverfile = webserver_find_image_file(coverfile);
                    }
                    if (sdslen(coverfile) > 0 && access(coverfile, F_OK ) == 0) { /* Flawfinder: ignore */
                        found = true;
                        break;
                    }
                    sdsclear(coverfile);
                }
            }
            if (found == false) {
                for (int j = 0; j < mg_user_data->coverimage_names_len; j++) {
                    coverfile = sdscatfmt(coverfile, "%S/%S/%S", mg_user_data->music_directory, path, mg_user_data->coverimage_names[j]);
                    if (strchr(mg_user_data->coverimage_names[j], '.') == NULL) {
                        //basename, try extensions
                        coverfile = webserver_find_image_file(coverfile);
                    }
                    if (sdslen(coverfile) > 0 && access(coverfile, F_OK ) == 0) { /* Flawfinder: ignore */
                        found = true;
                        break;
                    }
                    sdsclear(coverfile);
                }
            }
            if (found == true) {
                const char *mime_type = get_mime_type_by_ext(coverfile);
                MYMPD_LOG_DEBUG("Serving file %s (%s)", coverfile, mime_type);
                static struct mg_http_serve_opts s_http_server_opts;
                s_http_server_opts.root_dir = mg_user_data->browse_directory;
                s_http_server_opts.extra_headers = EXTRA_HEADERS_CACHE;
                s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
                mg_http_serve_file(nc, hm, coverfile, &s_http_server_opts);
                webserver_handle_connection_close(nc);
                FREE_SDS(uri_decoded);
                FREE_SDS(coverfile);
                FREE_SDS(mediafile);
                FREE_SDS(path);
                return true;
            }

            FREE_SDS(coverfile);
            MYMPD_LOG_DEBUG("No cover file found in music directory");
            FREE_SDS(path);
        }

        if (access(mediafile, F_OK) == 0) { /* Flawfinder: ignore */
            //try to extract albumart from media file
            bool rc = handle_coverextract(nc, config, uri_decoded, mediafile, mg_user_data->covercache, offset);
            if (rc == true) {
                FREE_SDS(uri_decoded);
                FREE_SDS(mediafile);
                return true;
            }
        }
        FREE_SDS(mediafile);
    }

    //ask mpd - mpd can read only first image
    if (mg_user_data->feat_mpd_albumart == true &&
        offset == 0)
    {
        MYMPD_LOG_DEBUG("Sending getalbumart to mpd_client_queue");
        struct t_work_request *request = create_request(conn_id, 0, INTERNAL_API_ALBUMART, NULL);
        request->data = tojson_char(request->data, "uri", uri_decoded, false);
        request->data = sdscatlen(request->data, "}}", 2);
        mympd_queue_push(mympd_api_queue, request, 0);
        FREE_SDS(uri_decoded);
        return false;
    }

    MYMPD_LOG_INFO("No coverimage found for \"%s\"", uri_decoded);
    FREE_SDS(uri_decoded);
    webserver_serve_na_image(nc, hm);
    return true;
}

//privat functions
static bool handle_coverextract(struct mg_connection *nc, struct t_config *config,
        const char *uri, const char *media_file, bool covercache, int offset)
{
    bool rc = false;
    const char *mime_type_media_file = get_mime_type_by_ext(media_file);
    MYMPD_LOG_DEBUG("Handle coverextract for uri \"%s\"", uri);
    MYMPD_LOG_DEBUG("Mimetype of %s is %s", media_file, mime_type_media_file);
    sds binary = sdsempty();
    if (strcmp(mime_type_media_file, "audio/mpeg") == 0) {
        rc = handle_coverextract_id3(config, uri, media_file, &binary, covercache, offset);
    }
    else if (strcmp(mime_type_media_file, "audio/ogg") == 0) {
        rc = handle_coverextract_flac(config, uri, media_file, &binary, true, covercache, offset);
    }
    else if (strcmp(mime_type_media_file, "audio/flac") == 0) {
        rc = handle_coverextract_flac(config, uri, media_file, &binary, false, covercache, offset);
    }
    if (rc == true) {
        const char *mime_type = get_mime_type_by_magic_stream(binary);
        MYMPD_LOG_DEBUG("Serving coverimage for \"%s\" (%s)", media_file, mime_type);
        sds headers = sdscatfmt(sdsempty(), "Content-Type: %s\r\n", mime_type);
        headers = sdscat(headers, EXTRA_HEADERS_CACHE);
        webserver_send_data(nc, binary,  sdslen(binary), headers);
        FREE_SDS(headers);
    }
    FREE_SDS(binary);
    return rc;
}

static bool handle_coverextract_id3(struct t_config *config, const char *uri, const char *media_file,
        sds *binary, bool covercache, int offset)
{
    bool rc = false;
    #ifdef ENABLE_LIBID3TAG
    MYMPD_LOG_DEBUG("Exctracting coverimage from %s", media_file);
    struct id3_file *file_struct = id3_file_open(media_file, ID3_FILE_MODE_READONLY);
    if (file_struct == NULL) {
        MYMPD_LOG_ERROR("Can't parse id3_file: %s", media_file);
        return false;
    }
    struct id3_tag *tags = id3_file_tag(file_struct);
    if (tags == NULL) {
        MYMPD_LOG_ERROR("Can't read id3 tags from file: %s", media_file);
        return false;
    }
    struct id3_frame *frame = id3_tag_findframe(tags, "APIC", (unsigned)offset);
    if (frame != NULL) {
        id3_length_t length = 0;
        const id3_byte_t *pic = id3_field_getbinarydata(id3_frame_field(frame, 4), &length);
        if (length > 0) {
            *binary = sdscatlen(*binary, pic, length);
            const char *mime_type = get_mime_type_by_magic_stream(*binary);
            if (mime_type != NULL) {
                if (covercache == true) {
                    covercache_write_file(config->cachedir, uri, mime_type, *binary, offset);
                }
                else {
                    MYMPD_LOG_DEBUG("Covercache is disabled");
                }
                MYMPD_LOG_DEBUG("Coverimage successfully extracted (%lu bytes)", (unsigned long)sdslen(*binary));
                rc = true;
            }
            else {
                MYMPD_LOG_WARN("Could not determine mimetype, discarding image");
                sdsclear(*binary);
            }
        }
        else {
            MYMPD_LOG_WARN("Embedded picture size is zero");
        }
    }
    else {
        MYMPD_LOG_DEBUG("No embedded picture detected");
    }
    id3_file_close(file_struct);
    #else
    (void) config;
    (void) uri;
    (void) media_file;
    (void) binary;
    (void) covercache;
    (void) offset;
    #endif
    return rc;
}

static bool handle_coverextract_flac(struct t_config *config, const char *uri, const char *media_file,
        sds *binary, bool is_ogg, bool covercache, int offset)
{
    bool rc = false;
    #ifdef ENABLE_FLAC
    MYMPD_LOG_DEBUG("Exctracting coverimage from %s", media_file);
    FLAC__StreamMetadata *metadata = NULL;

    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();

    if(! (is_ogg? FLAC__metadata_chain_read_ogg(chain, media_file) : FLAC__metadata_chain_read(chain, media_file)) ) {
        MYMPD_LOG_DEBUG("%s: ERROR: reading metadata", media_file);
        FLAC__metadata_chain_delete(chain);
        return false;
    }

    FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
    FLAC__metadata_iterator_init(iterator, chain);
    assert(iterator);
    int i = 0;
    do {
        FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
        if (block->type == FLAC__METADATA_TYPE_PICTURE) {
            if (i == offset) {
                metadata = block;
                break;
            }
            i++;
        }
    } while (FLAC__metadata_iterator_next(iterator) && metadata == NULL);

    if (metadata == NULL) {
        MYMPD_LOG_DEBUG("No embedded picture detected");
    }
    else if (metadata->data.picture.data_length > 0) {
        *binary = sdscatlen(*binary, metadata->data.picture.data, metadata->data.picture.data_length);
        const char *mime_type = get_mime_type_by_magic_stream(*binary);
        if (mime_type != NULL) {
            if (covercache == true) {
                covercache_write_file(config->cachedir, uri, mime_type, *binary, offset);
            }
            else {
                MYMPD_LOG_DEBUG("Covercache is disabled");
            }
            MYMPD_LOG_DEBUG("Coverimage successfully extracted (%lu bytes)", (unsigned long)sdslen(*binary));
            rc = true;
        }
        else {
            MYMPD_LOG_WARN("Could not determine mimetype, discarding image");
            sdsclear(*binary);
        }
    }
    else {
        MYMPD_LOG_WARN("Embedded picture size is zero");
    }
    FLAC__metadata_iterator_delete(iterator);
    FLAC__metadata_chain_delete(chain);
    #else
    (void) config;
    (void) uri;
    (void) media_file;
    (void) binary;
    (void) is_ogg;
    (void) covercache;
    (void) offset;
    #endif
    return rc;
}
