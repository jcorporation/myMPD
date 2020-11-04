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


bool mympd_api_swap_home_icon(t_mympd_state *mympd_state, unsigned int pos1, unsigned int pos2) {
    return list_swap_item_pos(&mympd_state->home_list, pos1, pos2);
}

bool mympd_api_rm_home_icon(t_mympd_state *mympd_state, unsigned int pos) {
    return list_shift(&mympd_state->home_list, pos);
}

bool mympd_api_save_home_icon(t_mympd_state *mympd_state, bool replace, unsigned int oldpos,
    const char *name, const char *ligature, const char *bgcolor, const char *image,
    const char *cmd, struct list *option_list) 
{
    sds key = sdscatlen(sdsempty(), "{", 1);
    key = tojson_char(key, "name", name, true);
    key = tojson_char(key, "ligature", ligature, true);
    key = tojson_char(key, "bgcolor", bgcolor, true);
    key = tojson_char(key, "image", image, true);
    key = tojson_char(key, "cmd", cmd, true);
    key = sdscat(key, "\"options\":[");
    struct list_node *current = option_list->head;
    int i = 0;
    while (current != NULL) {
        if (i++) {
            key = sdscatlen(key, ",", 1);
        }
        key = sdscatjson(key, current->key, sdslen(current->key));
        current = current->next;
    }    
    key = sdscatlen(key, "]}", 2);
    bool rc = false;
    if (replace == true) {
        rc = list_replace(&mympd_state->home_list, oldpos, key, 0, NULL, NULL);
    }
    else {
        rc = list_push(&mympd_state->home_list, key, 0, NULL, NULL);
    }
    sdsfree(key);
    return rc;
}

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

bool mympd_api_write_home_list(t_config *config, t_mympd_state *mympd_state) {
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/home_list.XXXXXX", config->varlibdir);
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    struct list_node *current = mympd_state->home_list.head;
    while (current != NULL) {
        fprintf(fp,"%s\n", current->key);
        current = current->next;
    }
    fclose(fp);
    sds home_file = sdscatfmt(sdsempty(), "%s/state/home_list", config->varlibdir);
    if (rename(tmp_file, home_file) == -1) {
        LOG_ERROR("Rename file from %s to %s failed", tmp_file, home_file);
        sdsfree(tmp_file);
        sdsfree(home_file);
        return false;
    }
    sdsfree(tmp_file);    
    sdsfree(home_file);
    return true;
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

