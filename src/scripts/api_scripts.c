/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/scripts/api_scripts.h"

#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/scripts/scripts_lua.h"

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

static sds parse_script_metadata(sds buffer, const char *scriptfilename, int *order);

/**
 * Public functions
 */

/**
 * Lists scripts
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param all true = print all scripts, false = print only scripts with order > 0
 * @return pointer to buffer
 */
sds script_list(sds workdir, sds buffer, unsigned request_id, bool all) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SCRIPT_LIST;
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    sds scriptdirname = sdscatfmt(sdsempty(), "%S/%s", workdir, DIR_WORK_SCRIPTS);
    errno = 0;
    DIR *script_dir = opendir(scriptdirname);
    if (script_dir == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can not open directory \"%s\"", scriptdirname);
        MYMPD_LOG_ERRNO(NULL, errno);
        FREE_SDS(scriptdirname);
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Can not open script directory");
        return buffer;
    }

    struct dirent *next_file;
    unsigned returned_entities = 0;
    sds entry = sdsempty();
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
        entry = sdscatlen(entry, "{", 1);
        entry = tojson_char(entry, "name", scriptname, true);
        scriptfilename = sdscatfmt(scriptfilename, "%S/%s", scriptdirname, next_file->d_name);
        int order = 0;
        entry = parse_script_metadata(entry, scriptfilename, &order);
        entry = sdscatlen(entry, "}", 1);
        if (all == true ||
            order > 0)
        {
            if (returned_entities++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscat(buffer, entry);
        }
        sdsclear(entry);
        sdsclear(scriptname);
        sdsclear(scriptfilename);
    }
    closedir(script_dir);
    FREE_SDS(scriptname);
    FREE_SDS(scriptfilename);
    FREE_SDS(entry);
    FREE_SDS(scriptdirname);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint(buffer, "returnedEntities", returned_entities, true);
    buffer = tojson_uint(buffer, "totalEntities", returned_entities, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}

/**
 * Deletes a script
 * @param workdir working directory
 * @param script script to delete (name without extension)
 * @return true on success, else false
 */
bool script_delete(sds workdir, sds script) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", workdir, DIR_WORK_SCRIPTS, script);
    bool rc = rm_file(filepath);
    FREE_SDS(filepath);
    return rc;
}

/**
 * Saves a script
 * @param workdir working directory
 * @param script scriptname
 * @param oldscript old scriptname (leave empty for a new script)
 * @param order script list is order by this value
 * @param content script content
 * @param arguments arguments for the script
 * @param error already allocated sds string to hold the error message
 * @return true on success, else false
 */
bool script_save(sds workdir, sds script, sds oldscript, int order, sds content, struct t_list *arguments, sds *error) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", workdir, DIR_WORK_SCRIPTS, script);
    sds argstr = list_to_json_array(sdsempty(), arguments);
    sds script_content = sdscatfmt(sdsempty(), "-- {\"order\":%i,\"arguments\":%S}\n%S", order, argstr, content);
    bool rc = write_data_to_file(filepath, script_content, sdslen(script_content));
    //delete old scriptfile
    if (rc == true &&
        sdslen(oldscript) > 0 &&
        strcmp(script, oldscript) != 0)
    {
        sds old_filepath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", workdir, DIR_WORK_SCRIPTS, oldscript);
        rc = rm_file(old_filepath);
        FREE_SDS(old_filepath);
    }
    FREE_SDS(argstr);
    FREE_SDS(script_content);
    FREE_SDS(filepath);
    if (rc == false) {
        *error = sdscat(*error, "Could not save script");
    }
    return rc;
}

/**
 * Gets the script and its details
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param script scriptname to read from filesystem
 * @return pointer to buffer
 */
sds script_get(sds workdir, sds buffer, unsigned request_id, sds script) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SCRIPT_GET;
    sds scriptfilename = sdscatfmt(sdsempty(), "%S/%s/%S.lua", workdir, DIR_WORK_SCRIPTS, script);
    errno = 0;
    FILE *fp = fopen(scriptfilename, OPEN_FLAGS_READ);
    if (fp != NULL) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = tojson_sds(buffer, "script", script, true);
        int nread = 0;
        sds line = sds_getline(sdsempty(), fp, LINE_LENGTH_MAX, &nread);
        if (nread >= 0 &&
            strncmp(line, "-- ", 3) == 0)
        {
            sdsrange(line, 3, -1);
            if (line[0] == '{' &&
                line[sdslen(line) - 1] == '}')
            {
                buffer = sdscat(buffer, "\"metadata\":");
                buffer = sdscat(buffer, line);
            }
            else {
                MYMPD_LOG_WARN(NULL, "Invalid metadata for script %s", scriptfilename);
                buffer = sdscat(buffer, "\"metadata\":{\"order\":0, \"arguments\":[]}");
            }
        }
        else {
            MYMPD_LOG_WARN(NULL, "Invalid metadata for script %s", scriptfilename);
            buffer = sdscat(buffer, "\"metadata\":{\"order\":0, \"arguments\":[]}");
        }
        FREE_SDS(line);
        buffer = sdscat(buffer, ",\"content\":");
        nread = 0;
        sds content = sds_getfile_from_fp(sdsempty(), fp, SCRIPTS_SIZE_MAX, false, &nread);
        (void) fclose(fp);
        buffer = sds_catjson(buffer, content, sdslen(content));
        FREE_SDS(content);
        buffer = jsonrpc_end(buffer);
    }
    else {
        MYMPD_LOG_ERROR(NULL, "Can not open file \"%s\"", scriptfilename);
        MYMPD_LOG_ERRNO(NULL, errno);
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Can not open scriptfile");
    }
    FREE_SDS(scriptfilename);

    return buffer;
}

/**
 * Private functions
 */

/**
 * Parses the script metadata line.
 * The metadata line is the first line of the scriptfile.
 * @param buffer already allocated sds string to append the metadata
 * @param scriptfilename file to read
 * @param order pointer to int to populate with order
 * @return pointer to buffer
 */
static sds parse_script_metadata(sds buffer, const char *scriptfilename, int *order) {
    errno = 0;
    FILE *fp = fopen(scriptfilename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can not open file \"%s\"", scriptfilename);
        MYMPD_LOG_ERRNO(NULL, errno);
        return buffer;
    }

    int nread = 0;
    sds line = sds_getline(sdsempty(), fp, LINE_LENGTH_MAX, &nread);
    if (nread >= 0 &&
        strncmp(line, "-- ", 3) == 0)
    {
        sdsrange(line, 3, -1);
        if (json_get_int(line, "$.order", 0, 99, order, NULL) == true) {
            buffer = sdscat(buffer, "\"metadata\":");
            buffer = sdscatsds(buffer, line);
        }
        else {
            MYMPD_LOG_WARN(NULL, "Invalid metadata for script %s", scriptfilename);
            buffer = sdscat(buffer, "\"metadata\":{\"order\":0,\"arguments\":[]}");
            *order = 0;
        }
    }
    else {
        MYMPD_LOG_WARN(NULL, "Invalid metadata for script %s", scriptfilename);
        buffer = sdscat(buffer, "\"metadata\":{\"order\":0,\"arguments\":[]}");
        *order = 0;
    }
    FREE_SDS(line);
    (void) fclose(fp);
    return buffer;
}
