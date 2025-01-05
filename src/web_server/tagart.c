/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief tagart functions
 */

#include "compile_time.h"
#include "src/web_server/tagart.h"

#include "src/lib/api.h"
#include "src/lib/config_def.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"
#include "src/web_server/placeholder.h"

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
        webserver_redirect_placeholder_image(nc, PLACEHOLDER_NA);
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
        webserver_serve_file(nc, hm, mg_user_data->browse_directory, EXTRA_HEADERS_IMAGE, mediafile);
        FREE_SDS(mediafile);
        FREE_SDS(tag);
        FREE_SDS(value);
        return true;
    }
    FREE_SDS(mediafile);

    #ifdef MYMPD_ENABLE_LUA
        //forward request to mympd_api thread
        MYMPD_LOG_DEBUG(NULL, "Sending INTERNAL_API_TAGART to mympdapi_queue");
        struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, nc->id, 0, INTERNAL_API_TAGART, NULL, MPD_PARTITION_DEFAULT);
        request->data = tojson_sds(request->data, "tag", tag, true);
        request->data = tojson_sds(request->data, "value", value, false);
        request->data = jsonrpc_end(request->data);
        mympd_queue_push(mympd_api_queue, request, 0);
        FREE_SDS(tag);
        FREE_SDS(value);
        return false;
    #else
        MYMPD_LOG_DEBUG(NULL, "No image for tag found");
        webserver_redirect_placeholder_image(nc, PLACEHOLDER_NA);
        FREE_SDS(tag);
        FREE_SDS(value);
        return true;
    #endif
}
