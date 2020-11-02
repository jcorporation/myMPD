/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <stdbool.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mympd_api_utility.h"
#include "mympd_api_home.h"

void mympd_api_read_home_list(t_config *config, t_mympd_state *mympd_state) {
    sds home_file = sdscatfmt(sdsempty(), "%s/state/home_list", config->varlibdir);
    FILE *fp = fopen(home_file, "r");
    if (fp != NULL) {
        char *line = NULL;
        char *crap = NULL;
        size_t n = 0;
        while (getline(&line, &n, fp) > 0) {
            strtok_r(line, "\n", &crap);
            list_push(&mympd_state->home_list, line, 0, NULL, NULL);
        }
        FREE_PTR(line);    
        fclose(fp);
    }
    sdsfree(home_file);
}

sds mympd_api_put_home_list(t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    int returned_entities = 0;
    struct list_node *current = mympd_state->home_list.head;
    while (current != NULL) {
        if (returned_entities++) {
            buffer = sdscat(buffer, ",");
        }
        buffer = sdscat(buffer, current->key);
        current = current->next;
    }
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "returnedEntities", returned_entities, false);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}
