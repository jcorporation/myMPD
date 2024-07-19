/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver utility functions
 */

#ifndef MYMPD_WEB_SERVER_PLACEHOLDER_H
#define MYMPD_WEB_SERVER_PLACEHOLDER_H

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"

/**
 * Placeholder types
 */
enum placeholder_types {
    PLACEHOLDER_UNKNOWN = -1,
    PLACEHOLDER_BOOKLET,
    PLACEHOLDER_FOLDER,
    PLACEHOLDER_MYMPD,
    PLACEHOLDER_NA,
    PLACEHOLDER_PLAYLIST,
    PLACEHOLDER_SMARTPLS,
    PLACEHOLDER_STREAM,
    PLACEHOLDER_COUNT
};

const char* placeholder_lookup_name(enum placeholder_types placeholder);
void webserver_redirect_placeholder_image(struct mg_connection *nc, enum placeholder_types placeholder_type);
void webserver_serve_placeholder_image(struct mg_connection *nc, struct mg_http_message *hm, sds uri);
void get_placeholder_image(sds workdir, const char *name, sds *result);

#endif
