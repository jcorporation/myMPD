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
    sds query = sdsnewlen(hm->query.ptr, hm->query.len);
    sds uri_decoded = sdsempty();
    if (sdslen(query) > 4 &&
        strncmp(query, "playlist=", 9) == 0)
    {
        //remove playlist=
        sdsrange(query, 9, -1);
        uri_decoded = sds_urldecode(uri_decoded, query, sdslen(query), false);
    }
    FREE_SDS(query);
    if (sdslen(uri_decoded) == 0 ||
        vcb_isfilepath(uri_decoded) == false)
    {
        MYMPD_LOG_ERROR(NULL, "Failed to decode query");
        webserver_serve_placeholder_image(nc, PLACEHOLDER_PLAYLIST);
        FREE_SDS(uri_decoded);
        return false;
    }
    strip_file_extension(uri_decoded);
    sanitize_filename2(uri_decoded);

    MYMPD_LOG_DEBUG(NULL, "Handle playlistart for \"%s\"", uri_decoded);
    //create absolute filepath
    sds mediafile = sdscatfmt(sdsempty(), "%S/%s/%S", config->workdir, DIR_WORK_PICS_THUMBS, uri_decoded);
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
    FREE_SDS(uri_decoded);
    return true;
}
