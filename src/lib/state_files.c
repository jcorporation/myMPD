/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "state_files.h"

#include "log.h"
#include "sds_extras.h"
#include "validate.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

sds camel_to_snake(sds text) {
    sds buffer = sdsempty();
    for (size_t i = 0; i < sdslen(text); i++) {
        if (isupper(text[i]) > 0) {
            buffer = sdscatfmt(buffer, "_%c", tolower((unsigned char)text[i]));
        }
        else {
            buffer = sdscatfmt(buffer, "%c", text[i]);
        }
    }
    return buffer;
}

sds state_file_rw_string_sds(const char *workdir, const char *dir, const char *name, sds old_value, validate_callback vcb, bool warn) {
    sds value = state_file_rw_string(workdir, dir, name, old_value, vcb, warn);
    FREE_SDS(old_value);
    return value;
}

sds state_file_rw_string(const char *workdir, const char *dir, const char *name, const char *def_value, validate_callback vcb, bool warn) {
    sds result = sdsempty();
    sds cfg_file = sdscatfmt(sdsempty(), "%s/%s/%s", workdir, dir, name);
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
    int n = sds_getline(&result, fp, 1000);
    fclose(fp);
    if (n == 0 &&             //sucessfully read the value
        vcb != NULL &&        //has validation callback
        vcb(result) == false) //validation failed, return default
    {
        sdsclear(result);
        result = sdscat(result, def_value);
        return result;
    }
    if (n == -2) {
        //too long line, return default
        sdsclear(result);
        result = sdscat(result, def_value);
    }
    MYMPD_LOG_DEBUG("State %s: %s", name, result);
    return result;
}

bool state_file_rw_bool(const char *workdir, const char *dir, const char *name, const bool def_value, bool warn) {
    bool value = def_value;
    sds line = state_file_rw_string(workdir, dir, name, def_value == true ? "true" : "false", NULL, warn);
    if (sdslen(line) > 0) {
        value = line[0] == 't' ? true : false;
    }
    FREE_SDS(line);
    return value;
}

int state_file_rw_int(const char *workdir, const char *dir, const char *name, const int def_value, const int min, const int max, bool warn) {
    char *crap = NULL;
    int value = def_value;
    sds def_value_str = sdsfromlonglong((long long)def_value);
    sds line = state_file_rw_string(workdir, dir, name, def_value_str, NULL, warn);
    FREE_SDS(def_value_str);
    value = (int)strtoimax(line, &crap, 10);
    FREE_SDS(line);
    if (value >= min && value <= max) {
        return value;
    }
    return def_value;
}

unsigned state_file_rw_uint(const char *workdir, const char *dir, const char *name, const unsigned def_value, const unsigned min, const unsigned max, bool warn) {
    char *crap = NULL;
    unsigned value = def_value;
    sds def_value_str = sdsfromlonglong((long long)def_value);
    sds line = state_file_rw_string(workdir, dir, name, def_value_str, NULL, warn);
    FREE_SDS(def_value_str);
    value = (unsigned)strtoumax(line, &crap, 10);
    FREE_SDS(line);
    if (value >= min && value <= max) {
        return value;
    }
    return def_value;
}

long state_file_rw_long(const char *workdir, const char *dir, const char *name, const long def_value, const long min, const long max, bool warn) {
    char *crap = NULL;
    long value = def_value;
    sds def_value_str = sdsfromlonglong((long long)def_value);
    sds line = state_file_rw_string(workdir, dir, name, def_value_str, NULL, warn);
    FREE_SDS(def_value_str);
    value = strtoimax(line, &crap, 10);
    FREE_SDS(line);
    if (value >= min && value <= max) {
        return value;
    }
    return def_value;
}

bool state_file_write(const char *workdir, const char *dir, const char *name, const char *value) {
    sds tmp_file = sdscatfmt(sdsempty(), "%s/%s/%s.XXXXXX", workdir, dir, name);
    errno = 0;
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", tmp_file);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    int rc = fputs(value, fp);
    if (rc == EOF) {
        MYMPD_LOG_ERROR("Can not write to file \"%s\"", tmp_file);
    }
    fclose(fp);
    sds cfg_file = sdscatfmt(sdsempty(), "%s/%s/%s", workdir, dir, name);
    errno = 0;
    if (rename(tmp_file, cfg_file) == -1) {
        MYMPD_LOG_ERROR("Renaming file from \"%s\" to \"%s\" failed", tmp_file, cfg_file);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(tmp_file);
        FREE_SDS(cfg_file);
        return false;
    }
    FREE_SDS(tmp_file);
    FREE_SDS(cfg_file);
    return true;
}
