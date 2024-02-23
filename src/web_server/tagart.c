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
    sds query = sdsnewlen(hm->query.ptr, hm->query.len);
    sds tag = sdsempty();
    sds value = sdsempty();
    // remove 'tag='
    sdsrange(query, 4, -1);
    size_t query_len = sdslen(query);
    // get tagname
    size_t i = 0;
    while (query[i] != '&' &&
        i < query_len)
    {
        tag = sdscatfmt(tag, "%c", query[i]);
        i++;
    }
    // remove 'tagname&value='
    size_t to_remove = i + 7;
    if (query_len > to_remove) {
        sdsrange(query, (ssize_t)to_remove, -1);
        // rest is the tag value
        value = sds_urldecode(value, query, sdslen(query), false);
    }
    FREE_SDS(query);
    // verify
    if (vcb_ismpdtag(tag) == false ||
        sdslen(value) == 0)
    {
        MYMPD_LOG_ERROR(NULL, "Failed to decode query");
        webserver_serve_placeholder_image(nc, PLACEHOLDER_NA);
        FREE_SDS(tag);
        FREE_SDS(value);
        return true;
    }

    sanitize_filename2(value);

    MYMPD_LOG_DEBUG(NULL, "Handle tagart for \"%s\": \"%s\"", tag, value);
    //create absolute filepath
    sds mediafile = sdscatfmt(sdsempty(), "%S/%s/%S/%S", config->workdir, DIR_WORK_PICS, tag, value);
    MYMPD_LOG_DEBUG(NULL, "Absolut media_file: %s", mediafile);
    mediafile = webserver_find_image_file(mediafile);
    if (sdslen(mediafile) > 0) {
        webserver_serve_file(nc, hm, mg_user_data->browse_directory, mediafile);
    }
    else {
        MYMPD_LOG_DEBUG(NULL, "No image for tag found");
        webserver_serve_placeholder_image(nc, PLACEHOLDER_NA);
    }
    FREE_SDS(mediafile);
    FREE_SDS(tag);
    FREE_SDS(value);
    return true;
}
