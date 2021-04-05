/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <libgen.h>
#include <dirent.h>
#include <stdbool.h>
#include <inttypes.h>

#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "../dist/src/rax/rax.h"
#include "sds_extras.h"
#include "log.h"
#include "list.h"
#include "mympd_config_defs.h"
#include "mympd_state.h"
#include "utility.h"
#include "state_files.h"

sds state_file_rw_string(struct t_config *config, const char *name, const char *def_value, bool warn) {
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    
    sds result = sdsempty();
    
    if (!validate_string(name)) {
        return result;
    }
    
    sds cfg_file = sdscatfmt(sdsempty(), "%s/state/%s", config->workdir, name);
    FILE *fp = fopen(cfg_file, "r");
    if (fp == NULL) {
        if (warn == true) {
            MYMPD_LOG_WARN("Can not open file \"%s\": %s", cfg_file, strerror(errno));
        }
        else if (errno != ENOENT) {
            MYMPD_LOG_ERROR("Can not open file \"%s\": %s", cfg_file, strerror(errno));
        }
        state_file_write(config, name, def_value);
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

bool state_file_rw_bool(struct t_config *config, const char *name, const bool def_value, bool warn) {
    bool value = def_value;
    sds line = state_file_rw_string(config, name, def_value == true ? "true" : "false", warn);
    if (sdslen(line) > 0) {
        value = strtobool(line);
        sdsfree(line);
    }
    return value;
}

int state_file_rw_int(struct t_config *config, const char *name, const int def_value, bool warn) {
    char *crap = NULL;
    int value = def_value;
    sds def_value_str = sdsfromlonglong(def_value);
    sds line = state_file_rw_string(config, name, def_value_str, warn);
    sdsfree(def_value_str);
    if (sdslen(line) > 0) {
        value = strtoimax(line, &crap, 10);
        sdsfree(line);
    }
    return value;
}

bool state_file_write(struct t_config *config, const char *name, const char *value) {
    if (!validate_string(name)) {
        return false;
    }
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/%s.XXXXXX", config->workdir, name);
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write: %s", tmp_file, strerror(errno));
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    int rc = fputs(value, fp);
    if (rc == EOF) {
        MYMPD_LOG_ERROR("Can not write to file \"%s\"", tmp_file);
    }
    fclose(fp);
    sds cfg_file = sdscatfmt(sdsempty(), "%s/state/%s", config->workdir, name);
    if (rename(tmp_file, cfg_file) == -1) {
        MYMPD_LOG_ERROR("Renaming file from \"%s\" to \"%s\" failed: %s", tmp_file, cfg_file, strerror(errno));
        sdsfree(tmp_file);
        sdsfree(cfg_file);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(cfg_file);
    return true;
}
