/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/http_client.h"

#include "dist/mongoose/mongoose.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

#include <errno.h>
#include <inttypes.h>

/**
 * Private definitions
 */
static void http_client_ev_handler(struct mg_connection *nc, int ev, void *ev_data,
    void *fn_data);

/**
 * Public functions
 */

/**
 * Reads the dns server from resolv.conf
 * @return newly allocated sds string with first nameserver
 */
sds get_dnsserver(void) {
    //read resolv.conf directly - musl does not support res_init
    sds buffer = sdsempty();
    errno = 0;
    FILE *fp = fopen("/etc/resolv.conf", OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_WARN(NULL, "Can not open /etc/resolv.conf");
        MYMPD_LOG_ERRNO(NULL, errno);
        return buffer;
    }
    sds line = sdsempty();
    sds nameserver = sdsempty();
    while (sds_getline(&line, fp, LINE_LENGTH_MAX) >= 0) {
        if (sdslen(line) > 10 &&
            strncmp(line, "nameserver", 10) == 0 &&
            isspace(line[10]))
        {
            char *p;
            char *z;
            for (p = line + 11; isspace(*p); p++) {
                //skip blank chars
            }
            for (z = p; *z != '\0' && (isdigit(*z) || *z == '.'); z++) {
                nameserver = sds_catchar(nameserver, *z);
            }
            struct sockaddr_in sa;
            if (inet_pton(AF_INET, nameserver, &(sa.sin_addr)) == 1) {
                //valid ipv4 address
                break;
            }
            MYMPD_LOG_DEBUG(NULL, "Skipping invalid nameserver entry in resolv.conf");
            sdsclear(nameserver);
        }
    }
    FREE_SDS(line);
    (void) fclose(fp);
    if (sdslen(nameserver) > 0) {
        buffer = sdscatfmt(buffer, "udp://%S:53", nameserver);
    }
    else {
        MYMPD_LOG_WARN(NULL, "No valid nameserver found");
        buffer = sdscat(buffer, "udp://8.8.8.8:53");
    }
    FREE_SDS(nameserver);
    return buffer;
}

/**
 * Makes a http request
 * @param mg_client_request pointer to mg_client_request_t struct
 * @param mg_client_response pointer to mg_client_response_t struct to populate
 */
void http_client_request(struct mg_client_request_t *mg_client_request,
    struct mg_client_response_t *mg_client_response)
{
    struct mg_mgr mgr_client;
    mg_mgr_init(&mgr_client);
    mg_log_set(1);
    //set dns server
    sds dns_uri = get_dnsserver();
    MYMPD_LOG_DEBUG(NULL, "Setting dns server to %s", dns_uri);
    mgr_client.dns4.url = dns_uri;

    mgr_client.userdata = mg_client_request;
    MYMPD_LOG_DEBUG(NULL, "HTTP client connecting to \"%s\"", mg_client_request->uri);
    mg_http_connect(&mgr_client, mg_client_request->uri, http_client_ev_handler, mg_client_response);
    while (mg_client_response->rc == -1) {
        mg_mgr_poll(&mgr_client, 1000);
    }
    FREE_SDS(dns_uri);
    mg_mgr_free(&mgr_client);
}

/**
 * Private functions
 */

/**
 * Event handler for the http request made by http_client_request
 * @param nc mongoose network connection
 * @param ev event id
 * @param ev_data event data (http response)
 * @param fn_data struct mg_client_response
 */
static void http_client_ev_handler(struct mg_connection *nc, int ev, void *ev_data,
    void *fn_data)
{
    struct mg_client_request_t *mg_client_request = (struct mg_client_request_t *) nc->mgr->userdata;
    if (ev == MG_EV_CONNECT) {
        //Connected to server. Extract host name from URL
        struct mg_str host = mg_url_host(mg_client_request->uri);

        //If uri is https://, tell client connection to use TLS
        if (mg_url_is_ssl(mg_client_request->uri)) {
            struct mg_tls_opts tls_opts = {
                .srvname = host
            };
            mg_tls_init(nc, &tls_opts);
        }

        //Send request
        MYMPD_LOG_DEBUG(NULL, "Sending data: \"%s\"", mg_client_request->post_data);
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
        //http response code
        sds response_code = sdsnewlen(hm->uri.ptr, hm->uri.len);
        mg_client_response->response_code = (int)strtoimax(response_code, NULL, 10);
        FREE_SDS(response_code);
        //set response code
        mg_client_response->rc =  mg_client_response->response_code == 200 ? 0: 1;

        MYMPD_LOG_DEBUG(NULL, "HTTP client response code \"%d\"", mg_client_response->response_code);
        MYMPD_LOG_DEBUG(NULL, "HTTP client received body \"%s\"", mg_client_response->body);
        //Tell mongoose to close this connection
        nc->is_closing = 1;
    }
    else if (ev == MG_EV_ERROR) {
        struct mg_client_response_t *mg_client_response = (struct mg_client_response_t *) fn_data;
        mg_client_response->body = sdscat(mg_client_response->body, "HTTP connection failed");
        mg_client_response->rc = 2;
        MYMPD_LOG_ERROR(NULL, "HTTP connection to \"%s\" failed", mg_client_request->uri);
    }
}
