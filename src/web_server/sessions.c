/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/web_server/sessions.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/pin.h"
#include "src/lib/sds_extras.h"
#include "src/lib/validate.h"
#include "src/web_server/utility.h"

#include <openssl/rand.h>
#include <string.h>
#include <time.h>

/**
 * Request handler for the session api
 * @param nc mongoose connection
 * @param cmd_id jsonrpc method
 * @param body http body (jsonrpc request)
 * @param request_id jsonrpc request id
 * @param session session hash
 * @param mg_user_data webserver configuration
 */
void webserver_session_api(struct mg_connection *nc, enum mympd_cmd_ids cmd_id, sds body, unsigned request_id,
        sds session, struct t_mg_user_data *mg_user_data)
{
    switch(cmd_id) {
        case MYMPD_API_SESSION_LOGIN: {
            sds pin = NULL;
            bool is_valid = false;
            if (json_get_string(body, "$.params.pin", 1, 20, &pin, vcb_isalnum, NULL) == true) {
                is_valid = pin_validate(pin, mg_user_data->config->pin_hash);
            }
            FREE_SDS(pin);
            sds response = sdsempty();
            if (is_valid == true) {
                sds new_session = webserver_session_new(&mg_user_data->session_list);
                if (new_session != NULL) {
                    response = jsonrpc_respond_start(response, cmd_id, request_id);
                    response = tojson_sds(response, "session", new_session, false);
                    response = jsonrpc_end(response);
                    FREE_SDS(new_session);
                }
                else {
                    response = jsonrpc_respond_message(response, cmd_id, request_id,
                        JSONRPC_FACILITY_SESSION, JSONRPC_SEVERITY_ERROR, "Could not create session");
                }
            }
            else {
                response = jsonrpc_respond_message(response, cmd_id, request_id,
                    JSONRPC_FACILITY_SESSION, JSONRPC_SEVERITY_ERROR, "Invalid pin");
            }
            webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
            FREE_SDS(response);
            break;
        }
        case MYMPD_API_SESSION_LOGOUT: {
            bool rc = false;
            sds response = sdsempty();
            if (sdslen(session) == 20) {
                rc = webserver_session_remove(&mg_user_data->session_list, session);
                if (rc == true) {
                    response = jsonrpc_respond_message(response, cmd_id, request_id,
                        JSONRPC_FACILITY_SESSION, JSONRPC_SEVERITY_INFO, "Session removed");
                }
            }
            if (rc == false) {
                response = jsonrpc_respond_message(response, cmd_id, request_id,
                    JSONRPC_FACILITY_SESSION, JSONRPC_SEVERITY_ERROR, "Invalid session");
            }
            webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
            FREE_SDS(response);
            break;
        }
        case MYMPD_API_SESSION_VALIDATE: {
            //session is already validated
            sds response = jsonrpc_respond_ok(sdsempty(), cmd_id, request_id, JSONRPC_FACILITY_SESSION);
            webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
            FREE_SDS(response);
            break;
        }
        default: {
            sds response = jsonrpc_respond_message(sdsempty(), cmd_id, request_id,
                JSONRPC_FACILITY_SESSION, JSONRPC_SEVERITY_ERROR, "Invalid API request");
            webserver_send_data(nc, response, sdslen(response), EXTRA_HEADERS_JSON_CONTENT);
            FREE_SDS(response);
        }
    }
}

/**
 * Creates a new session
 * @param session_list the session list
 * @return newly allocated sds string with the session hash or NULL on error
 */
sds webserver_session_new(struct t_list *session_list) {
    unsigned char buf[10];
    if (RAND_bytes((unsigned char *)&buf, sizeof(buf)) != 1) {
        return NULL;
    }
    sds session = sdsempty();
    for (int i = 0; i < 10; i++) {
        session = sdscatprintf(session, "%02x", buf[i]);
    }
    //timeout old sessions
    webserver_session_validate(session_list, NULL);
    //add new session with 30 min timeout
    list_push(session_list, session, (time(NULL) + HTTP_SESSION_TIMEOUT), NULL, NULL);
    MYMPD_LOG_DEBUG(NULL, "Created session %s", session);
    //limit sessions to 10
    if (session_list->length > HTTP_SESSIONS_MAX) {
        list_remove_node(session_list, 0);
        MYMPD_LOG_WARN(NULL, "To many sessions, discarding oldest session");
    }
    return session;
}

/**
 * Validates a session
 * @param session_list the session list 
 * @param session session hash to validate
 * @return true on success, else false
 */
bool webserver_session_validate(struct t_list *session_list, const char *session) {
    time_t now = time(NULL);
    struct t_list_node *current = session_list->head;
    unsigned i = 0;
    while (current != NULL) {
        if (current->value_i < now) {
            MYMPD_LOG_DEBUG(NULL, "Session %s timed out", current->key);
            struct t_list_node *next = current->next;
            list_remove_node(session_list, i);
            current = next;
        }
        else {
            //validate session
            if (session != NULL &&
                strcmp(current->key, session) == 0)
            {
                MYMPD_LOG_DEBUG(NULL, "Extending session \"%s\"", session);
                current->value_i = time(NULL) + 1800;
                return true;
            }
            //skip to next entrie
            i++;
            current = current->next;
        }
    }
    if (session != NULL) {
        MYMPD_LOG_WARN(NULL, "Session \"%s\" not found", session);
    }
    return false;
}

/**
 * 
 * @param session_list the session list
 * @param session session hash to remove
 * @return true on success, else false
 */
bool webserver_session_remove(struct t_list *session_list, const char *session) {
    struct t_list_node *current = session_list->head;
    unsigned i = 0;
    while (current != NULL) {
        if (strcmp(current->key, session) == 0) {
            MYMPD_LOG_DEBUG(NULL, "Session %s removed", current->key);
            list_remove_node(session_list, i);
            return true;
        }
        current = current->next;
        i++;
    }
    MYMPD_LOG_DEBUG(NULL, "Session %s not found", session);
    return false;
}
