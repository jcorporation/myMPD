/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Webserver URL parser functions
 */

#ifndef MYMPD_WEB_SERVER_PARSER_H
#define MYMPD_WEB_SERVER_PARSER_H

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/list/list.h"

sds get_uri_param(struct mg_str *query, const char *name);
struct t_list *webserver_parse_arguments(struct mg_str *query);

#endif
