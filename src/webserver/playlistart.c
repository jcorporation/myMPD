/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Playlistart functions
 */

#include "compile_time.h"
#include "src/webserver/playlistart.h"

#include "src/lib/api.h"
#include "src/lib/config_def.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"
#include "src/webserver/placeholder.h"

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
    sds type = get_uri_param(&hm->query, "type=");

    if (name == NULL ||
        type == NULL ||
        sdslen(name) == 0 ||
        sdslen(type) == 0 ||
        vcb_isfilepath(name) == false)
    {
        MYMPD_LOG_ERROR(NULL, "Failed to decode query");
        webserver_redirect_placeholder_image(nc, PLACEHOLDER_PLAYLIST);
        FREE_SDS(name);
        FREE_SDS(type);
        return false;
    }
    strip_file_extension(name);
    sanitize_filename2(name);

    MYMPD_LOG_DEBUG(NULL, "Handle playlistart for \"%s\"", name);
    //create absolute filepath
    sds mediafile = sdscatfmt(sdsempty(), "%S/%s/%S", config->workdir, DIR_WORK_PICS_PLAYLISTS, name);
    MYMPD_LOG_DEBUG(NULL, "Absolut media_file: %s", mediafile);
    mediafile = webserver_find_image_file(mediafile);
    if (sdslen(mediafile) > 0) {
        webserver_serve_file(nc, hm, EXTRA_HEADERS_IMAGE, mediafile);
        FREE_SDS(mediafile);
        FREE_SDS(name);
        FREE_SDS(type);
        return true;
    }
    FREE_SDS(mediafile);

    #ifdef MYMPD_ENABLE_LUA
        //forward request to mympd_api thread
        MYMPD_LOG_DEBUG(NULL, "Sending INTERNAL_API_TAGART to mympdapi_queue");
        struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, nc->id, 0, INTERNAL_API_PLAYLISTART, NULL, MPD_PARTITION_DEFAULT);
        request->data = tojson_sds(request->data, "name", name, true);
        request->data = tojson_sds(request->data, "type", type, false);
        request->data = jsonrpc_end(request->data);
        mympd_queue_push(mympd_api_queue, request, 0);
        FREE_SDS(name);
        FREE_SDS(type);
        return false;
    #else
        MYMPD_LOG_DEBUG(NULL, "No image for playlist found");
        if (strcmp(type, "smartpls") == 0) {
            webserver_redirect_placeholder_image(nc, PLACEHOLDER_SMARTPLS);
        }
        else {
            webserver_redirect_placeholder_image(nc, PLACEHOLDER_PLAYLIST);
        }
        FREE_SDS(name);
        FREE_SDS(type);
        return true;
    #endif
}
