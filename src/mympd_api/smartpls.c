/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "smartpls.h"

#include "../../dist/utf8/utf8.h"
#include "../lib/api.h"
#include "../lib/filehandler.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

sds mympd_api_smartpls_get(sds workdir, sds buffer, long request_id, const char *playlist) {
    enum mympd_cmd_ids cmd_id = MYMPD_API_SMARTPLS_GET;
    sds pl_file = sdscatfmt(sdsempty(), "%S/smartpls/%s", workdir, playlist);
    FILE *fp = fopen(pl_file, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Cant read smart playlist \"%s\"", playlist);
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id, 
            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Can not read smart playlist file");
        FREE_SDS(pl_file);
        return buffer;
    }
    sds content = sdsempty();
    sds_getfile(&content, fp, SMARTPLS_SIZE_MAX, true);
    FREE_SDS(pl_file);
    (void) fclose(fp);

    sds smartpltype = NULL;
    sds sds_buf1 = NULL;
    int int_buf1 = 0;
    int int_buf2 = 0;

    if (json_get_string(content, "$.type", 1, 200, &smartpltype, vcb_isalnum, NULL) == true) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = tojson_char(buffer, "plist", playlist, true);
        buffer = tojson_char(buffer, "type", smartpltype, true);
        bool rc = true;
        if (strcmp(smartpltype, "sticker") == 0) {
            if (json_get_string(content, "$.sticker", 1, 200, &sds_buf1, vcb_isalnum, NULL) == true &&
                json_get_int(content, "$.maxentries", 0, MPD_PLAYLIST_LENGTH_MAX, &int_buf1, NULL) == true &&
                json_get_int(content, "$.minvalue", 0, 100, &int_buf2, NULL) == true)
            {
                buffer = tojson_sds(buffer, "sticker", sds_buf1, true);
                buffer = tojson_long(buffer, "maxentries", int_buf1, true);
                buffer = tojson_long(buffer, "minvalue", int_buf2, true);
            }
            else {
                rc = false;
            }
        }
        else if (strcmp(smartpltype, "newest") == 0) {
            if (json_get_int(content, "$.timerange", 0, JSONRPC_INT_MAX, &int_buf1, NULL) == true) {
                buffer = tojson_long(buffer, "timerange", int_buf1, true);
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
            if (json_get_string(content, "$.sort", 0, 100, &sds_buf1, vcb_ismpdsort, NULL) == true) {
                buffer = tojson_sds(buffer, "sort", sds_buf1, false);
            }
            else {
                buffer = tojson_char_len(buffer, "sort", "", 0, false);
            }
            buffer = jsonrpc_respond_end(buffer);
        }
        else {
            buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
                JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Can not parse smart playlist file");
            MYMPD_LOG_ERROR("Can't parse smart playlist file: %s", playlist);
        }
    }
    else {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Unknown smart playlist type");
        MYMPD_LOG_ERROR("Unknown type for smart playlist \"%s\"", playlist);
    }
    FREE_SDS(smartpltype);
    FREE_SDS(content);
    FREE_SDS(sds_buf1);
    return buffer;
}
