/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/tagart.h"

#include "src/lib/config_def.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"
#include "src/lib/validate.h"

/**
 * Request handler for /tagart
 * @param nc mongoose connection
 * @param hm http message
 * @param mg_user_data webserver configuration
 * @return true on success, else false
 */
bool request_handler_tagart(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data)
{
    struct t_config *config = mg_user_data->config;
    sds query = sdsnewlen(hm->query.ptr, hm->query.len);
    //remove uri=
    sdsrange(query, 4, -1);
    //decode uri
    sds uri_decoded = sds_urldecode(sdsempty(), query, sdslen(query), false);
    FREE_SDS(query);
    if (sdslen(uri_decoded) == 0) {
        MYMPD_LOG_ERROR("Failed to decode uri");
        webserver_serve_na_image(nc);
        FREE_SDS(uri_decoded);
        return true;
    }
    if (vcb_isfilepath(uri_decoded) == false) {
        MYMPD_LOG_ERROR("Invalid URI: %s", uri_decoded);
        webserver_serve_na_image(nc);
        FREE_SDS(uri_decoded);
        return true;
    }
    MYMPD_LOG_DEBUG("Handle tagart for uri \"%s\"", uri_decoded);
    //create absolute file
    sds mediafile = sdscatfmt(sdsempty(), "%S/pics/%S", config->workdir, uri_decoded);
    MYMPD_LOG_DEBUG("Absolut media_file: %s", mediafile);
    mediafile = webserver_find_image_file(mediafile);
    if (sdslen(mediafile) > 0) {
        const char *mime_type = get_mime_type_by_ext(mediafile);
        MYMPD_LOG_DEBUG("Serving file %s (%s)", mediafile, mime_type);
        static struct mg_http_serve_opts s_http_server_opts;
        s_http_server_opts.root_dir = mg_user_data->browse_directory;
        s_http_server_opts.extra_headers = EXTRA_HEADERS_CACHE;
        s_http_server_opts.mime_types = EXTRA_MIME_TYPES;
        mg_http_serve_file(nc, hm, mediafile, &s_http_server_opts);
    }
    else {
        MYMPD_LOG_DEBUG("No image for tag found");
        webserver_serve_na_image(nc);
    }
    FREE_SDS(mediafile);
    FREE_SDS(uri_decoded);
    return true;
}
