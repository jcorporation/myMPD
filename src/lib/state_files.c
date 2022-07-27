/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "state_files.h"

#include "filehandler.h"
#include "log.h"
#include "sds_extras.h"
#include "validate.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
 * @param workdir mympd working directory
 * @param dir subdir 
 * @param name filename to read/write
 * @param def_value default value as sds string (is freed by this function)
 * @param vcb validation callback from validate.h
 * @param warn if true a warning is logged if file does not exists
 * @return newly allocated sds string
 */
sds state_file_rw_string_sds(sds workdir, const char *dir, const char *name, sds def_value, validate_callback vcb, bool warn) {
    sds value = state_file_rw_string(workdir, dir, name, def_value, vcb, warn);
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
 * @param warn if true a warning is logged if file does not exists
 * @return newly allocated sds string
 */
sds state_file_rw_string(sds workdir, const char *dir, const char *name, const char *def_value, validate_callback vcb, bool warn) {
    sds result = sdsempty();
    sds cfg_file = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, dir, name);
    errno = 0;
    FILE *fp = fopen(cfg_file, OPEN_FLAGS_READ);
    if (fp == NULL) {
        if (warn == true) {
            MYMPD_LOG_WARN("Can not open file \"%s\"", cfg_file);
            MYMPD_LOG_ERRNO(errno);
        }
        else if (errno != ENOENT) {
            MYMPD_LOG_ERROR("Can not open file \"%s\"", cfg_file);
            MYMPD_LOG_ERRNO(errno);
        }
        //file does not exist, create it with default value and return
        state_file_write(workdir, dir, name, def_value);
        result = sdscat(result, def_value);
        FREE_SDS(cfg_file);
        return result;
    }
    FREE_SDS(cfg_file);
    int n = sds_getfile(&result, fp, LINE_LENGTH_MAX, true);
    (void) fclose(fp);
    if (n == GETLINE_OK &&    //sucessfully read the value
        vcb != NULL &&        //has validation callback
        vcb(result) == false) //validation failed, return default
    {
        sdsclear(result);
        result = sdscat(result, def_value);
        MYMPD_LOG_ERROR("Validation failed for state \"%s\"", name);
        return result;
    }
    if (n == GETLINE_TOO_LONG) {
        //too long line, return default
        sdsclear(result);
        result = sdscat(result, def_value);
    }
    MYMPD_LOG_DEBUG("State %s: %s", name, result);
    return result;
}

/**
 * Reads a bool from a file or writes the file with a default value if not exists or value is invalid
 * @param workdir mympd working directory
 * @param dir subdir 
 * @param name filename to read/write
 * @param def_value default value
 * @param warn if true a warning is logged if file does not exists
 * @return newly allocated sds string
 */
bool state_file_rw_bool(sds workdir, const char *dir, const char *name, const bool def_value, bool warn) {
    bool value = def_value;
    sds line = state_file_rw_string(workdir, dir, name, def_value == true ? "true" : "false", NULL, warn);
    if (sdslen(line) > 0) {
        value = line[0] == 't' ? true : false;
    }
    FREE_SDS(line);
    return value;
}

/**
 * Reads an integer from a file or writes the file with a default value if not exists or value is invalid
 * @param workdir mympd working directory
 * @param dir subdir 
 * @param name filename to read/write
 * @param def_value default value as sds string (is freed by this function)
 * @param min minimum value
 * @param max maximum value
 * @param warn if true a warning is logged if file does not exists
 * @return newly allocated sds string
 */
int state_file_rw_int(sds workdir, const char *dir, const char *name, const int def_value, const int min, const int max, bool warn) {
    return (int)state_file_rw_long(workdir, dir, name, def_value, min, max, warn);
}

/**
 * Reads a long from a file or writes the file with a default value if not exists or value is invalid
 * @param workdir mympd working directory
 * @param dir subdir 
 * @param name filename to read/write
 * @param def_value default value as sds string (is freed by this function)
 * @param min minimum value
 * @param max maximum value
 * @param warn if true a warning is logged if file does not exists
 * @return newly allocated sds string
 */
long state_file_rw_long(sds workdir, const char *dir, const char *name, const long def_value, const long min, const long max, bool warn) {
    char *crap = NULL;
    sds def_value_str = sdsfromlonglong((long long)def_value);
    sds line = state_file_rw_string(workdir, dir, name, def_value_str, NULL, warn);
    FREE_SDS(def_value_str);
    long value = (long)strtoimax(line, &crap, 10);
    FREE_SDS(line);
    if (value >= min && value <= max) {
        return value;
    }
    return def_value;
}

/**
 * Reads an unsigned from a file or writes the file with a default value if not exists or value is invalid
 * @param workdir mympd working directory
 * @param dir subdir 
 * @param name filename to read/write
 * @param def_value default value as sds string (is freed by this function)
 * @param min minimum value
 * @param max maximum value
 * @param warn if true a warning is logged if file does not exists
 * @return newly allocated sds string
 */
unsigned state_file_rw_uint(sds workdir, const char *dir, const char *name, const unsigned def_value, const unsigned min, const unsigned max, bool warn) {
    char *crap = NULL;
    sds def_value_str = sdsfromlonglong((long long)def_value);
    sds line = state_file_rw_string(workdir, dir, name, def_value_str, NULL, warn);
    FREE_SDS(def_value_str);
    unsigned value = (unsigned)strtoumax(line, &crap, 10);
    FREE_SDS(line);
    if (value >= min && value <= max) {
        return value;
    }
    return def_value;
}

/**
 * Writes the statefile
 * @param workdir mympd working directory
 * @param dir subdir 
 * @param name filename to read/write
 * @param value default value as sds string (is freed by this function)
 * @return true on success else false
 */
bool state_file_write(sds workdir, const char *dir, const char *name, const char *value) {
    sds filepath = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, dir, name);
    bool rc = write_data_to_file(filepath, value, strlen(value));
    FREE_SDS(filepath);
    return rc;
}
