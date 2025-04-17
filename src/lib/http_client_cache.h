/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief HTTP client cache
 */

#ifndef MYMPD_HTTP_CLIENT_CACHE_H
#define MYMPD_HTTP_CLIENT_CACHE_H

#include "src/lib/config_def.h"

struct mg_client_response_t *http_client_cache_check(struct t_config *config, const char *uri);
struct mg_client_response_t *http_client_cache_read(const char *filepath);
bool http_client_cache_write(struct t_config *config, const char *uri, struct mg_client_response_t *mg_client_response);

#endif
