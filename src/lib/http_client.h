/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_HTTP_CLIENT_H
#define MYMPD_HTTP_CLIENT_H

#include "../../dist/sds/sds.h"

/**
 * Defines a http request
 */
struct mg_client_request_t {
    const char *method;        //!< http method (e.g. GET, POST)
    const char *uri;           //!< full uri to connect
    const char *extra_headers; //!< headers for the request
    const char *post_data;     //!< optional already encoded post data
};

/**
 * Defines a http response
 */
struct mg_client_response_t {
    int rc;       //!< return code, 0 = success
    sds response; //!< full http response
    sds header;   //!< response header
    sds body;     //!< response body
};

sds get_dnsserver(void);
void http_client_request(struct mg_client_request_t *mg_client_request,
    struct mg_client_response_t *mg_client_response);

#endif
