/*
 SPDX-License-Identifier: GPL-2.0fd-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../dist/src/mongoose/mongoose.h"
#include "../dist/src/sds/sds.h"
#include "../src/sds_extras.h"

static int s_exit_flag = 0;

void print_usage(char **argv) {
    fprintf(stderr, "Usage: %s <URL>\n"
                    "myMPD script utility, for details look at https://github.com/jcorporation/myMPD\n\n",
            argv[0]);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message *) ev_data;

    switch (ev) {
        case MG_EV_CONNECT:
            if (*(int *) ev_data != 0) {
                fprintf(stderr, "Connection failed: %s\n", strerror(*(int *) ev_data));
                s_exit_flag = 1;
            }
            break;
        case MG_EV_HTTP_REPLY:
            nc->flags |= MG_F_CLOSE_IMMEDIATELY;
            fwrite(hm->body.p, 1, hm->body.len, stdout);
            putchar('\n');
            s_exit_flag = 1;
            break;
        case MG_EV_CLOSE:
            if (s_exit_flag == 0) {
                printf("Server closed connection\n");
                s_exit_flag = 1;
            }
            break;
        default:
            break;
    }
}

int main(int argc, char **argv) {
    int rc = EXIT_SUCCESS;
  
    if (argc < 2 || argc > 3) {
        print_usage(argv);
        goto cleanup;
    }
    
    sds post_data = sdsempty();
    sds uri = sdsnew(argv[1]);
    
    if (argc == 2) {
        uri = sdscat(uri, "/api/script");
        char c;
        sds script_data = sdsempty();
        while ((c = getchar()) != EOF) {
            script_data = sdscatlen(script_data, &c, 1);
        }

        post_data = sdscat(post_data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SCRIPT_POST_EXECUTE\",\"params\":{\"script\":");
        post_data = sdscatjson(post_data, script_data, sdslen(script_data));
        post_data = sdscat(post_data, "}}");
        sdsfree(script_data);
    }
    else {
        uri = sdscat(uri, "/api");
        post_data = sdscat(post_data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SCRIPT_EXECUTE\",\"params\":{\"script\":");
        post_data = sdscatjson(post_data, argv[2], strlen(argv[2]));
        post_data = sdscat(post_data, "}}");
    }

    struct mg_mgr mgr;
    mg_mgr_init(&mgr, NULL);
    mg_connect_http(&mgr, ev_handler, uri, "Content-Type: application/json\r\n", post_data);
    while (s_exit_flag == 0) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    sdsfree(uri);
    sdsfree(post_data);

    cleanup:
    return rc;
}
