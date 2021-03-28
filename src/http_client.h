/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__
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
