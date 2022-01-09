/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "http_client.h"

#include "../../dist/mongoose/mongoose.h"
#include "log.h"
#include "sds_extras.h"

#include <errno.h>

//private definitions
static void _http_client_ev_handler(struct mg_connection *nc, int ev, void *ev_data,
    void *fn_data);

//public functions
sds get_dnsserver(void) {
    //read resolv.conf directly - musl does not support res_init
    sds buffer = sdsempty();
    errno = 0;
    FILE *fp = fopen("/etc/resolv.conf", OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_WARN("Can not open /etc/resolv.conf");
        MYMPD_LOG_ERRNO(errno);
        return buffer;
    }
    sds line = sdsempty();
    sds nameserver = sdsempty();
    while (sds_getline(&line, fp, 1000) == 0) {
        if (sdslen(line) > 10 && strncmp(line, "nameserver", 10) == 0 && isspace(line[10])) {
            char *p;
            char *z;
            for (p = line + 11; isspace(*p); p++) {
                //skip blank chars
            }
            for (z = p; *z != '\0' && (isdigit(*z) || *z == '.'); z++) {
                nameserver = sdscatfmt(nameserver, "%c", *z);
            }
            struct sockaddr_in sa;
            if (inet_pton(AF_INET, nameserver, &(sa.sin_addr)) == 1) {
                //valid ipv4 address
                break;
            }
            MYMPD_LOG_DEBUG("Skipping invalid nameserver entry in resolv.conf");
            sdsclear(nameserver);
        }
    }
    FREE_SDS(line);
    fclose(fp);
    if (sdslen(nameserver) > 0) {
        buffer = sdscatfmt(buffer, "udp://%s:53", nameserver);
    }
    else {
        MYMPD_LOG_WARN("No valid nameserver found");
        buffer = sdscat(buffer, "udp://8.8.8.8:53");
    }
    FREE_SDS(nameserver);
    return buffer;
}

void http_client_request(struct mg_client_request_t *mg_client_request,
    struct mg_client_response_t *mg_client_response)
{
    struct mg_mgr mgr_client;
    mg_mgr_init(&mgr_client);
    #ifdef DEBUG
    mg_log_set("1");
    #endif
    //set dns server
    sds dns_uri = get_dnsserver();
    MYMPD_LOG_DEBUG("Setting dns server to %s", dns_uri);
    mgr_client.dns4.url = dns_uri;

    mgr_client.userdata = mg_client_request;
    MYMPD_LOG_DEBUG("HTTP client connecting to \"%s\"", mg_client_request->uri);
    mg_http_connect(&mgr_client, mg_client_request->uri, _http_client_ev_handler, mg_client_response);
    while (mg_client_response->rc == -1) {
        mg_mgr_poll(&mgr_client, 1000);
    }
    FREE_SDS(dns_uri);
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
                "Content-length: %lu\r\n"
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
            mg_client_response->header = sdscatlen(mg_client_response->header, hm->headers[i].name.ptr, hm->headers[i].name.len);
            mg_client_response->header = sdscatlen(mg_client_response->header, ": ", 2);
            mg_client_response->header = sdscatlen(mg_client_response->header, hm->headers[i].value.ptr, hm->headers[i].value.len);
            mg_client_response->header = sdscatlen(mg_client_response->header, "\n", 1);
        }
        //response code line
        for (unsigned i = 0; i < hm->message.len; i++) {
            if (hm->message.ptr[i] == '\n') {
                break;
            }
            if (isprint(hm->message.ptr[i])) {
                mg_client_response->response = sdscatfmt(mg_client_response->response,
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
