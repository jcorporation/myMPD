/*
 SPDX-License-Identifier: GPL-2.0fd-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "../dist/sds/sds.h"
#include "../src/lib/http_client.h"
#include "../src/lib/sds_extras.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

static void print_usage(char **argv) {
    fprintf(stderr, "\nmyMPD script utility\n\n"
        "Usage: %s <uri> <partition> <scriptname> [<arguments>]\n"
        "  <uri>:        myMPD listening uri, e.g. \"https://localhost\"\n"
        "  <partition>:  MPD partition, e.g. \"default\"\n"
        "  <scriptname>: script to execute, use \"-\" to read the script from stdin.\n"
        "  <arguments>:  optional space separated key=value pairs for script arguments.\n"
        "\nFor further details look at https://jcorporation.github.io/myMPD/scripting/\n\n",
        argv[0]);
}

static sds parse_arguments(sds post_data, char **argv, int argc) {
    if (argc < 5) {
        return post_data;
    }
    for (int i = 4; i < argc; i++) {
        if (i > 4) {
            post_data = sdscatlen(post_data, ",", 1);
        }
        int count = 0;
        sds *kv = sdssplitlen(argv[i], (ssize_t)strlen(argv[i]), "=", 1, &count);
        if (count == 2) {
            post_data = sds_catjson(post_data, kv[0], sdslen(kv[0]));
            post_data = sdscat(post_data, ":");
            post_data = sds_catjson(post_data, kv[1], sdslen(kv[1]));
        }
        else {
            fprintf(stderr, "Invalid key/value pair: %s\n", argv[i]);
        }
        sdsfreesplitres(kv, count);
    }
    return post_data;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        print_usage(argv);
        return EXIT_FAILURE;
    }

    set_loglevel(5);

    sds uri = sdsempty();
    sds post_data = sdsempty();

    if (strlen(argv[3]) == 1 && argv[3][0] == '-') {
        uri = sdscatfmt(uri, "%s/script-api/%s", argv[1], argv[2]);
        int c;
        sds script_data = sdsempty();
        while ((c = getchar()) != EOF) {
            script_data = sdscatlen(script_data, &c, 1);
        }
        post_data = sdscat(post_data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"INTERNAL_API_SCRIPT_POST_EXECUTE\",\"params\":{\"script\":");
        post_data = sds_catjson(post_data, script_data, sdslen(script_data));
        post_data = sdscat(post_data, ",\"arguments\":{");
        post_data = parse_arguments(post_data, argv, argc);
        post_data = sdscat(post_data, "}}}");
        sdsfree(script_data);
    }
    else {
        uri = sdscatfmt(uri, "%s/api/%s", argv[1], argv[2]);

        post_data = sdscat(post_data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SCRIPT_EXECUTE\",\"params\":{\"script\":");
        post_data = sds_catjson(post_data, argv[3], strlen(argv[3]));
        post_data = sdscat(post_data, ",\"arguments\":{");
        post_data = parse_arguments(post_data, argv, argc);
        post_data = sdscat(post_data, "}}}");
    }

    struct mg_client_request_t request = {
        .method = "POST",
        .uri = uri,
        .extra_headers = "Content-type: application/json\r\n",
        .post_data = post_data
    };

    struct mg_client_response_t response = {
        .rc = -1,
        .response = sdsempty(),
        .header = sdsempty(),
        .body = sdsempty()
    };

    http_client_request(&request, &response);
    puts(response.response);
    puts(response.body);

    sdsfree(response.response);
    sdsfree(response.header);
    sdsfree(response.body);
    sdsfree(uri);
    sdsfree(post_data);

    return response.rc;
}
