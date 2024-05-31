/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/tagart.h"

#include "src/lib/config_def.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
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
    sds tag = get_uri_param(&hm->query, "tag=");
    sds value = get_uri_param(&hm->query, "value=");

    if (tag == NULL ||
        value == NULL ||
        sdslen(value) == 0 ||
        vcb_ismpdtag(tag) == false)
    {
        MYMPD_LOG_ERROR(NULL, "Failed to decode query");
        webserver_serve_placeholder_image(nc, PLACEHOLDER_NA);
        FREE_SDS(tag);
        FREE_SDS(value);
        return true;
    }

    MYMPD_LOG_DEBUG(NULL, "Handle tagart for \"%s\": \"%s\"", tag, value);

    //check thumbs cache and serve image from it if found
    if (check_imagescache(nc, hm, mg_user_data, DIR_CACHE_THUMBS, value, 0) == true) {
        FREE_SDS(tag);
        FREE_SDS(value);
        return true;
    }

    //create absolute filepath
    sanitize_filename2(value);
    sds mediafile = sdscatfmt(sdsempty(), "%S/%s/%S/%S", config->workdir, DIR_WORK_PICS, tag, value);
    MYMPD_LOG_DEBUG(NULL, "Absolut media_file: %s", mediafile);
    mediafile = webserver_find_image_file(mediafile);
    if (sdslen(mediafile) > 0) {
        webserver_serve_file(nc, hm, mg_user_data->browse_directory, mediafile);
    }
    //TODO: implement trigger for tagart retrieval
    //      send request to mympd_api thread
    else {
        MYMPD_LOG_DEBUG(NULL, "No image for tag found");
        webserver_serve_placeholder_image(nc, PLACEHOLDER_NA);
    }
    FREE_SDS(mediafile);
    FREE_SDS(tag);
    FREE_SDS(value);
    return true;
}
