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
#include "src/mympd_api/requests.h"
#include "src/mympd_api/settings.h"

#include <errno.h>

/**
 * Applies a preset
 * @param partition_state pointer to partition state
 * @param preset_name preset name
 * @param error pointer to alreay allocated sds string to append the error message
 * @return true on success, else false
 */
bool preset_apply(struct t_partition_state *partition_state, sds preset_name, sds *error) {
    struct t_list_node *preset = list_get_node(&partition_state->preset_list, preset_name);
    if (preset != NULL) {
        struct t_jsonrpc_parse_error parse_error;
        jsonrpc_parse_error_init(&parse_error);
        if (json_iterate_object(preset->value_p, "$", mympd_api_settings_mpd_options_set, partition_state, NULL, 100, &parse_error) == true) {
            if (partition_state->jukebox_mode != JUKEBOX_OFF) {
                mympd_api_request_jukebox_restart(partition_state->name);
            }
            jsonrpc_parse_error_clear(&parse_error);
            return true;
        }
        *error = sdscat(*error, "Can't set playback options");
        jsonrpc_parse_error_clear(&parse_error);
    }
    else {
        *error = sdscat(*error,  "Could not load preset");
    }
    return false;
}

/**
 * Prints the list of presets as json array
 * @param preset_list preset list
 * @param buffer sds string to append the array
 * @return sds pointer to buffer
 */
sds presets_list(struct t_list *preset_list, sds buffer) {
    struct t_list_node *current = preset_list->head;
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

/**
 * Saves a preset
 * @param preset_list pointer to preset list
 * @param preset_name preset name
 * @param preset_value preset value (stringified json)
 * @param error pointer to alreay allocated sds string to append the error message
 * @return true on success, else false
 */
bool preset_save(struct t_list *preset_list, sds preset_name, sds preset_value, sds *error) {
    if (preset_value == NULL) {
        *error = sdscat(*error, "Can't save preset");
        return false;
    }

    int idx = list_get_node_idx(preset_list, preset_name);
    if (idx > -1) {
        // update existing preset
        return list_replace(preset_list, idx, preset_name, 0, preset_value, NULL);
    }
    // add new preset
    return list_push(preset_list, preset_name, 0, preset_value, NULL);
}

/**
 * Deletes a preset
 * @param preset_list pointer to preset list
 * @param preset_name preset name
 * @return bool 
 */
bool preset_delete(struct t_list *preset_list, const char *preset_name) {
    int idx = list_get_node_idx(preset_list, preset_name);
    return list_remove_node(preset_list, idx);
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
bool preset_list_save(struct t_partition_state *partition_state) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%S/%s",
        partition_state->config->workdir, partition_state->state_dir, FILENAME_PRESETS);
    bool rc = list_write_to_disk(filepath, &partition_state->preset_list, preset_to_line_cb);
    FREE_SDS(filepath);
    return rc;
}

/**
 * Reads the presets file from disc
 * @param partition_state pointer to partition state
 * @return bool true on success, else false
 */
bool preset_list_load(struct t_partition_state *partition_state) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%S/%s",
        partition_state->config->workdir, partition_state->state_dir, FILENAME_PRESETS);
    errno = 0;
    FILE *fp = fopen(filepath, OPEN_FLAGS_READ);
    if (fp != NULL) {
        sds line = sdsempty();
        while (sds_getline(&line, fp, LINE_LENGTH_MAX) >= 0) {
            sds name = NULL;
            if (json_get_string_max(line, "$.name", &name, vcb_isname, NULL) == true) {
                list_push(&partition_state->preset_list, name, 0, line, NULL);
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
