/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Placeholder image functions
 */

#include "compile_time.h"
#include "src/web_server/placeholder.h"

#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/web_server/utility.h"

/**
 * Placeholder image names
 */
const char *placeholder_image_names[] = {
    [PLACEHOLDER_BOOKLET] = "coverimage-booklet",
    [PLACEHOLDER_FOLDER] = "coverimage-folder",
    [PLACEHOLDER_MYMPD] = "coverimage-mympd",
    [PLACEHOLDER_NA] = "coverimage-notavailable",
    [PLACEHOLDER_PLAYLIST] = "coverimage-playlist",
    [PLACEHOLDER_SMARTPLS] = "coverimage-smartpls",
    [PLACEHOLDER_STREAM] = "coverimage-stream"
};

/**
 * Lookups the placeholder string
 * @param placeholder Placeholder
 * @return placeholder name
 */
const char* placeholder_lookup_name(enum placeholder_types placeholder) {
    if (placeholder >= 0 && placeholder < PLACEHOLDER_COUNT) {
        return placeholder_image_names[placeholder];
    }
    return placeholder_image_names[PLACEHOLDER_NA];
}

/**
 * Redirects to the placeholder image
 * @param nc mongoose connection
 * @param placeholder_type Type of placeholder image
 */
void webserver_redirect_placeholder_image(struct mg_connection *nc, enum placeholder_types placeholder_type) {
    sds placeholder_uri = sdscatfmt(sdsempty(), "/assets/%s", placeholder_lookup_name(placeholder_type));
    webserver_send_header_redirect(nc, placeholder_uri, "");
    FREE_SDS(placeholder_uri);
}

/**
 * Serves the placeholder image
 * @param nc mongoose connection
 * @param hm http message
 * @param uri placeholder uri
 */
void webserver_serve_placeholder_image(struct mg_connection *nc, struct mg_http_message *hm, sds uri) {
    struct t_mg_user_data *mg_user_data = (struct t_mg_user_data *) nc->mgr->userdata;
    if (uri[1] == 'a') {
        // Default placeholders
        #ifdef MYMPD_EMBEDDED_ASSETS
            webserver_serve_embedded_files(nc, uri);
        #else
            sds abs_uri = sdscatfmt(sdsempty(), "%s%S", MYMPD_DOC_ROOT, uri);
            webserver_serve_file(nc, hm, MYMPD_DOC_ROOT, abs_uri);
            FREE_SDS(abs_uri);
        #endif
    }
    else {
        // Custom placeholders
        webserver_serve_file(nc, hm, mg_user_data->config->workdir, uri);
    }
}

/**
 * Finds and sets the placeholder images
 * @param workdir myMPD working directory
 * @param name basename to search for
 * @param result pointer to sds result
 */
void get_placeholder_image(sds workdir, const char *name, sds *result) {
    sds file = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_PICS_THUMBS, name);
    MYMPD_LOG_DEBUG(NULL, "Check for custom placeholder image \"%s\"", file);
    file = webserver_find_image_file(file);
    sdsclear(*result);
    if (sdslen(file) > 0) {
        MYMPD_LOG_INFO(NULL, "Setting custom placeholder image for %s to \"%s\"", name, file);
        *result = sdscatsds(*result, file);
    }
    else {
        *result = sdscatfmt(*result, "/assets/%s.svg", name);
    }
    FREE_SDS(file);
}
