/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief HTTP lua script handler
 */

#include "compile_time.h"
#include "src/webserver/scripts.h"

#include "dist/mongoose/mongoose.h"
#include "dist/sds/sds.h"
#include "src/lib/api.h"
#include "src/lib/filehandler.h"
#include "src/lib/sds_extras.h"
#include "src/lib/validate.h"
#include "src/mympd_api/requests.h"
#include "src/scripts/events.h"
#include "src/webserver/utility.h"

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

    struct t_list *arguments = webserver_parse_arguments(hm);

    struct t_work_request *request = create_request(REQUEST_TYPE_DEFAULT, nc->id, 0, INTERNAL_API_SCRIPT_EXECUTE, "", partition);
    struct t_script_execute_data *extra = script_execute_data_new(script, SCRIPT_START_HTTP);
    extra->arguments = arguments;
    request->extra = extra;
    request->extra_free = script_execute_data_free_void;
    FREE_SDS(partition);
    FREE_SDS(script);
    return push_request(request, 0);
}

/**
 * Emits the MYMPD_BGIMAGE trigger.
 * @param nc mongoose connection
 * @param hm http message
 * @return true on success, else false
 */
bool script_execute_bgimage(struct mg_connection *nc, struct mg_http_message *hm) {
    sds partition = sds_urldecode(sdsempty(), hm->uri.buf, hm->uri.len, false);
    partition = sds_basename(partition);
    struct t_list *arguments = webserver_parse_arguments(hm);
    bool rc = mympd_api_request_trigger_event_emit(TRIGGER_MYMPD_BGIMAGE, partition, arguments, nc->id);
    FREE_SDS(partition);
    return rc;
}
