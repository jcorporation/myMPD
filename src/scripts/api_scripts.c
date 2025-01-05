/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Scripts API functions
 */

#include "compile_time.h"
#include "src/scripts/api_scripts.h"

#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/scripts/util.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

/**
 * Private definitions
 */

bool parse_script(sds scriptfilename, sds *metadata, sds *content, int *order);

/**
 * Public functions
 */

/**
 * Reload the scripts from disk.
 * @param scripts_state pointer to scripts_state
 * @return true on success, else false
 */
bool scripts_file_reload(struct t_scripts_state *scripts_state) {
    list_clear_user_data(&scripts_state->script_list, list_free_cb_script_list_user_data);
    return scripts_file_read(scripts_state);
}

/**
 * Reads the scripts from the scripts directory
 * @param scripts_state pointer to scripts_state
 * @return true on success, else false
 */
bool scripts_file_read(struct t_scripts_state *scripts_state) {
    sds scriptdirname = sdscatfmt(sdsempty(), "%S/%s", scripts_state->config->workdir, DIR_WORK_SCRIPTS);
    errno = 0;
    DIR *script_dir = opendir(scriptdirname);
    if (script_dir == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can not open directory \"%s\"", scriptdirname);
        MYMPD_LOG_ERRNO(NULL, errno);
        FREE_SDS(scriptdirname);
        return false;
    }

    struct dirent *next_file;
    sds metadata = sdsempty();
    sds scriptname = sdsempty();
    sds scriptfilename = sdsempty();
    while ((next_file = readdir(script_dir)) != NULL ) {
        const char *ext = get_extension_from_filename(next_file->d_name);
        if (ext == NULL ||
            strcasecmp(ext, "lua") != 0)
        {
            continue;
        }
        scriptname = sdscat(scriptname, next_file->d_name);
        strip_file_extension(scriptname);
        scriptfilename = sdscatfmt(scriptfilename, "%S/%s", scriptdirname, next_file->d_name);
        int order = 0;
        sds content = sdsempty();
        bool rc = parse_script(scriptfilename, &metadata, &content, &order);
        if (rc == true) {
            struct t_script_list_data *user_data = malloc_assert(sizeof(struct t_script_list_data));
            user_data->bytecode = NULL;
            user_data->script = content;
            list_push(&scripts_state->script_list, scriptname, order, metadata, user_data);
        }
        sdsclear(metadata);
        sdsclear(scriptname);
        sdsclear(scriptfilename);
    }
    closedir(script_dir);
    FREE_SDS(scriptname);
    FREE_SDS(scriptfilename);
    FREE_SDS(metadata);
    FREE_SDS(scriptdirname);
    list_sort_by_key(&scripts_state->script_list, LIST_SORT_ASC);
    MYMPD_LOG_INFO(NULL, "Read %u script(s) from disc", scripts_state->script_list.length);
    return true;
}

/**
 * Lists scripts
 * @param script_list list of scripts
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param all true = print all scripts, false = print only scripts with order > 0
 * @return pointer to buffer
 */
