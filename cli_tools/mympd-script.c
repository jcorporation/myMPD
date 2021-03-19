/*
 SPDX-License-Identifier: GPL-2.0fd-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "../dist/src/mongoose/mongoose.h"
#include "../dist/src/sds/sds.h"
#include "../src/sds_extras.h"

static sds post_data;
static sds uri;

static void print_usage(char **argv) {
    fprintf(stderr, "Usage: %s <URL> <scriptname> key=val ...\n"
                    "myMPD script utility\n"
                    "If scriptname is -, the script is read from stdin.\n"
                    "For further details look at https://github.com/jcorporation/myMPD/wiki/Scripting\n\n",
            argv[0]);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_CONNECT) {
        // Connected to server. Extract host name from URL
        struct mg_str host = mg_url_host(uri);

        // If s_url is https://, tell client connection to use TLS
        if (mg_url_is_ssl(uri)) {
            mg_tls_init(nc, NULL);
        }

        // Send request
        mg_printf(nc,
              "POST %s HTTP/1.0\r\n"
              "Host: %.*s\r\n"
              "Content-type: application/json\r\n"
              "\r\n"
              "%s\r\n",
              mg_url_uri(uri), (int) host.len, host.ptr, post_data);
    } 
    else if (ev == MG_EV_HTTP_MSG) {
        // Response is received. Print it
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        printf("%.*s", (int) hm->message.len, hm->message.ptr);
        nc->is_closing = 1;         // Tell mongoose to close this connection
        *(bool *) fn_data = true;  // Tell event loop to stop
    } 
    else if (ev == MG_EV_ERROR) {
        *(bool *) fn_data = true;  // Error, tell event loop to stop
        fprintf(stderr, "Connection failed\n");
    }
}

static sds parse_arguments(char **argv, int argc) {
    if (argc < 4) {
        return post_data;
    }
    for (int i = 3; i < argc; i++) {
        if (i > 3) {
            post_data = sdscatlen(post_data, ",", 1);
        }
        int count;
        sds *kv = sdssplitlen(argv[i], strlen(argv[i]), "=", 1, &count);
        if (count == 2) {
            post_data = sdscatjson(post_data, kv[0], sdslen(kv[0]));
            post_data = sdscat(post_data, ":");
            post_data = sdscatjson(post_data, kv[1], sdslen(kv[1]));
        }
        else {
            fprintf(stderr, "Invalid key/value pair: %s\n", argv[i]);
        }
        sdsfreesplitres(kv, count);
    }
    return post_data;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print_usage(argv);
        return EXIT_FAILURE;
    }
    
    post_data = sdsempty();
    uri = sdsnew(argv[1]);
    
    if (strlen(argv[2]) == 1 && argv[2][0] == '-') {
        uri = sdscat(uri, "/api/script");
        int c;
        sds script_data = sdsempty();
        while ((c = getchar()) != EOF) {
            script_data = sdscatlen(script_data, &c, 1);
        }

        post_data = sdscat(post_data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SCRIPT_POST_EXECUTE\",\"params\":{\"script\":");
        post_data = sdscatjson(post_data, script_data, sdslen(script_data));
        post_data = sdscat(post_data, ",arguments:{");
        post_data = parse_arguments(argv, argc);
        post_data = sdscat(post_data, "}}}");
        sdsfree(script_data);
    }
    else {
        uri = sdscat(uri, "/api");
        post_data = sdscat(post_data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SCRIPT_EXECUTE\",\"params\":{\"script\":");
        post_data = sdscatjson(post_data, argv[2], strlen(argv[2]));
        post_data = sdscat(post_data, ",arguments:{");
        post_data = parse_arguments(argv, argc);
        post_data = sdscat(post_data, "}}}");
    }

    struct mg_mgr mgr;
    bool done = false;
    mg_mgr_init(&mgr);
    mg_http_connect(&mgr, uri, ev_handler, &done);
    while (!done) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    sdsfree(uri);
    sdsfree(post_data);

    return EXIT_SUCCESS;
}
