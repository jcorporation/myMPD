/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/lib/state_files.h"

#include "src/lib/convert.h"
#include "src/lib/filehandler.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>


/**
 * Checks if the state dir for a partition exists
 * @param workdir myMPD working directory
 * @param partition partition name
 * @return true on success, else false
 */
bool check_partition_state_dir(sds workdir, sds partition) {
    sds partition_dir = sdsdup(partition);
    sanitize_filename(partition_dir);
    sds state_dir_name = sdscatfmt(sdsempty(), "%S/%s/%S", workdir, DIR_WORK_STATE, partition_dir);
    DIR *state_dir = opendir(state_dir_name);
    FREE_SDS(partition_dir);
    FREE_SDS(state_dir_name);
    if (state_dir == NULL) {
        return false;
    }
    closedir(state_dir);
    return true;
}

/**
 * Converts camel case to snake notation
 * @param text string to convert
 * @return newly allocated sds string in snake notation
 */
sds camel_to_snake(sds text) {
    //pre-allocate buffer to avoid continuous reallocations 
    sds buffer = sdsempty();
    buffer = sdsMakeRoomFor(buffer, sdslen(text) + 5);
    for (size_t i = 0; i < sdslen(text); i++) {
        if (isupper(text[i]) > 0) {
            buffer = sdscatfmt(buffer, "_%c", tolower((unsigned char)text[i]));
        }
        else {
            buffer = sds_catchar(buffer, text[i]);
        }
    }
    return buffer;
}

/**
 * Reads a string from a file or writes the file with a default value if not exists or value is invalid
 * Frees the default value.
 * @param workdir mympd working directory
 * @param dir subdir 
 * @param name filename to read/write
 * @param def_value default value as sds string (is freed by this function)
 * @param vcb validation callback from validate.h
 * @param write if true create the file if not exists
 * @return newly allocated sds string
 */
sds state_file_rw_string_sds(sds workdir, const char *dir, const char *name, sds def_value, validate_callback vcb, bool write) {
    sds value = state_file_rw_string(workdir, dir, name, def_value, vcb, write);
    FREE_SDS(def_value);
    return value;
}

/**
 * Reads a string from a file or writes the file with a default value if not exists or value is invalid
 * @param workdir mympd working directory
 * @param dir subdir 
 * @param name filename to read/write
 * @param def_value default value as c string
 * @param vcb validation callback from validate.h
 * @param write if true create the file if not exists
 * @return newly allocated sds string
 */
sds state_file_rw_string(sds workdir, const char *dir, const char *name, const char *def_value,
        validate_callback vcb, bool write)
{
    sds result = sdsempty();
    sds cfg_file = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, dir, name);
    errno = 0;
    FILE *fp = fopen(cfg_file, OPEN_FLAGS_READ);
    if (fp == NULL) {
        if (errno != ENOENT) {
            MYMPD_LOG_ERROR(NULL, "Can not open file \"%s\"", cfg_file);
            MYMPD_LOG_ERRNO(NULL, errno);
        }
        if (write == true) {
            //file does not exist, create it with default value and return
            state_file_write(workdir, dir, name, def_value);
        }
        result = sdscat(result, def_value);
        FREE_SDS(cfg_file);
        return result;
    }
    FREE_SDS(cfg_file);
    //file exists - read the value
    int nread = 0;
    result = sds_getfile_from_fp(result, fp, LINE_LENGTH_MAX, true, &nread);
    (void) fclose(fp);
    if (nread > 0 &&              //successfully read the value
        vcb != NULL &&        //has validation callback
        vcb(result) == false) //validation failed, return default
    {
        sdsclear(result);
        result = sdscat(result, def_value);
        MYMPD_LOG_ERROR(NULL, "Validation failed for state \"%s\"", name);
        return result;
    }
    if (nread <= 0) {
        //error reading state file, use default
        sdsclear(result);
        result = sdscat(result, def_value);
    }
    MYMPD_LOG_DEBUG(NULL, "State %s: %s", name, result);
    return result;
}

