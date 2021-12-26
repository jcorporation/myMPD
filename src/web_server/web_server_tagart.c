/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "web_server_tagart.h"

#include "../lib/log.h"
#include "../lib/mimetype.h"
#include "../lib/sds_extras.h"
#include "../lib/validate.h"

bool webserver_tagart_handler(struct mg_connection *nc, struct mg_http_message *hm,
                   struct t_mg_user_data *mg_user_data) {
    //decode uri
    sds uri_decoded = sds_urldecode(sdsempty(), hm->uri.ptr, (int)hm->uri.len, 0);
    if (sdslen(uri_decoded) == 0) {
        MYMPD_LOG_ERROR("Failed to decode uri");
        webserver_serve_na_image(nc, hm);
        FREE_SDS(uri_decoded);
        return true;
    }
    if (vcb_isfilepath(uri_decoded) == false) {
        MYMPD_LOG_ERROR("Invalid URI: %s", uri_decoded);
        webserver_serve_na_image(nc, hm);
        FREE_SDS(uri_decoded);
        return true;
    }
    MYMPD_LOG_DEBUG("Handle tagart for uri \"%s\"", uri_decoded);
    //create absolute file
    sdsrange(uri_decoded, 8, -1);
    sds mediafile = sdscatfmt(sdsempty(), "%s/%s", mg_user_data->pics_directory, uri_decoded);
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
        webserver_serve_na_image(nc, hm);
    }
    FREE_SDS(mediafile);
    FREE_SDS(uri_decoded);
    return true;
}
