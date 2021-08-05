/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_home.h"

#include "../log.h"
#include "../sds_extras.h"
#include "../utility.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool mympd_api_move_home_icon(struct t_mympd_state *mympd_state, unsigned int from, unsigned int to) {
    return list_move_item_pos(&mympd_state->home_list, from, to);
}

bool mympd_api_rm_home_icon(struct t_mympd_state *mympd_state, unsigned int pos) {
    return list_shift(&mympd_state->home_list, pos);
}

bool mympd_api_save_home_icon(struct t_mympd_state *mympd_state, bool replace, unsigned int oldpos,
    const char *name, const char *ligature, const char *bgcolor, const char *color, const char *image,
    const char *cmd, struct list *option_list) 
{
    sds key = sdscatlen(sdsempty(), "{", 1);
    key = tojson_char(key, "name", name, true);
    key = tojson_char(key, "ligature", ligature, true);
    key = tojson_char(key, "bgcolor", bgcolor, true);
    key = tojson_char(key, "color", color, true);
    key = tojson_char(key, "image", image, true);
    key = tojson_char(key, "cmd", cmd, true);
    key = sdscat(key, "\"options\":[");
    struct list_node *current = option_list->head;
    int i = 0;
    while (current != NULL) {
        if (i++) {
            key = sdscatlen(key, ",", 1);
        }
        if (strcmp(current->key, "!undefined!") == 0) {
            key = sdscatjson(key, "", 0);
        }
        else {
            key = sdscatjson(key, current->key, sdslen(current->key));
        }
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

bool mympd_api_read_home_list(struct t_mympd_state *mympd_state) {
    sds home_file = sdscatfmt(sdsempty(), "%s/state/home_list", mympd_state->config->workdir);
    errno = 0;
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
    else {
        //ignore error
        MYMPD_LOG_DEBUG("Can not open file \"%s\"", home_file);
        if (errno != ENOENT) {
            MYMPD_LOG_ERRNO(errno);
        }
        sdsfree(home_file);
        return false;
    }
    sdsfree(home_file);
    return true;
}

bool mympd_api_write_home_list(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_INFO("Saving home icons to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/home_list.XXXXXX", mympd_state->config->workdir);
    errno = 0;
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        MYMPD_LOG_ERROR("Can not open \"%s\" for write", tmp_file);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    struct list_node *current = mympd_state->home_list.head;
    while (current != NULL) {
        int rc = fprintf(fp,"%s\n", current->key);
        if (rc < 0) {
            MYMPD_LOG_ERROR("Can not write to file \"%s\"", tmp_file);
            sdsfree(tmp_file);
            fclose(fp);
            return false;
        }
        current = current->next;
    }
    fclose(fp);
    sds home_file = sdscatfmt(sdsempty(), "%s/state/home_list", mympd_state->config->workdir);
    errno = 0;
    if (rename(tmp_file, home_file) == -1) {
        MYMPD_LOG_ERROR("Rename file from \"%s\" to \"%s\" failed", tmp_file, home_file);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(tmp_file);
        sdsfree(home_file);
        return false;
    }
    sdsfree(tmp_file);    
    sdsfree(home_file);
    return true;
}

sds mympd_api_put_home_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
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
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

sds mympd_api_get_home_icon(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, unsigned pos) {
    struct list_node *current = list_node_at(&mympd_state->home_list, pos);

    if (current != NULL) {
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = sdscat(buffer, "\"data\":");
        buffer = sdscat(buffer, current->key);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = tojson_long(buffer, "returnedEntities", 1, false);
        buffer = jsonrpc_result_end(buffer);
        return buffer;
    }

    MYMPD_LOG_ERROR("Can not get home icon at pos %u", pos);
    buffer = jsonrpc_respond_message(buffer, method, request_id, true, "home", "error", "Can not get home icon");
    return buffer;
}
