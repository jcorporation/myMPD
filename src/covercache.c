/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "../dist/src/sds/sds.h"
#include "sds_extras.h"
#include "list.h"
#include "mympd_config_defs.h"
#include "utility.h"
#include "log.h"
#include "covercache.h"

bool write_covercache_file(t_config *config, const char *uri, const char *mime_type, sds binary) {
    bool rc = false;
    sds filename = sdsnew(uri);
    uri_to_filename(filename);
    sds tmp_file = sdscatfmt(sdsempty(), "%s/covercache/%s.XXXXXX", config->varlibdir, filename);
    int fd = mkstemp(tmp_file);
    if (fd < 0) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write: %s", tmp_file, strerror(errno));
    }
    else {
        FILE *fp = fdopen(fd, "w");
        fwrite(binary, 1, sdslen(binary), fp);
        fclose(fp);
        sds ext = get_ext_by_mime_type(mime_type);
        sds cover_file = sdscatfmt(sdsempty(), "%s/covercache/%s.%s", config->varlibdir, filename, ext);
        if (rename(tmp_file, cover_file) == -1) {
            MYMPD_LOG_ERROR("Rename file from \"%s\" to \"%s\" failed: %s", tmp_file, cover_file, strerror(errno));
            if (unlink(tmp_file) != 0) {
                MYMPD_LOG_ERROR("Error removing file \"%s\": %s", tmp_file, strerror(errno));
            }
        }
        MYMPD_LOG_DEBUG("Write covercache file \"%s\" for uri \"%s\"", cover_file, uri);
        sdsfree(ext);
        sdsfree(cover_file);
        rc = true;
    }
    sdsfree(tmp_file);
    sdsfree(filename);
    return rc;
}
