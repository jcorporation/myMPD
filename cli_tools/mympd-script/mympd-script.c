/*
 SPDX-License-Identifier: GPL-2.0fd-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "dist/sds/sds.h"
#include "src/lib/cacertstore.h"
#include "src/lib/filehandler.h"
#include "src/lib/http_client.h"
#include "src/lib/list.h"
#include "src/lib/sds_extras.h"
#include "cli_tools/log.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(char **argv) {
    fprintf(stderr, "\nmyMPD script utility %s\n\n"
        "Usage: %s [-k] <uri> <partition> <scriptname> [<arguments>]\n"
        "  -k:           Disable certificate checking\n"
        "  <uri>:        myMPD listening uri, e.g. \"https://localhost:8443\"\n"
        "  <partition>:  MPD partition, e.g. \"default\"\n"
        "  <scriptname>: script to execute, use \"-\" to read the script from stdin.\n"
        "  <arguments>:  optional space separated key=value pairs for script arguments.\n"
        "\n"
        "For further details look at https://jcorporation.github.io/myMPD/050-scripting/mympd-script/\n\n",
        MYMPD_VERSION, argv[0]);
}

static sds parse_arguments(sds post_data, char **argv, int argc, int arg_offset) {
    if (argc < 5 + arg_offset) {
        return post_data;
    }
    for (int i = 4 + arg_offset; i < argc; i++) {
        if (i > 4 + arg_offset) {
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
    bool cert_check = true;
    int min_args = 4;
    int arg_offset = 0;
    if (argc > 1 && strcmp(argv[1], "-k") == 0) {
        cert_check = false;
        min_args++;
        arg_offset++;
    }

    if (argc < min_args) {
        print_usage(argv);
        return EXIT_FAILURE;
    }

    set_loglevel(5);

    sds uri = sdsempty();
    sds post_data = sdsempty();

    if (strlen(argv[3 + arg_offset]) == 1 && argv[3 + arg_offset][0] == '-') {
        uri = sdscatfmt(uri, "%s/script-api/%s", argv[1 + arg_offset], argv[2 + arg_offset]);
        int c;
        sds script_data = sdsempty();
        while ((c = getchar()) != EOF) {
            script_data = sdscatlen(script_data, &c, 1);
        }
        post_data = sdscat(post_data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"INTERNAL_API_SCRIPT_POST_EXECUTE\",\"params\":{\"script\":");
        post_data = sds_catjson(post_data, script_data, sdslen(script_data));
        post_data = sdscat(post_data, ",\"arguments\":{");
        post_data = parse_arguments(post_data, argv, argc, arg_offset);
        post_data = sdscat(post_data, "},\"event\":\"extern\"}");
        sdsfree(script_data);
    }
    else {
        uri = sdscatfmt(uri, "%s/api/%s", argv[1 + arg_offset], argv[2 + arg_offset]);
        post_data = sdscat(post_data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SCRIPT_EXECUTE\",\"params\":{\"script\":");
        post_data = sds_catjson(post_data, argv[3 + arg_offset], strlen(argv[3 + arg_offset]));
        post_data = sdscat(post_data, ",\"arguments\":{");
        post_data = parse_arguments(post_data, argv, argc, arg_offset);
        post_data = sdscat(post_data, "},\"event\":\"user\"}");
    }

    struct mg_client_request_t request = {
        .method = "POST",
        .uri = uri,
        .extra_headers = "Content-type: application/json\r\n",
        .post_data = post_data,
        .cert_check = cert_check
    };

    if (cert_check == true) {
        const char *ca_cert_store = find_ca_cert_store(true);
        sds ca_certs;
        if (ca_cert_store != NULL) {
            int nread;
            ca_certs = sds_getfile(sdsempty(), ca_cert_store, CACERT_STORE_SIZE_MAX, false, true, &nread);
            if (nread < 0) {
                printf("ERROR: System certificate store not found.");
                ca_certs = sdscat(ca_certs, "invalid");
            }
        }
        else {
            printf("ERROR: System certificate store not found.");
            ca_certs = sdscat(sdsempty(), "invalid");
        }
        request.ca_certs = ca_certs;
    }

    struct mg_client_response_t response;
    http_client_response_init(&response);

    http_client_request(&request, &response);
    puts(response.body);

    http_client_response_clear(&response);
    sdsfree(uri);
    sdsfree(post_data);

    return response.rc;
}
