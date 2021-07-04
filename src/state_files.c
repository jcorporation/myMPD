/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <inttypes.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "log.h"
#include "utility.h"
#include "state_files.h"

sds camel_to_snake(const char *text, size_t len) {
    sds buffer = sdsempty();
    for (size_t i = 0;  i < len; i++) {
        if (isupper(text[i]) > 0) {
            buffer = sdscatprintf(buffer, "_%c", tolower((unsigned char)text[i]));
        }
        else {
            buffer = sdscatprintf(buffer, "%c", text[i]);
        }
    }
    return buffer;
}

sds state_file_rw_string_sds(const char *workdir, const char *dir, const char *name, sds old_value, bool warn) {
    sds value = state_file_rw_string(workdir, dir, name, old_value, warn);
    sdsfree(old_value);
    return value;
}

sds state_file_rw_string(const char *workdir, const char *dir, const char *name, const char *def_value, bool warn) {
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    
    sds result = sdsempty();
    
    if (!validate_string(name)) {
        return result;
    }
    
    sds cfg_file = sdscatfmt(sdsempty(), "%s/%s/%s", workdir, dir, name);
    errno = 0;
    FILE *fp = fopen(cfg_file, "r");
    if (fp == NULL) {
        if (warn == true) {
            MYMPD_LOG_WARN("Can not open file \"%s\"", cfg_file);
            MYMPD_LOG_ERRNO(errno);
        }
        else if (errno != ENOENT) {
            MYMPD_LOG_ERROR("Can not open file \"%s\"", cfg_file);
            MYMPD_LOG_ERRNO(errno);
        }
        state_file_write(workdir, dir, name, def_value);
        result = sdscat(result, def_value);
        sdsfree(cfg_file);
        return result;
    }
    sdsfree(cfg_file);
    read = getline(&line, &n, fp);
    if (read > 0) {
        MYMPD_LOG_DEBUG("State %s: %s", name, line);
    }
    fclose(fp);
    if (read > 0) {
        result = sdscat(result, line);
        sdstrim(result, " \n\r");
        FREE_PTR(line);
        return result;
    }
    
    FREE_PTR(line);
    result = sdscat(result, def_value);
    return result;
}

bool state_file_rw_bool(const char *workdir, const char *dir, const char *name, const bool def_value, bool warn) {
    bool value = def_value;
    sds line = state_file_rw_string(workdir, dir, name, def_value == true ? "true" : "false", warn);
    if (sdslen(line) > 0) {
        value = strtobool(line);
        sdsfree(line);
    }
    return value;
}

int state_file_rw_int(const char *workdir, const char *dir, const char *name, const int def_value, bool warn) {
    char *crap = NULL;
    int value = def_value;
    sds def_value_str = sdsfromlonglong(def_value);
    sds line = state_file_rw_string(workdir, dir, name, def_value_str, warn);
    sdsfree(def_value_str);
    if (sdslen(line) > 0) {
        value = strtoimax(line, &crap, 10);
        sdsfree(line);
    }
    return value;
}

unsigned state_file_rw_uint(const char *workdir, const char *dir, const char *name, const unsigned def_value, bool warn) {
    char *crap = NULL;
    unsigned value = def_value;
    sds def_value_str = sdsfromlonglong(def_value);
    sds line = state_file_rw_string(workdir, dir, name, def_value_str, warn);
    sdsfree(def_value_str);
    if (sdslen(line) > 0) {
        value = strtoimax(line, &crap, 10);
        sdsfree(line);
    }
    return value;
}

bool state_file_write(const char *workdir, const char *dir, const char *name, const char *value) {
    if (!validate_string(name)) {
        MYMPD_LOG_ERROR("Invalid filename \"%s\"", name);
        return false;
    }
    sds tmp_file = sdscatfmt(sdsempty(), "%s/%s/%s.XXXXXX", workdir, dir, name);
    errno = 0;
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", tmp_file);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(tmp_file);
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
        sdsfree(tmp_file);
        sdsfree(cfg_file);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(cfg_file);
    return true;
}
