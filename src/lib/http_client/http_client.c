/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Synchronous HTTP client
 */

#include "compile_time.h"
#include "src/lib/http_client/http_client.h"

#include "dist/mongoose/mongoose.h"
#include "src/lib/log.h"
#include "src/lib/sds/sds_extras.h"
#include "src/lib/sds/sds_file.h"

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <strings.h>

/**
 * Private definitions
 */
static void http_client_ev_handler(struct mg_connection *nc, int ev, void *ev_data);
static sds resolve_redirect(sds connect_uri, sds redirect_uri);

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
    while (sds_getline(fp, LINE_LENGTH_MAX, &line) >= 0) {
        if (sdslen(line) > 10 &&
            strncmp(line, "nameserver", 10) == 0 &&
            isspace((unsigned char)line[10]))
        {
            char *p;
            char *z;
            for (p = line + 11; isspace((unsigned char)*p); p++) {
                //skip blank chars
            }
            for (z = p; *z != '\0' && (isdigit((unsigned char)*z) || *z == '.'); z++) {
                nameserver = sds_catchar(nameserver, *z);
            }
            struct sockaddr_in sa;
            if (inet_pton(AF_INET, nameserver, &sa.sin_addr) == 1) {
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
            mg_client_request->connect_uri = resolve_redirect(mg_client_request->connect_uri, location->value_p);
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
 * Resolve the redirect uri
 * @param connect_uri The original uri
 * @param redirect_uri The uri from the location header
 * @return Replaced connect_uri buffer
 */
static sds resolve_redirect(sds connect_uri, sds redirect_uri) {
    if (strncasecmp(redirect_uri, "http://", 7) == 0 ||
        strncasecmp(redirect_uri, "https://", 8) == 0)
    {
        //Full uri with scheme + host and path for the redirect
        sdsclear(connect_uri);
        return sdscat(connect_uri, redirect_uri);
    }

    // Get scheme and host from original uri
    const char *scheme = strstr(connect_uri, "://");
    if (scheme == NULL) {
        return sdscat(connect_uri, redirect_uri);
    }
    const char *authority_end = strpbrk(scheme + 3, "/?#");
    size_t origin_len = authority_end != NULL
        ? (size_t)(authority_end - connect_uri)
        : sdslen(connect_uri);

    // Absolute redirect uri
    if (redirect_uri[0] == '/') {
        // cut of path
        sdsrange(connect_uri, 0, (ssize_t)origin_len - 1);
        // add absolute uri
        return sdscat(connect_uri, redirect_uri);
    }

    if (authority_end == NULL || *authority_end != '/') {
        // The original URI has no path; resolve the relative URI from the host root.
        sdsrange(connect_uri, 0, (ssize_t)origin_len - 1);
        connect_uri = sdscat(connect_uri, "/");
    }
    else {
        // Keep the current directory and discard the filename and query.
        const char *path_end = strpbrk(authority_end, "?#");
        if (path_end == NULL) {
            path_end = connect_uri + sdslen(connect_uri);
        }
        const char *directory_end = path_end;
        while (directory_end > authority_end && directory_end[-1] != '/') {
            directory_end--;
        }
        sdsrange(connect_uri, 0, (ssize_t)(directory_end - connect_uri) - 1);
    }
    return sdscat(connect_uri, redirect_uri);
}

/**
 * Returns a sds pointer to the content-type header from response
 * @param response http response
 * @return sds or NULL if not found
 */
sds http_client_get_content_type(struct mg_client_response_t *response) {
    struct t_list_node *current = response->header.tail;
    while (current != NULL) {
        if (strcasecmp(current->key, "content-type") == 0) {
            return current->value_p;
        }
        current = current->next;
    }
    return NULL;
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
                .name = host,
            };
            if (mg_client_request->cert_check == true) {
                tls_opts.ca = mg_str(mg_client_request->ca_certs);
            }
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
                if (mg_str_to_num(hm->headers[i].value, 10, &content_length, sizeof(content_length)) == false) {
                    MYMPD_LOG_ERROR(NULL, "HTTP client invalid content-length");
                }
            }
            list_push_len(&mg_client_response->header, name, sdslen(name), 0, hm->headers[i].value.buf, hm->headers[i].value.len, NULL);
            sdsclear(name);
        }
        FREE_SDS(name);
        //http response code
        if (mg_str_to_num(hm->uri, 10, &mg_client_response->response_code, sizeof(mg_client_response->response_code)) == false) {
            MYMPD_LOG_ERROR(NULL, "HTTP client invalid response code");
            mg_client_response->response_code = 0;
        }
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
