/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Folderart functions
 */

#include "compile_time.h"
#include "src/webserver/folderart.h"

#include "src/lib/api.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/validate.h"
#include "src/webserver/placeholder.h"

/**
 * Serves the first image in a folder
 * @param nc mongoose connection
 * @param hm http message
 * @param mg_user_data pointer to mongoose configuration
 * @return true if an image was found, else false
 */
bool request_handler_folderart(struct mg_connection *nc, struct mg_http_message *hm,
        struct t_mg_user_data *mg_user_data)
{
    if (sdslen(mg_user_data->music_directory) == 0) {
        webserver_redirect_placeholder_image(nc, PLACEHOLDER_NA);
        return false;
    }

    sds path = get_uri_param(&hm->query, "path=");

    if (path == NULL ||
        sdslen(path) == 0 ||
        vcb_isfilepath(path) == false)
    {
        MYMPD_LOG_ERROR(NULL, "Failed to decode query");
        webserver_redirect_placeholder_image(nc, PLACEHOLDER_FOLDER);
        FREE_SDS(path);
        return false;
    }
    sds coverfile = sdsempty();
    bool found = find_image_in_folder(&coverfile, mg_user_data->music_directory, path, mg_user_data->thumbnail_names, mg_user_data->thumbnail_names_len) ||
        find_image_in_folder(&coverfile, mg_user_data->music_directory, path, mg_user_data->coverimage_names, mg_user_data->coverimage_names_len);

    if (found == true) {
        webserver_serve_file(nc, hm, EXTRA_HEADERS_IMAGE, coverfile);
        FREE_SDS(path);
        FREE_SDS(coverfile);
        return true;
    }
    FREE_SDS(coverfile);

    #ifdef MYMPD_ENABLE_LUA
        //forward request to mympd_api thread
        MYMPD_LOG_DEBUG(NULL, "Sending INTERNAL_API_FOLDERART to mympdapi_queue");
        struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, nc->id, 0, INTERNAL_API_FOLDERART, NULL, MPD_PARTITION_DEFAULT);
        request->data = tojson_sds(request->data, "path", path, true);
        request->data = jsonrpc_end(request->data);
        mympd_queue_push(mympd_api_queue, request, 0);
        FREE_SDS(path);
        return false;
    #else
        MYMPD_LOG_INFO(NULL, "No folderimage found for \"%s\"", path);
        FREE_SDS(path);
        webserver_redirect_placeholder_image(nc, PLACEHOLDER_FOLDER);
        return true;
    #endif
}