/**
 * Reads a bool from a file or writes the file with a default value if not exists or value is invalid
 * @param workdir mympd working directory
 * @param dir subdir 
 * @param name filename to read/write
 * @param def_value default value
 * @param write if true create the file if not exists
 * @return newly allocated sds string
 */
bool state_file_rw_bool(sds workdir, const char *dir, const char *name, bool def_value, bool write) {
    bool value = def_value;
    sds line = state_file_rw_string(workdir, dir, name, def_value == true ? "true" : "false", NULL, write);
    if (sdslen(line) > 0) {
        value = line[0] == 't'
            ? true
            : false;
    }
    FREE_SDS(line);
    return value;
}

/**
 * Reads a tag name from a file, parses it to a mpd_tag_type or writes the file with a default value if not exists or value is invalid
 * @param workdir mympd working directory
 * @param dir subdir
 * @param name filename to read/write
 * @param def_value default value as mpd_tag_type
 * @param write if true create the file if not exists
 * @return parsed string as mpd_tag_type
 */
enum mpd_tag_type state_file_rw_tag(sds workdir, const char *dir, const char *name, enum mpd_tag_type def_value, bool write) {
    sds line = state_file_rw_string(workdir, dir, name, mpd_tag_name(def_value), NULL, write);
    enum mpd_tag_type value = sdslen(line) > 0
        ? mpd_tag_name_iparse(line)
        : def_value;
    FREE_SDS(line);
    return value == MPD_TAG_UNKNOWN
        ? def_value
        : value;
}

/**
 * Reads an int value from a file or writes the file with a default value if not exists or value is invalid
 * @param workdir mympd working directory
 * @param dir subdir 
 * @param name filename to read/write
 * @param def_value default value as sds string (is freed by this function)
 * @param min minimum value
 * @param max maximum value
 * @param write if true create the file if not exists
 * @return newly allocated sds string
 */
int state_file_rw_int(sds workdir, const char *dir, const char *name, int def_value, int min, int max, bool write) {
    sds def_value_str = sdsfromlonglong((long long)def_value);
    sds line = state_file_rw_string(workdir, dir, name, def_value_str, NULL, write);
    FREE_SDS(def_value_str);
    int value;
    enum str2int_errno rc = str2int(&value, line);
    FREE_SDS(line);
    if (rc != STR2INT_SUCCESS) {
        return def_value;
    }
    if (value >= min && value <= max) {
        return value;
    }
    return def_value;
}

/**
 * Reads an unsigned value from a file or writes the file with a default value if not exists or value is invalid
 * @param workdir mympd working directory
 * @param dir subdir 
 * @param name filename to read/write
 * @param def_value default value as sds string (is freed by this function)
 * @param min minimum value
 * @param max maximum value
 * @param write if true create the file if not exists
 * @return newly allocated sds string
 */
unsigned state_file_rw_uint(sds workdir, const char *dir, const char *name, unsigned def_value, unsigned min, unsigned max, bool write) {
    sds def_value_str = sdsfromlonglong((long long)def_value);
    sds line = state_file_rw_string(workdir, dir, name, def_value_str, NULL, write);
    FREE_SDS(def_value_str);
    unsigned value;
    enum str2int_errno rc = str2uint(&value, line);
    FREE_SDS(line);
    if (rc != STR2INT_SUCCESS) {
        return def_value;
    }
    if (value >= min && value <= max) {
        return value;
    }
    return def_value;
}

/**
 * Writes the statefile
 * @param workdir mympd working directory
 * @param subdir subdir
 * @param filename filename to write
 * @param value value to write fo file
 * @return true on success else false
 */
bool state_file_write(sds workdir, const char *subdir, const char *filename, const char *value) {
    sds state_dir = sdscatfmt(sdsempty(), "%S/%s", workdir, subdir);
    bool rc = false;
    if (testdir(subdir, state_dir, true, true) < 2) {
        //dir exists or was created, write state file
        sds filepath = sdscatfmt(sdsempty(), "%S/%s", state_dir, filename);
        rc = write_data_to_file(filepath, value, strlen(value));
        FREE_SDS(filepath);
    }
    FREE_SDS(state_dir);
    return rc;
}
