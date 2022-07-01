/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_home.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool mympd_api_home_icon_move(struct t_mympd_state *mympd_state, long from, long to) {
    return list_move_item_pos(&mympd_state->home_list, from, to);
}

bool mympd_api_home_icon_delete(struct t_mympd_state *mympd_state, long pos) {
    return list_remove_node(&mympd_state->home_list, pos);
}

bool mympd_api_home_icon_save(struct t_mympd_state *mympd_state, bool replace, long oldpos,
    const char *name, const char *ligature, const char *bgcolor, const char *color, const char *image,
    const char *cmd, struct t_list *option_list)
{
    sds key = sdsnewlen("{", 1);
    key = tojson_char(key, "name", name, true);
    key = tojson_char(key, "ligature", ligature, true);
    key = tojson_char(key, "bgcolor", bgcolor, true);
    key = tojson_char(key, "color", color, true);
    key = tojson_char(key, "image", image, true);
    key = tojson_char(key, "cmd", cmd, true);
    key = sdscat(key, "\"options\":[");
    struct t_list_node *current = option_list->head;
    int i = 0;
    while (current != NULL) {
        if (i++) {
            key = sdscatlen(key, ",", 1);
        }
        key = sds_catjson(key, current->key, sdslen(current->key));
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
    FREE_SDS(key);
    return rc;
}

bool mympd_api_home_file_read(struct t_mympd_state *mympd_state) {
    sds home_file = sdscatfmt(sdsempty(), "%s/state/home_list", mympd_state->config->workdir);
    errno = 0;
    FILE *fp = fopen(home_file, OPEN_FLAGS_READ);
    int i = 0;
    if (fp == NULL) {
        //ignore error
        MYMPD_LOG_DEBUG("Can not open file \"%s\"", home_file);
        if (errno != ENOENT) {
            MYMPD_LOG_ERRNO(errno);
        }
        FREE_SDS(home_file);
        return false;
    }

    sds line = sdsempty();
    while (sds_getline(&line, fp, 1000) == 0) {
        if (validate_json(line) == false) {
            MYMPD_LOG_ERROR("Invalid line");
            break;
        }
        list_push(&mympd_state->home_list, line, 0, NULL, NULL);
        i++;
        if (i == LIST_HOME_ICONS_MAX) {
            MYMPD_LOG_WARN("Too many lines in home_list");
            break;
        }
    }
    FREE_SDS(line);
    (void) fclose(fp);
    FREE_SDS(home_file);
    return true;
}

static sds homeicon_to_line_cb(sds buffer, struct t_list_node *current) {
    return sdscatfmt(buffer, "%S\n", current->key);
}

bool mympd_api_home_file_save(struct t_mympd_state *mympd_state) {
    MYMPD_LOG_INFO("Saving home icons to disc");
    sds filepath = sdscatfmt(sdsempty(), "%s/state/home_list", mympd_state->config->workdir);
    bool rc = list_write_to_disk(filepath, &mympd_state->home_list, homeicon_to_line_cb);
    FREE_SDS(filepath);
    return rc;
}

sds mympd_api_home_icon_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id) {
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    int returned_entities = 0;
    struct t_list_node *current = mympd_state->home_list.head;
    while (current != NULL) {
        if (returned_entities++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sdscat(buffer, current->key);
        current = current->next;
    }
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "returnedEntities", returned_entities, false);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

sds mympd_api_home_icon_get(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id, long pos) {
    struct t_list_node *current = list_node_at(&mympd_state->home_list, pos);
    if (current != NULL) {
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = sdscat(buffer, "\"data\":");
        buffer = sdscat(buffer, current->key);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = tojson_long(buffer, "returnedEntities", 1, false);
        buffer = jsonrpc_result_end(buffer);
        return buffer;
    }

    MYMPD_LOG_ERROR("Can not get home icon at pos %ld", pos);
    buffer = jsonrpc_respond_message(buffer, method, request_id, true, "home", "error", "Can not get home icon");
    return buffer;
}