sds script_list(struct t_list *script_list, sds buffer, unsigned request_id, bool all) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SCRIPT_LIST;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    unsigned returned_entities = 0;
    struct t_list_node *current = script_list->head;
    while (current != NULL) {
        if (all == true ||
            current->value_i > 0)
        {
            if (returned_entities++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_char(buffer, "name", current->key, true);
            buffer = tojson_raw(buffer, "metadata", current->value_p, false);
            buffer = sdscatlen(buffer, "}", 1);
        }
        current = current->next;
    }
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint(buffer, "returnedEntities", returned_entities, true);
    buffer = tojson_uint(buffer, "totalEntities", returned_entities, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Deletes a script
 * @param scripts_state pointer to scripts_state
 * @param scriptname script to delete (name without extension)
 * @return true on success, else false
 */
bool script_delete(struct t_scripts_state *scripts_state, sds scriptname) {
    bool rc = false;
    if (list_remove_node_by_key_user_data(&scripts_state->script_list, scriptname, list_free_cb_script_list_user_data) == true) {
        sds filepath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", scripts_state->config->workdir, DIR_WORK_SCRIPTS, scriptname);
        rc = rm_file(filepath);
        FREE_SDS(filepath);
    }
    return rc;
}

/**
 * Saves a script
 * @param scripts_state pointer to scripts_state
 * @param scriptname scriptname
 * @param oldscript old scriptname (leave empty for a new script)
 * @param file imported script filename (empty for local scripts)
 * @param order script list is ordered by this value
 * @param version imported script version (0 for local scripts)
 * @param content script content
 * @param arguments arguments for the script
 * @param error already allocated sds string to hold the error message
 * @return true on success, else false
 */
bool script_save(struct t_scripts_state *scripts_state, sds scriptname, sds oldscript,
        sds file, int order, int version, sds content, struct t_list *arguments, sds *error)
{
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", scripts_state->config->workdir, DIR_WORK_SCRIPTS, scriptname);
    sds argstr = list_to_json_array(sdsempty(), arguments);
    sds metadata = sdscatlen(sdsempty(), "{", 1);
    metadata = tojson_int(metadata, "order", order, true);
    metadata = tojson_sds(metadata, "file", file, true);
    metadata = tojson_int(metadata, "version", version, true);
    metadata = tojson_raw(metadata, "arguments", argstr, false);
    metadata = sdscatlen(metadata, "}", 1);
    sds scriptfile_content = sdscatfmt(sdsempty(), "-- %S\n%S", metadata, content);
    bool rc = write_data_to_file(filepath, scriptfile_content, sdslen(scriptfile_content));
    //delete old scriptfile
    if (rc == true &&
        sdslen(oldscript) > 0)
    {
        if (strcmp(scriptname, oldscript) != 0) {
            sds old_filepath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", scripts_state->config->workdir, DIR_WORK_SCRIPTS, oldscript);
            rc = rm_file(old_filepath);
            FREE_SDS(old_filepath);
        }
        list_remove_node_by_key_user_data(&scripts_state->script_list, oldscript, list_free_cb_script_list_user_data);
    }
    FREE_SDS(argstr);
    FREE_SDS(scriptfile_content);
    FREE_SDS(filepath);
    if (rc == true) {
        struct t_script_list_data *user_data = malloc_assert(sizeof(struct t_script_list_data));
        user_data->bytecode = NULL;
        user_data->script = sdsdup(content);
        list_push(&scripts_state->script_list, scriptname, order, metadata, user_data);
        list_sort_by_key(&scripts_state->script_list, LIST_SORT_ASC);
    }
    else {
        *error = sdscat(*error, "Could not save script");
    }
    FREE_SDS(metadata);
    return rc;
}

/**
 * Gets the script and its details
 * @param script_list list of scripts
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param scriptname scriptname to read from filesystem
 * @return pointer to buffer
 */
sds script_get(struct t_list *script_list, sds buffer, unsigned request_id, sds scriptname) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SCRIPT_GET;

    struct t_list_node *script = list_get_node(script_list, scriptname);
    if (script == NULL) {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Can not open script");
    }
    struct t_script_list_data *data = (struct t_script_list_data *)script->user_data;

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = tojson_sds(buffer, "script", scriptname, true);
    buffer = tojson_raw(buffer, "metadata", script->value_p, true);
    buffer = tojson_sds(buffer, "content", data->script, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Private functions
 */

/**
 * Parses the script file.
 * The metadata line is the first line of the scriptfile.
 * @param scriptfilename File to read
 * @param metadata Pointer to sds string to populate the metadata
 * @param content Pointer to sds string to populate with the script content
 * @param order Pointer to int to populate with order
 * @return true on success, else false
 */
bool parse_script(sds scriptfilename, sds *metadata, sds *content, int *order) {
    errno = 0;
    FILE *fp = fopen(scriptfilename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can not open file \"%s\"", scriptfilename);
        MYMPD_LOG_ERRNO(NULL, errno);
        return false;
    }

    int nread = 0;
    sds line = sds_getline(sdsempty(), fp, LINE_LENGTH_MAX, &nread);
    if (nread >= 0 &&
        strncmp(line, "-- ", 3) == 0)
    {
        sdsrange(line, 3, -1);
        if (json_get_int(line, "$.order", 0, 99, order, NULL) == true) {
            *metadata = sdscat(*metadata, line);
        }
        else {
            MYMPD_LOG_WARN(NULL, "Invalid metadata for script %s", scriptfilename);
            return false;
        }
    }
    else {
        MYMPD_LOG_WARN(NULL, "Invalid metadata for script %s", scriptfilename);
        return false;
    }
    FREE_SDS(line);
    *content = sds_getfile_from_fp(*content, fp, CONTENT_LEN_MAX, false, &nread);
    (void) fclose(fp);
    return true;
}
