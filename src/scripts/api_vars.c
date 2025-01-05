/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Variables API functions
 */

#include "compile_time.h"
#include "src/scripts/api_vars.h"

#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

#include <errno.h>

// Private definitions
static sds var_to_line_cb(sds buffer, struct t_list_node *current, bool newline);

// Public functions

/**
 * Deletes a variable in the list
 * @param script_var_list pointer to script var list
 * @param key key to remove
 * @return true on success, else false
 */
bool scripts_vars_delete(struct t_list *script_var_list, sds key) {
    return list_remove_node_by_key(script_var_list, key);
}

/**
 * Adds/modifies a variable in the list
 * @param script_var_list pointer to script var list
 * @param key name of the variable
 * @param value value of the variable
 * @return true on success, else false
 */
bool scripts_vars_save(struct t_list *script_var_list, sds key, sds value) {
    struct t_list_node *node = list_get_node(script_var_list, key);
    if (node == NULL) {
        list_push(script_var_list, key, 0, value, NULL);
    }
    else {
        sdsclear(node->value_p);
        node->value_p = sdscat(node->value_p, value);
    }
    list_sort_by_key(script_var_list, LIST_SORT_ASC);
    return true;
}

/**
 * Reads the scripts variables from the filesystem
 * @param script_var_list pointer to script var list
 * @param workdir working directory
 * @return true on success, else false
 */
bool scripts_vars_file_read(struct t_list *script_var_list, sds workdir) {
    sds scripts_vars_file = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_STATE, FILENAME_SCRIPTVARS);
    errno = 0;
    FILE *fp = fopen(scripts_vars_file, OPEN_FLAGS_READ);
    int i = 0;
    if (fp == NULL) {
        //ignore error
        MYMPD_LOG_DEBUG(NULL, "Can not open file \"%s\"", scripts_vars_file);
        if (errno != ENOENT) {
            MYMPD_LOG_ERRNO(NULL, errno);
        }
        FREE_SDS(scripts_vars_file);
        return false;
    }

    sds line = sdsempty();
    int nread = 0;
    struct t_jsonrpc_parse_error parse_error;
    jsonrpc_parse_error_init(&parse_error);
    sds key = NULL;
    sds value = NULL;
    while ((line = sds_getline(line, fp, LINE_LENGTH_MAX, &nread)) && nread >= 0) {
        if (json_get_string_max(line, "$.key", &key, vcb_isname, &parse_error) == true &&
            json_get_string_max(line, "$.value", &value, vcb_isname, &parse_error) == true)
        {
            list_push(script_var_list, key, 0, value, NULL);
            i++;
        }
        else {
            MYMPD_LOG_ERROR(NULL, "Invalid line");
            break;
        }
        if (i == LIST_SCRIPT_VARS_MAX) {
            MYMPD_LOG_WARN(NULL, "Too many lines in scriptvars_list");
            break;
        }
        FREE_SDS(key);
        FREE_SDS(value);
    }
    FREE_SDS(line);
    FREE_SDS(key);
    FREE_SDS(value);
    jsonrpc_parse_error_clear(&parse_error);
    (void) fclose(fp);
    FREE_SDS(scripts_vars_file);
    MYMPD_LOG_INFO(NULL, "Read %u script variable(s) from disc", script_var_list->length);
    list_sort_by_key(script_var_list, LIST_SORT_ASC);
    return true;
}

/**
 * Writes the home icons to the filesystem
 * @param script_var_list pointer to script var list
 * @param workdir working directory
 * @return true on success, else false
 */
bool scripts_vars_file_save(struct t_list *script_var_list, sds workdir) {
    MYMPD_LOG_INFO(NULL, "Saving %u script variables to disc", script_var_list->length);
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_STATE, FILENAME_SCRIPTVARS);
    bool rc = list_write_to_disk(filepath, script_var_list, var_to_line_cb);
    FREE_SDS(filepath);
    return rc;
}

/**
 * Returns a jsonrpc response with all script variables
 * @param script_var_list pointer to script var list
 * @param buffer buffer to append the response
 * @param request_id jsonrpc request id
 * @return pointer to buffer
 */
sds scripts_vars_list(struct t_list *script_var_list, sds buffer, unsigned request_id) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SCRIPT_VAR_LIST;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    unsigned returned_entities = 0;
    struct t_list_node *current = script_var_list->head;
    while (current != NULL) {
        if (returned_entities++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = var_to_line_cb(buffer, current, false);
        current = current->next;
    }
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint(buffer, "returnedEntities", returned_entities, true);
    buffer = tojson_uint(buffer, "totalEntities", returned_entities, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

// private functions

/**
 * Callback function for mympd_api_script_vars_file_save
 * @param buffer buffer to append the line
 * @param current list node to print
 * @param newline append a newline char
 * @return pointer to buffer
 */
static sds var_to_line_cb(sds buffer, struct t_list_node *current, bool newline) {
    buffer = sdscatlen(buffer, "{", 1);
    buffer = tojson_sds(buffer, "key", current->key, true);
    buffer = tojson_sds(buffer, "value", current->value_p, false);
    buffer = sdscatlen(buffer, "}", 1);
    if (newline == true) {
        buffer = sdscatlen(buffer, "\n", 1);
    }
    return buffer;
}
