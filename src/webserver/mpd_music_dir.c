/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD music directory functions
 */

#include "src/webserver/mpd_music_dir.h"
#include "compile_time.h"

#include "dist/sds/sds.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds/sds_extras.h"
#include "src/lib/sds/sds_url.h"
#include "src/lib/validate.h"
#include "src/webserver/mg_user_data.h"
#include "src/webserver/placeholder.h"
#include "src/webserver/response.h"

/**
 * Serves a file from a folder in the MPD music directory
 * @param nc mongoose connection
 * @param hm http message
 * @return true if a file was served, else false
 */
bool request_handler_mpd_music_directory(struct mg_connection *nc, struct mg_http_message *hm) {
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *)nc->mgr->userdata;
    if (sdslen(mg_user_data->music_directory) == 0) {
        webserver_send_error(nc, 503, "MPD Music directory is not available.");
        return false;
    }

    sds file_path = sds_urldecode(sdsempty(), hm->uri.buf, hm->uri.len, false);
    if (sdslen(file_path) == 0) {
        MYMPD_LOG_ERROR(NULL, "Failure decoding file path \"%.*s\".", (int)hm->uri.len, hm->uri.buf);
        FREE_SDS(file_path);
        webserver_send_error(nc, 500, "Internal server error.");
        return true;
    }
    sdsrange(file_path, 6, -1); // Strip /music prefix
    sds music_dir_file = sdscatfmt(sdsempty(), "%S%S", mg_user_data->music_directory, file_path);
    FREE_SDS(file_path);

    if (vcb_isfilepath(music_dir_file) == true &&
        is_file(music_dir_file, false) == true)
    {
        MYMPD_LOG_DEBUG(NULL, "Attempt to serve file \"%s\".", music_dir_file);
        webserver_serve_file(nc, hm, EXTRA_HEADERS_IMAGE, music_dir_file);
        FREE_SDS(music_dir_file);
        return true;
    }

    MYMPD_LOG_ERROR(NULL, "File \"%s\" not found.", music_dir_file);
    if (is_image(music_dir_file) == true) {
        webserver_redirect_placeholder_image(nc, PLACEHOLDER_NA);
    }
    else {
        webserver_send_error(nc, 404, "File not found.");
    }
    FREE_SDS(music_dir_file);
    return true;
}
