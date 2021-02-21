/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <limits.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <libgen.h>

#include "../../dist/src/sds/sds.h"
#include "../../dist/src/mongoose/mongoose.h"
#include "../../dist/src/frozen/frozen.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../log.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "web_server_utility.h"
#include "web_server_tagpics.h"

//public functions

//returns true if lyrics are served
//returns false if waiting for mpd_client to handle request - not implemented yet
bool handle_tagpics(struct mg_connection *nc, struct http_message *hm, t_mg_user_data *mg_user_data, t_config *config, int conn_id) {
    //decode uri
    sds uri_decoded = sdsurldecode(sdsempty(), hm->uri.p, (int)hm->uri.len, 0);
    if (sdslen(uri_decoded) == 0) {
        MYMPD_LOG_ERROR("Failed to decode uri");
        serve_plaintext(nc, "Failed to decode uri");
        sdsfree(uri_decoded);
        return true;
    }
    if (validate_uri(uri_decoded) == false) {
        MYMPD_LOG_ERROR("Invalid URI: %s", uri_decoded);
        serve_plaintext(nc, "Invalid URI");
        sdsfree(uri_decoded);
        return true;
    }
    //remove /tagpics/
    sdsrange(uri_decoded, 9, -1);
    //create absolute file
    sds mediafile = sdscatfmt(sdsempty(), "%s/pics/%s", config->varlibdir, uri_decoded);
    sdsfree(uri_decoded);
    MYMPD_LOG_DEBUG("Absolut media_file: %s", mediafile);
    mediafile = find_image_file(mediafile);
    if (sdslen(mediafile) > 0) {
        sds mime_type = get_mime_type_by_ext(mediafile);
        MYMPD_LOG_DEBUG("Serving file %s (%s)", mediafile, mime_type);
        mg_http_serve_file(nc, hm, mediafile, mg_mk_str(mime_type), mg_mk_str(EXTRA_HEADERS_CACHE));
        sdsfree(mime_type);
    }
    else {
        serve_na_image(nc, hm);
    }

    sdsfree(mediafile);
    (void) conn_id;
    (void) mg_user_data;
    return true;
}
