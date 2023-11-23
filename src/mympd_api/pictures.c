/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/pictures.h"

#include "src/lib/api.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mimetype.h"
#include "src/lib/sds_extras.h"

#include <dirent.h>
#include <errno.h>
#include <string.h>

/**
 * Returns a jsonrpc response with pictures in a directory
 * @param workdir myMPD working directory
 * @param buffer sds string to append the response
 * @param request_id jsonrpc request id
 * @param subdir subdirectory in workdir/pics/ to get images from
 * @return pointer to buffer
 */
sds mympd_api_settings_picture_list(sds workdir, sds buffer, unsigned request_id, sds subdir) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_PICTURE_LIST;
    sds pic_dirname = sdscatfmt(sdsempty(), "%S/%s/%S", workdir, DIR_WORK_PICS, subdir);
    errno = 0;
    DIR *pic_dir = opendir(pic_dirname);
    if (pic_dir == NULL) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Can not open directory pics");
        MYMPD_LOG_ERROR(NULL, "Can not open directory \"%s\"", pic_dirname);
        MYMPD_LOG_ERRNO(NULL, errno);
        FREE_SDS(pic_dirname);
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    unsigned returned_entities = 0;
    struct dirent *next_file;
    while ((next_file = readdir(pic_dir)) != NULL ) {
        if (next_file->d_type == DT_REG) {
            if (is_image(next_file->d_name) == true) {
                if (returned_entities++) {
                    buffer = sdscatlen(buffer, ",", 1);
                }
                buffer = sds_catjson(buffer, next_file->d_name, strlen(next_file->d_name));
            }
        }
    }
    closedir(pic_dir);
    FREE_SDS(pic_dirname);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint(buffer, "totalEntities", returned_entities, true);
    buffer = tojson_uint(buffer, "returnedEntities", returned_entities, false);
    buffer = jsonrpc_end(buffer);
    return buffer;
}
