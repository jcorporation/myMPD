/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief HTTP lua script handler
 */

#include "compile_time.h"
#include "src/web_server/scripts.h"

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/sds_extras.h"
#include "src/lib/validate.h"
#include "src/scripts/events.h"
#include "src/web_server/utility.h"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <pthread.h>

/**
 * Executes the script as request handler.
 * @param nc mongoose connection
 * @param hm http message
 * @param config Pointer to config
 * @return true on success, else false
 */
bool script_execute_http(struct mg_connection *nc, struct mg_http_message *hm, struct t_config *config) {
    sds partition = sds_urldecode(sdsempty(), hm->uri.buf, hm->uri.len, false);
    sds script = sdsdup(partition);
    partition = sds_dirname(partition);
    partition = sds_basename(partition);
    script = sds_basename(script);
    if (vcb_isfilepath(script) == false) {
        FREE_SDS(script);
        FREE_SDS(partition);
        webserver_send_error(nc, 500, "Invalid script name");
        return false;
    }
    sds script_fullpath = sdscatfmt(sdsempty(), "%S/%s/%S.lua", config->workdir, DIR_WORK_SCRIPTS, script);
    bool script_exists = testfile_read(script_fullpath);
    FREE_SDS(script_fullpath);
    if (script_exists == false) {
        FREE_SDS(script);
        FREE_SDS(partition);
        webserver_send_error(nc, 500, "Script not found");
        return false;
    }

    //Extract arguments from query parameters
    struct t_list *arguments = list_new();
    int params_count;
    sds *params = sdssplitlen(hm->query.buf, (ssize_t)hm->query.len, "&", 1, &params_count);
    for (int i = 0; i < params_count; i++) {
        int kv_count;
        sds *kv = sdssplitlen(params[i], (ssize_t)sdslen(params[i]), "=", 1, &kv_count);
        if (kv_count == 2) {
            sds decoded = sds_urldecode(sdsempty(), kv[1], sdslen(kv[1]), false);
            list_push(arguments, kv[0], 0, decoded, NULL);
            FREE_SDS(decoded);
        }
        sdsfreesplitres(kv, kv_count);
    }
    sdsfreesplitres(params, params_count);

    struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, nc->id, 0, INTERNAL_API_SCRIPT_EXECUTE, "", partition);
    struct t_script_execute_data *extra = script_execute_data_new(script, SCRIPT_START_HTTP);
    extra->arguments = arguments;
    request->extra = extra;
    FREE_SDS(partition);
    FREE_SDS(script);
    return push_request(request, 0);
}
