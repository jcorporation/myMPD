/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>

#include "../dist/src/sds/sds.h"
#include "../dist/src/mongoose/mongoose.h"

#include "log.h"
#include "http_client.h"

//private definitions
static void _http_client_ev_handler(struct mg_connection *nc, int ev, void *ev_data, 
    void *fn_data);

//public functions
sds get_dnsserver(void) {
    sds buffer = sdsempty();
    if (res_init() != 0) {
        MYMPD_LOG_WARN("Can not initialize dns server structure");
        return buffer;
    }
    if (_res.nscount <= 0) {
        MYMPD_LOG_WARN("No dns servers found");
        return buffer;
    }
    char ip_string[INET_ADDRSTRLEN] = "";
    if (inet_ntop(AF_INET, &_res.nsaddr_list[0].sin_addr.s_addr, ip_string, INET_ADDRSTRLEN) != NULL) {
        buffer = sdscatprintf(buffer, "udp://%s:%d", 
            ip_string, 
            ntohs(_res.nsaddr_list[0].sin_port)
        );
    }
    return buffer;
}

void http_client_request(struct mg_client_request_t *mg_client_request, 
    struct mg_client_response_t *mg_client_response)
{
    struct mg_mgr mgr_client;
    mg_mgr_init(&mgr_client);
    mg_log_set("1");
    //set dns server
    sds dns_uri = get_dnsserver();
    if (strlen(dns_uri) == 0) {
        MYMPD_LOG_WARN("Error reading dns server settings");
        dns_uri = sdscat(dns_uri, "udp://8.8.8.8:53");
    }
    MYMPD_LOG_DEBUG("Setting dns server to %s", dns_uri);
    mgr_client.dns4.url = dns_uri;
    
    mgr_client.userdata = mg_client_request;
    MYMPD_LOG_DEBUG("HTTP client connecting to \"%s\"", mg_client_request->uri);
    mg_http_connect(&mgr_client, mg_client_request->uri, _http_client_ev_handler, mg_client_response);
    while (mg_client_response->rc == -1) {
        mg_mgr_poll(&mgr_client, 1000);
    }
    sdsfree(dns_uri);
    mg_mgr_free(&mgr_client);
}

//private functions
static void _http_client_ev_handler(struct mg_connection *nc, int ev, void *ev_data, 
    void *fn_data)
{
    struct mg_client_request_t *mg_client_request = (struct mg_client_request_t *) nc->mgr->userdata;
    if (ev == MG_EV_CONNECT) {
        // Connected to server. Extract host name from URL
        struct mg_str host = mg_url_host(mg_client_request->uri);

        // If s_url is https://, tell client connection to use TLS
        if (mg_url_is_ssl(mg_client_request->uri)) {
            struct mg_tls_opts tls_opts = {
                .srvname = host
            };
            mg_tls_init(nc, &tls_opts);
        }

        //Send request
        MYMPD_LOG_DEBUG("Sending data: \"%s\"", mg_client_request->post_data);
        if (strcmp(mg_client_request->method, "POST") == 0) {
            mg_printf(nc,
                "POST %s HTTP/1.0\r\n"
                "Host: %.*s\r\n"
                "%s"
                "Content-length: %ul\r\n"
                "\r\n"
                "%s\r\n",
                mg_url_uri(mg_client_request->uri),
                (int) host.len, host.ptr, 
                mg_client_request->extra_headers,
                strlen(mg_client_request->post_data),
                mg_client_request->post_data);
        }
        else {
            mg_printf(nc,
                "GET %s HTTP/1.0\r\n"
                "Host: %.*s\r\n"
                "%s"
                "\r\n",
                mg_url_uri(mg_client_request->uri),
                (int) host.len, host.ptr,
                mg_client_request->extra_headers);
        }
    } 
    else if (ev == MG_EV_HTTP_MSG) {
        //Response is received. Return it
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        struct mg_client_response_t *mg_client_response = (struct mg_client_response_t *) fn_data;
        mg_client_response->body = sdscatlen(mg_client_response->body, hm->body.ptr, hm->body.len);
        //headers string
        for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++) {
            if (hm->headers[i].name.len == 0) {
                break;
            }
            mg_client_response->header = sdscatprintf(mg_client_response->header, "%.*s: %.*s\n", 
                (int) hm->headers[i].name.len, hm->headers[i].name.ptr,
                (int) hm->headers[i].value.len, hm->headers[i].value.ptr);
        }
        //response code line
        for (unsigned i = 0; i <  hm->message.len; i++) {
            if (hm->message.ptr[i] == '\n') {
                break;
            }
            if (isprint(hm->message.ptr[i])) {
                mg_client_response->response = sdscatprintf(mg_client_response->response, 
                    "%c", hm->message.ptr[i]);
            }
        }
        //set response code
        if (strncmp("HTTP/1.1 200", mg_client_response->response, 12) == 0) {;
            mg_client_response->rc = 0;
        }
        else {
            mg_client_response->rc = 1;
        }
        MYMPD_LOG_DEBUG("HTTP client received response \"%s\"", mg_client_response->response);
        MYMPD_LOG_DEBUG("HTTP client received body \"%s\"", mg_client_response->body);
        //Tell mongoose to close this connection
        nc->is_closing = 1;
    } 
    else if (ev == MG_EV_ERROR) {
        struct mg_client_response_t *mg_client_response = (struct mg_client_response_t *) fn_data;
        mg_client_response->body = sdscat(mg_client_response->body, "HTTP connection failed");
        mg_client_response->rc = 2;
        MYMPD_LOG_ERROR("HTTP connection to \"%s\" failed", mg_client_request->uri);
    }
}
