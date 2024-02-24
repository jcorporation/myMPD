/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/playlistart.h"

#include "src/lib/config_def.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"

/**
 * Request handler for /playlistart
 * @param nc mongoose connection
 * @param hm http message
 * @param mg_user_data webserver configuration
 * @return true on success, else false
 */
bool request_handler_playlistart(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data)
{
    struct t_config *config = mg_user_data->config;
    sds name = get_uri_param(&hm->query, "playlist=");

    if (name == NULL ||
        sdslen(name) == 0 ||
        vcb_isfilepath(name) == false)
    {
        MYMPD_LOG_ERROR(NULL, "Failed to decode query");
        webserver_serve_placeholder_image(nc, PLACEHOLDER_PLAYLIST);
        FREE_SDS(name);
        return false;
    }
    strip_file_extension(name);
    sanitize_filename2(name);

    MYMPD_LOG_DEBUG(NULL, "Handle playlistart for \"%s\"", name);
    //create absolute filepath
    sds mediafile = sdscatfmt(sdsempty(), "%S/%s/%S", config->workdir, DIR_WORK_PICS_THUMBS, name);
    MYMPD_LOG_DEBUG(NULL, "Absolut media_file: %s", mediafile);
    mediafile = webserver_find_image_file(mediafile);
    if (sdslen(mediafile) > 0) {
        webserver_serve_file(nc, hm, mg_user_data->browse_directory, mediafile);
    }
    else {
        MYMPD_LOG_DEBUG(NULL, "No image for tag found");
        webserver_serve_placeholder_image(nc, PLACEHOLDER_PLAYLIST);
    }
    FREE_SDS(mediafile);
    FREE_SDS(name);
    return true;
}
