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

struct mg_user_data_t {
    sds post_data;
    sds uri;
};

static void print_usage(char **argv) {
    fprintf(stderr, "Usage: %s <URL> <scriptname> key=val ...\n"
                    "myMPD script utility\n"
                    "If scriptname is -, the script is read from stdin.\n"
                    "<URL>: e.g. https://localhost\n"
                    "For further details look at https://github.com/jcorporation/myMPD/wiki/Scripting\n\n",
            argv[0]);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *fn_data) {
    struct mg_user_data_t *mg_user_data = (struct mg_user_data_t *) nc->mgr->userdata;
    if (ev == MG_EV_CONNECT) {
        // Connected to server. Extract host name from URL
        struct mg_str host = mg_url_host(mg_user_data->uri);

        // If s_url is https://, tell client connection to use TLS
        if (mg_url_is_ssl(mg_user_data->uri)) {
            struct mg_tls_opts tls_opts = {
                .srvname = host
            };
            mg_tls_init(nc, &tls_opts);
        }
        
        // Send request
        mg_printf(nc,
              "POST %s HTTP/1.0\r\n"
              "Host: %.*s\r\n"
              "Content-type: application/json\r\n"
              "Content-length: %d\r\n"
              "\r\n"
              "%s\r\n",
              mg_url_uri(mg_user_data->uri), (int) host.len, host.ptr, 
              sdslen(mg_user_data->post_data), mg_user_data->post_data);
    } 
    else if (ev == MG_EV_HTTP_MSG) {
        // Response is received. Print it
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        printf("%.*s\n", (int) hm->body.len, hm->body.ptr);
        // Map response to error code
        if (strncmp("HTTP/1.1 200", hm->message.ptr, hm->message.len) == 0) {
            *(int *) fn_data = 0;  // return OK
        }
        else {
            *(int *) fn_data = 1;  // return error
        }
        nc->is_closing = 1;        // Tell mongoose to close this connection
    } 
    else if (ev == MG_EV_ERROR) {
        *(int *) fn_data = 1;  // Error, tell event loop to stop
        fprintf(stderr, "Connection failed\n");
    }
}

static sds parse_arguments(sds post_data, char **argv, int argc) {
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

    struct mg_user_data_t mg_user_data = {
        .post_data = sdsempty(),
        .uri = sdsnew(argv[1])
    };

    sdstrim(mg_user_data.uri, "/ ");
    
    if (strlen(argv[2]) == 1 && argv[2][0] == '-') {
        mg_user_data.uri = sdscat(mg_user_data.uri, "/api/script");
        int c;
        sds script_data = sdsempty();
        while ((c = getchar()) != EOF) {
            script_data = sdscatlen(script_data, &c, 1);
        }
        mg_user_data.post_data = sdscat(mg_user_data.post_data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SCRIPT_POST_EXECUTE\",\"params\":{\"script\":");
        mg_user_data.post_data = sdscatjson(mg_user_data.post_data, script_data, sdslen(script_data));
        mg_user_data.post_data = sdscat(mg_user_data.post_data, ",arguments:{");
        mg_user_data.post_data = parse_arguments(mg_user_data.post_data, argv, argc);
        mg_user_data.post_data = sdscat(mg_user_data.post_data, "}}}");
        sdsfree(script_data);
    }
    else {
        mg_user_data.uri = sdscat(mg_user_data.uri, "/api");
        mg_user_data.post_data = sdscat(mg_user_data.post_data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SCRIPT_EXECUTE\",\"params\":{\"script\":");
        mg_user_data.post_data = sdscatjson(mg_user_data.post_data, argv[2], strlen(argv[2]));
        mg_user_data.post_data = sdscat(mg_user_data.post_data, ",arguments:{");
        mg_user_data.post_data = parse_arguments(mg_user_data.post_data, argv, argc);
        mg_user_data.post_data = sdscat(mg_user_data.post_data, "}}}");
    }
    
    struct mg_mgr mgr;
    int rc = -1;
    mg_log_set("0");
    mg_mgr_init(&mgr);
    mgr.userdata = &mg_user_data;
    mg_http_connect(&mgr, mg_user_data.uri, ev_handler, &rc);
    while (rc == -1) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    sdsfree(mg_user_data.uri);
    sdsfree(mg_user_data.post_data);

    return rc;
}
