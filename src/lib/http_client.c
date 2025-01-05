/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Synchronous HTTP client
 */

#include "compile_time.h"
#include "src/lib/http_client.h"

#include "dist/mongoose/mongoose.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/mg_str_utils.h"
#include "src/lib/sds_extras.h"

#include <errno.h>
#include <inttypes.h>

/**
 * Private definitions
 */
static void http_client_ev_handler(struct mg_connection *nc, int ev, void *ev_data);

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
    int nread = 0;
    while ((line = sds_getline(line, fp, LINE_LENGTH_MAX, &nread)) && nread >= 0) {
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
 * Initializes a http response struct for the HTTP client
 * @param mg_client_response pointer to mg_client response struct
 */
void http_client_response_init(struct mg_client_response_t *mg_client_response) {
    mg_client_response->response_code = 0;
    mg_client_response->body = sdsempty();
    list_init(&mg_client_response->header);
    mg_client_response->rc = -1;
}

/**
 * Clears a http response struct for the HTTP client
 * @param mg_client_response pointer to mg_client response struct
 */
void http_client_response_clear(struct mg_client_response_t *mg_client_response) {
    FREE_SDS(mg_client_response->body);
    list_clear(&mg_client_response->header);
}

/**
 * Sends a HTTP request and follows redirects.
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
    MYMPD_LOG_DEBUG(NULL, "HTTP client setting dns server to %s", dns_uri);
    mgr_client.dns4.url = dns_uri;

    mgr_client.userdata = mg_client_request;
    mg_client_request->connect_uri = sdsnew(mg_client_request->uri);
    for (int i = 0; i < 10; i++) {
        MYMPD_LOG_DEBUG(NULL, "HTTP client connecting to \"%s\"", mg_client_request->connect_uri);
        mg_http_connect(&mgr_client, mg_client_request->connect_uri, http_client_ev_handler, mg_client_response);
        while (mg_client_response->rc == -1) {
            mg_mgr_poll(&mgr_client, 1000);
        }
        if (mg_client_response->response_code >= 300 &&
            mg_client_response->response_code < 400)
        {
            // follow redirects
            struct t_list_node *location = list_get_node(&mg_client_response->header, "location");
            if (location == NULL) {
                break;
            }
            sds last_host = sdsdup(mg_client_request->connect_uri);
            sdsclear(mg_client_request->connect_uri);
            if (strncmp(location->value_p, "http://", 7) != 0 &&
                strncmp(location->value_p, "https://", 8) != 0)
            {
                // redirect uri without host, keep last host part
                int k = 0;
                for (size_t j = 0; j < sdslen(last_host); j++) {
                    if (last_host[j] == '/') {
                        k++;
                    }
                    if (k == 3) {
                        break;
                    }
                    mg_client_request->connect_uri = sds_catchar(mg_client_request->connect_uri, last_host[j]);
                }
            }
            FREE_SDS(last_host);
            mg_client_request->connect_uri = sdscatsds(mg_client_request->connect_uri, location->value_p);
            list_clear(&mg_client_response->header);
            sdsclear(mg_client_response->body);
            mg_client_response->rc = -1;
            mg_client_response->response_code = 0;
        }
        else {
            break;
        }
    }
    FREE_SDS(dns_uri);
    FREE_SDS(mg_client_request->connect_uri);
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
 */
static void http_client_ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    struct mg_client_request_t *mg_client_request = (struct mg_client_request_t *) nc->mgr->userdata;
    if (ev == MG_EV_CONNECT) {
        //Connected to server. Extract host name from URL
        struct mg_str host = mg_url_host(mg_client_request->connect_uri);

        //If uri is https://, tell client connection to use TLS
        if (mg_url_is_ssl(mg_client_request->connect_uri)) {
            struct mg_tls_opts tls_opts = {
                .name = host
            };
            mg_tls_init(nc, &tls_opts);
        }

        //Send request
        if (mg_client_request->post_data != NULL &&
            strlen(mg_client_request->post_data) > 0)
        {
            MYMPD_LOG_DEBUG(NULL, "HTTP client sending data: \"%s\"", mg_client_request->post_data);
            mg_printf(nc,
                "%s %s HTTP/1.1\r\n"
                "Host: %.*s\r\n"
                "%s"
                "Content-Length: %lu\r\n"
                "Accept: */*\r\n"
                "Accept-Encoding: none\r\n"
                "User-Agent: myMPD/"MYMPD_VERSION" (https://github.com/jcorporation/myMPD)\r\n"
                "\r\n"
                "%s",
                mg_client_request->method,
                mg_url_uri(mg_client_request->connect_uri),
                (int) host.len, host.buf,
                mg_client_request->extra_headers,
                strlen(mg_client_request->post_data),
                mg_client_request->post_data);
        }
        else {
            mg_printf(nc,
                "%s %s HTTP/1.1\r\n"
                "Host: %.*s\r\n"
                "%s"
                "Accept: */*\r\n"
                "Accept-Encoding: none\r\n"
                "User-Agent: myMPD/"MYMPD_VERSION" (https://github.com/jcorporation/myMPD)\r\n"
                "\r\n",
                mg_client_request->method,
                mg_url_uri(mg_client_request->connect_uri),
                (int) host.len, host.buf,
                mg_client_request->extra_headers);
        }
    }
    else if (ev == MG_EV_HTTP_MSG) {
        //Response is received. Return it
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        struct mg_client_response_t *mg_client_response = (struct mg_client_response_t *) nc->fn_data;
        unsigned content_length = 0;
        mg_client_response->body = sdscatlen(mg_client_response->body, hm->body.buf, hm->body.len);
        //headers list
        sds name = sdsempty();
        for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++) {
            if (hm->headers[i].name.len == 0) {
                break;
            }
            name = sdscatlen(name, hm->headers[i].name.buf, hm->headers[i].name.len);
            sdstolower(name);
            if (strcmp(name, "content-length") == 0) {
                content_length = mg_str_to_uint(&hm->headers[i].value);
            }
            list_push_len(&mg_client_response->header, name, sdslen(name), 0, hm->headers[i].value.buf, hm->headers[i].value.len, NULL);
            sdsclear(name);
        }
        FREE_SDS(name);
        //http response code
        mg_client_response->response_code = mg_str_to_int(&hm->uri);
        //set response code
        if (content_length > 0 &&
            content_length != hm->body.len)
        {
            mg_client_response->rc = 1;
            MYMPD_LOG_ERROR(NULL, "HTTP client response code \"%d\"", mg_client_response->response_code);
            MYMPD_LOG_ERROR(NULL, "HTTP client invalid response size, received %lu bytes, expected %u bytes",
                    (unsigned long)hm->body.len, content_length);
        }
        else if (mg_client_response->response_code > 399) {
            mg_client_response->rc = 1;
            MYMPD_LOG_ERROR(NULL, "HTTP client response code \"%d\"", mg_client_response->response_code);
        }
        else {
            mg_client_response->rc = 0;
            MYMPD_LOG_INFO(NULL, "HTTP client response code \"%d\"", mg_client_response->response_code);
        }
        //Tell mongoose to close this connection
        nc->is_draining = 1;
    }
    else if (ev == MG_EV_ERROR) {
        struct mg_client_response_t *mg_client_response = (struct mg_client_response_t *) nc->fn_data;
        mg_client_response->body = sdscat(mg_client_response->body, "HTTP connection failed");
        mg_client_response->rc = 2;
        MYMPD_LOG_ERROR(NULL, "HTTP client connection to \"%s\" failed", mg_client_request->connect_uri);
    }
}
