/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Synchronous HTTP client
 */

#ifndef MYMPD_HTTP_CLIENT_H
#define MYMPD_HTTP_CLIENT_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"

/**
 * Defines a http request
 */
struct mg_client_request_t {
    const char *method;        //!< http method (e.g. GET, POST)
    const char *uri;           //!< full uri to connect
    const char *extra_headers; //!< headers for the request
    const char *post_data;     //!< optional already encoded post data
    sds connect_uri;           //!< redirect uri (only internal)
};

/**
 * Defines a http response
 */
struct mg_client_response_t {
    int rc;                //!< return code, 0 = success
    int response_code;     //!< http response code
    struct t_list header;  //!< response header
    sds body;              //!< response body
};

sds get_dnsserver(void);
void http_client_response_init(struct mg_client_response_t *mg_client_response);
void http_client_response_clear(struct mg_client_response_t *mg_client_response);
void http_client_request(struct mg_client_request_t *mg_client_request,
    struct mg_client_response_t *mg_client_response);

#endif
