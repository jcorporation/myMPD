/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_HTTP_CLIENT_H
#define MYMPD_HTTP_CLIENT_H

#include "../../dist/sds/sds.h"

struct mg_client_request_t {
    const char *method;
    const char *uri;
    const char *extra_headers;
    const char *post_data;
};

struct mg_client_response_t {
    int rc;
    sds response;
    sds header;
    sds body;
};

sds get_dnsserver(void);
void http_client_request(struct mg_client_request_t *mg_client_request,
    struct mg_client_response_t *mg_client_response);

#endif
