/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD smart playlist API
 */

#include "compile_time.h"
#include "src/mympd_api/smartpls.h"

#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"

#include <string.h>

/**
 * Prints smart playlist details
 * @param workdir working directory
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param playlist smart playlist name to print
 * @return pointer to buffer
 */
sds mympd_api_smartpls_get(sds workdir, sds buffer, unsigned request_id, const char *playlist) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SMARTPLS_GET;
    sds pl_file = sdscatfmt(sdsempty(), "%S/%s/%s", workdir, DIR_WORK_SMARTPLS, playlist);
    int nread = 0;
    sds content = sds_getfile(sdsempty(), pl_file, SMARTPLS_SIZE_MAX, true, true, &nread);
    FREE_SDS(pl_file);
    if (nread <= 0) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id, 
            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Can not read smart playlist file");
        FREE_SDS(content);
        return buffer;
    }

    sds smartpltype = NULL;
    sds sds_buf1 = NULL;
    sds sds_buf2 = NULL;
    sds sds_buf3 = NULL;
    int int_buf1 = 0;

    if (json_get_string(content, "$.type", 1, 200, &smartpltype, vcb_isalnum, NULL) == true) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = tojson_char(buffer, "plist", playlist, true);
        buffer = tojson_char(buffer, "type", smartpltype, true);
        unsigned max_entries = 0;
        json_get_uint(content, "$.maxentries", 0, MPD_PLAYLIST_LENGTH_MAX, &max_entries, NULL);
        buffer = tojson_uint(buffer, "maxentries", max_entries, true);
        bool rc = true;
        if (strcmp(smartpltype, "sticker") == 0) {
            if (json_get_string(content, "$.sticker", 1, 200, &sds_buf1, vcb_isname, NULL) == true &&
                json_get_string(content, "$.value", 0, NAME_LEN_MAX, &sds_buf2, vcb_isname, NULL) == true &&
                json_get_string(content, "$.op", 1, STICKER_OP_LEN_MAX, &sds_buf3, vcb_isstickerop, NULL) == true)
            {
                buffer = tojson_sds(buffer, "sticker", sds_buf1, true);
                buffer = tojson_sds(buffer, "value", sds_buf2, true);
                buffer = tojson_sds(buffer, "op", sds_buf3, true);
            }
            else {
                rc = false;
            }
        }
        else if (strcmp(smartpltype, "newest") == 0) {
            if (json_get_int(content, "$.timerange", 0, JSONRPC_INT_MAX, &int_buf1, NULL) == true) {
                buffer = tojson_int(buffer, "timerange", int_buf1, true);
            }
            else {
                rc = false;
            }
        }
        else if (strcmp(smartpltype, "search") == 0) {
            if (json_get_string(content, "$.expression", 1, 200, &sds_buf1, vcb_isname, NULL) == true) {
                buffer = tojson_sds(buffer, "expression", sds_buf1, true);
            }
            else {
                rc = false;
            }
        }
        else {
            rc = false;
        }
        if (rc == true) {
            FREE_SDS(sds_buf1);
            if (json_get_string(content, "$.sort", 0, SORT_LEN_MAX, &sds_buf1, vcb_ismpd_sticker_sort, NULL) == true) {
                buffer = tojson_sds(buffer, "sort", sds_buf1, true);
            }
            else {
                buffer = tojson_char_len(buffer, "sort", "", 0, true);
            }
            bool bool_buf = false;
            json_get_bool(content, "$.sortdesc", &bool_buf, NULL);
            buffer = tojson_bool(buffer, "sortdesc", bool_buf, false);
            buffer = jsonrpc_end(buffer);
        }
        else {
            buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
                JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Can not parse smart playlist file");
            MYMPD_LOG_ERROR(NULL, "Can't parse smart playlist file: %s", playlist);
        }
    }
    else {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Unknown smart playlist type");
        MYMPD_LOG_ERROR(NULL, "Unknown type for smart playlist \"%s\"", playlist);
    }
    FREE_SDS(smartpltype);
    FREE_SDS(content);
    FREE_SDS(sds_buf1);
    FREE_SDS(sds_buf2);
    FREE_SDS(sds_buf3);
    return buffer;
}
