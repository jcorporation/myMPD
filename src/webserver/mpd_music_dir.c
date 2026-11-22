/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief MPD music directory functions
 */

#include "compile_time.h"
#include "src/webserver/mpd_music_dir.h"

#include "dist/sds/sds.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
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
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    if (sdslen(mg_user_data->music_directory) == 0) {
        webserver_redirect_placeholder_image(nc, PLACEHOLDER_NA);
        return false;
    }

    sds file_path = sds_urldecode(sdsempty(), hm->uri.buf, hm->uri.len, false);
    sdsrange(file_path, 6, -1);  // Strip /music prefix
    if (sdslen(file_path) == 0) {
      MYMPD_LOG_ERROR(NULL, "Failure decoding file path \"%.*s\".", (int)hm->uri.len, hm->uri.buf);
      FREE_SDS(file_path);
    }
    sds coverfile = sdscatfmt(sdsempty(), "%S%S", mg_user_data->music_directory, file_path);
    FREE_SDS(file_path);

    if (sdslen(file_path) > 0 &&
        vcb_isfilepath(coverfile) == true &&
        is_file(coverfile, false) == true)
    {
        MYMPD_LOG_DEBUG(NULL, "Attempt to serve file \"%s\".", coverfile);
        webserver_serve_file(nc, hm, EXTRA_HEADERS_IMAGE, coverfile);
        FREE_SDS(coverfile);
        return true;
    }
    MYMPD_LOG_ERROR(NULL, "Picture \"%s\" not found.", coverfile);
    FREE_SDS(coverfile);
    webserver_redirect_placeholder_image(nc, PLACEHOLDER_FOLDER);
    return true;
}
