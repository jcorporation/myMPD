/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Scripts thread API handler
 */

#include "compile_time.h"
#include "src/scripts/api_handler.h"

#include "src/lib/config_def.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/scripts/api_scripts.h"
#include "src/scripts/api_vars.h"
#include "src/scripts/scripts_lua.h"
#include "src/scripts/util.h"

/**
 * Central scripts api handler function
 * @param scripts_state pointer to scripts state
 * @param request pointer to the jsonrpc request struct
 */
void scripts_api_handler(struct t_scripts_state *scripts_state, struct t_work_request *request) {
    struct t_jsonrpc_parse_error parse_error;
    jsonrpc_parse_error_init(&parse_error);
    const char *method = get_cmd_id_method_name(request->cmd_id);
    MYMPD_LOG_DEBUG(request->partition, "MYMPD API request (%lu)(%u) %s: %s",
        request->conn_id, request->id, method, request->data);

    //shortcuts
    struct t_config *config = scripts_state->config;

    //some buffer variables
    bool rc;
    sds error = sdsempty();
    sds sds_buf1 = NULL;
    sds sds_buf2 = NULL;
    sds sds_buf3 = NULL;
    sds sds_buf4 = NULL;
    int int_buf1;
    int int_buf2;
    bool bool_buf1;
    bool respond = true;

    //create response struct
    struct t_work_response *response = create_response(request);

    switch(request->cmd_id) {
        case INTERNAL_API_STATE_SAVE:
            scripts_state_save(scripts_state, false);
            response->data = jsonrpc_respond_ok(response->data, request->cmd_id, request->id, JSONRPC_FACILITY_GENERAL);
            break;
        case MYMPD_API_SCRIPT_VALIDATE:
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true &&
                json_get_string(request->data, "$.params.content", 0, CONTENT_LEN_MAX, &sds_buf2, vcb_istext, &parse_error) == true)
            {
                rc = script_validate(config, sds_buf1, sds_buf2, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, error);
            }
        break;
        case MYMPD_API_SCRIPT_SAVE: {
            struct t_list arguments;
            list_init(&arguments);
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true &&
                json_get_string(request->data, "$.params.oldscript", 0, FILENAME_LEN_MAX, &sds_buf2, vcb_isfilename, &parse_error) == true &&
                json_get_string(request->data, "$.params.file", 0, FILENAME_LEN_MAX, &sds_buf3, vcb_isfilepath, &parse_error) == true &&
                json_get_int(request->data, "$.params.order", 0, LIST_TIMER_MAX, &int_buf1, &parse_error) == true &&
                json_get_int(request->data, "$.params.version", 0, LIST_TIMER_MAX, &int_buf2, &parse_error) == true &&
                json_get_string(request->data, "$.params.content", 0, CONTENT_LEN_MAX, &sds_buf4, vcb_istext, &parse_error) == true &&
                json_get_array_string(request->data, "$.params.arguments", &arguments, vcb_isname, SCRIPT_ARGUMENTS_MAX, &parse_error) == true)
            {
                rc = script_validate(config, sds_buf1, sds_buf4, &error) &&
                    script_save(scripts_state, sds_buf1, sds_buf2, sds_buf3, int_buf1, int_buf2, sds_buf4, &arguments, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, error);
            }
            list_clear(&arguments);
            break;
        }
        case MYMPD_API_SCRIPT_RELOAD:
            rc = scripts_file_reload(scripts_state);
            response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, "Could not reload scripts from disk");
            break;
        case MYMPD_API_SCRIPT_RM:
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true) {
                rc = script_delete(scripts_state, sds_buf1);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, "Could not delete script");
            }
            break;
        case MYMPD_API_SCRIPT_GET:
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true) {
                response->data = script_get(&scripts_state->script_list, response->data, request->id, sds_buf1);
            }
            break;
        case MYMPD_API_SCRIPT_LIST: {
            if (json_get_bool(request->data, "$.params.all", &bool_buf1, &parse_error) == true) {
                response->data = script_list(&scripts_state->script_list, response->data, request->id, bool_buf1);
            }
            break;
        }
        case INTERNAL_API_SCRIPT_EXECUTE: {
            struct t_script_execute_data *extra = (struct t_script_execute_data *)request->extra;
            script_start(scripts_state, extra->scriptname, extra->arguments, request->partition,
                    true, extra->script_event, response->id, request->conn_id, &error);
            respond = false;
            script_execute_data_free(extra);
            extra = NULL;
            break;
        }
        case MYMPD_API_SCRIPT_EXECUTE: {
            struct t_list arguments;
            list_init(&arguments);
            if (json_get_string(request->data, "$.params.script", 1, FILENAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true &&
                json_get_string(request->data, "$.params.event", 0, FILENAME_LEN_MAX, &sds_buf2, vcb_isfilename, &parse_error) == true &&
                json_get_object_string(request->data, "$.params.arguments", &arguments, vcb_isname, vcb_isname, 10, &parse_error) == true)
            {
                enum script_start_events script_event = script_start_event_parse(sds_buf2);
                if (script_event == SCRIPT_START_HTTP) {
                    respond = false;
                }
                rc = script_start(scripts_state, sds_buf1, &arguments, request->partition,
                    true, script_event, response->id, request->conn_id, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, error);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Invalid script name");
            }
            list_clear(&arguments);
            break;
        }
        case INTERNAL_API_SCRIPT_POST_EXECUTE: {
            struct t_list arguments;
            list_init(&arguments);
            if (json_get_string(request->data, "$.params.script", 1, CONTENT_LEN_MAX, &sds_buf1, vcb_istext, &parse_error) == true &&
                json_get_object_string(request->data, "$.params.arguments", &arguments, vcb_isname, vcb_isname, 10, &parse_error) == true)
            {
                rc = script_start(scripts_state, sds_buf1, &arguments, request->partition,
                    false, SCRIPT_START_EXTERN, 0, 0, &error);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, error);
            }
            else {
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                        JSONRPC_FACILITY_SCRIPT, JSONRPC_SEVERITY_ERROR, "Invalid script content");
            }
            list_clear(&arguments);
            break;
        }
        case MYMPD_API_SCRIPT_VAR_DELETE:
            if (json_get_string(request->data, "$.params.key", 1, NAME_LEN_MAX, &sds_buf1, vcb_isfilename, &parse_error) == true) {
                rc = scripts_vars_delete(&scripts_state->var_list, sds_buf1);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, "Can't delete script variable");
            }
            break;
        case MYMPD_API_SCRIPT_VAR_LIST:
            response->data = scripts_vars_list(&scripts_state->var_list, response->data, request->id);
            break;
        case MYMPD_API_SCRIPT_VAR_SET:
            if (json_get_string(request->data, "$.params.key", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, &parse_error) == true &&
                json_get_string(request->data, "$.params.value", 1, NAME_LEN_MAX, &sds_buf2, vcb_isname, &parse_error) == true)
            {
                rc = scripts_vars_save(&scripts_state->var_list, sds_buf1, sds_buf2);
                response->data = jsonrpc_respond_with_ok_or_error(response->data, request->cmd_id, request->id, rc,
                        JSONRPC_FACILITY_SCRIPT, "Can't save script variable");
            }
            break;
        // unhandled method
        default:
            response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "Unknown request");
            MYMPD_LOG_ERROR(request->partition, "Unknown API request: %.*s", (int)sdslen(request->data), request->data);
    }

    FREE_SDS(sds_buf1);
    FREE_SDS(sds_buf2);
    FREE_SDS(sds_buf3);
    FREE_SDS(sds_buf4);

    if (respond == true) {
        if (sdslen(response->data) == 0) {
            // error handling
            if (parse_error.message != NULL) {
                // jsonrpc parsing error
                response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, parse_error.message, 2, "path", parse_error.path);
            }
            else if (sdslen(error) > 0) {
                // error from api function
                response->data = jsonrpc_respond_message(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, error);
                MYMPD_LOG_ERROR(request->partition, "Error processing method \"%s\"", method);
            }
            else if (response->type != RESPONSE_TYPE_DISCARD) {
                // no response and no error - this should not occur
                response->data = jsonrpc_respond_message_phrase(response->data, request->cmd_id, request->id,
                    JSONRPC_FACILITY_GENERAL, JSONRPC_SEVERITY_ERROR, "No response for method %{method}", 2, "method", method);
                MYMPD_LOG_ERROR(request->partition, "No response for method \"%s\"", method);
            }
        }
        push_response(response);
    }
    else {
        // response is generated by script
        free_response(response);
    }
    free_request(request);
    FREE_SDS(error);
    jsonrpc_parse_error_clear(&parse_error);
}
