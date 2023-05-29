/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/presets.h"

#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/list.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

#include <errno.h>

/**
 * Prints the list of presets as json array
 * @param presets preset list
 * @param buffer sds string to append the array
 * @return sds pointer to buffer
 */
sds presets_list(struct t_list *presets, sds buffer) {
    struct t_list_node *current = presets->head;
    int i = 0;
    while (current != NULL) {
        if (i++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = sds_catjson(buffer, current->key, sdslen(current->key));
        current = current->next;
    }
    return buffer;
}

bool presets_delete(struct t_list *presets, const char *preset) {
    int idx = list_get_node_idx(presets, preset);
    return list_remove_node(presets, idx);
}

/**
 * Callback function for presets_save
 * @param buffer buffer to append the line
 * @param current list node to print
 * @return pointer to buffer
 */
static sds preset_to_line_cb(sds buffer, struct t_list_node *current) {
    return sdscatfmt(buffer, "%S\n", current->value_p);
}

/**
 * Writes the preset list to disc
 * @param partition_state pointer to partition state
 * @return bool true on success, else false
 */
bool presets_save(struct t_partition_state *partition_state) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%S/%s",
        partition_state->mympd_state->config->workdir, partition_state->state_dir, FILENAME_PRESETS);
    bool rc = list_write_to_disk(filepath, &partition_state->presets, preset_to_line_cb);
    FREE_SDS(filepath);
    return rc;
}

/**
 * Reads the presets file from disc
 * @param partition_state pointer to partition state
 * @return bool true on success, else false
 */
bool presets_load(struct t_partition_state *partition_state) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%S/%s",
        partition_state->mympd_state->config->workdir, partition_state->state_dir, FILENAME_PRESETS);
    errno = 0;
    FILE *fp = fopen(filepath, OPEN_FLAGS_READ);
    if (fp != NULL) {
        sds line = sdsempty();
        while (sds_getline(&line, fp, LINE_LENGTH_MAX) >= 0) {
            sds name = NULL;
            if (json_get_string_max(line, "$.name", &name, vcb_isname, NULL) == true) {
                list_push(&partition_state->presets, name, 0, line, NULL);
            }
            else {
                MYMPD_LOG_ERROR(partition_state->name, "Reading presets line failed");
                MYMPD_LOG_DEBUG(partition_state->name, "Erroneous line: %s", line);
            }
            FREE_SDS(name);
        }
        (void) fclose(fp);
        FREE_SDS(line);
    }
    else {
        MYMPD_LOG_DEBUG(partition_state->name, "Can not open file \"%s\"", filepath);
        if (errno != ENOENT) {
            //ignore missing presets file
            MYMPD_LOG_ERRNO(partition_state->name, errno);
        }
    }
    FREE_SDS(filepath);
    return true;
}
