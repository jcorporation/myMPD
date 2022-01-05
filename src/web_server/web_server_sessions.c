/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "web_server_sessions.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mympd_pin.h"
#include "../lib/sds_extras.h"
#include "../lib/validate.h"
#include "web_server_utility.h"

#include <string.h>
#include <time.h>

#ifdef ENABLE_SSL
    #include <openssl/rand.h>
#endif

void webserver_session_api(struct mg_connection *nc, enum mympd_cmd_ids cmd_id, sds body, int id, sds session, struct t_mg_user_data *mg_user_data) {
    const char *cmd = get_cmd_id_method_name(cmd_id);
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
                sds ses = webserver_session_new(&mg_user_data->session_list);
                response = jsonrpc_result_start(response, "MYMPD_API_SESSION_LOGIN", 0);
                response = tojson_char(response, "session", ses, false);
                response = jsonrpc_result_end(response);
                FREE_SDS(ses);
            }
            else {
                response = jsonrpc_respond_message(response, "MYMPD_API_SESSION_LOGIN", 0, true, "session", "error", "Invalid pin");
            }
            webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
            FREE_SDS(response);
            break;
        }
        case MYMPD_API_SESSION_LOGOUT: {
            bool rc = false;
            sds response = sdsempty();
            if (sdslen(session) == 20) {
                rc = webserver_session_remove(&mg_user_data->session_list, session);
                if (rc == true) {
                    response = jsonrpc_respond_message(response, "MYMPD_API_SESSION_LOGOUT", 0, false, "session", "info", "Session removed");
                }
            }
            if (rc == false) {
                response = jsonrpc_respond_message(response, "MYMPD_API_SESSION_LOGOUT", 0, true, "session", "error", "Invalid session");
            }
            webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
            FREE_SDS(response);
            break;
        }
        case MYMPD_API_SESSION_VALIDATE: {
            //session is already validated
            sds response = jsonrpc_respond_ok(sdsempty(), "MYMPD_API_SESSION_VALIDATE", 0, "session");
            webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
            FREE_SDS(response);
            break;
        }
        default: {
            sds response = jsonrpc_respond_message(sdsempty(), cmd, id, true,
                "general", "error", "Invalid API request");
            webserver_send_data(nc, response, sdslen(response), "Content-Type: application/json\r\n");
            FREE_SDS(response);
        }
    }
}

sds webserver_session_new(struct t_list *session_list) {
    sds session = sdsempty();
    #ifdef ENABLE_SSL
    unsigned char *buf = malloc_assert(10 * sizeof(unsigned char));
    RAND_bytes(buf, 10);
    for (unsigned i = 0; i < 10; i++) {
        session = sdscatprintf(session, "%02x", buf[i]);
    }
    FREE_PTR(buf);
    #else
    return session;
    #endif
    //timeout old sessions
    webserver_session_validate(session_list, NULL);
    //add new session with 30 min timeout
    list_push(session_list, session, (time(NULL) + HTTP_SESSION_TIMEOUT), NULL, NULL);
    MYMPD_LOG_DEBUG("Created session %s", session);
    //limit sessions to 10
    if (session_list->length > HTTP_SESSIONS_MAX) {
        list_shift(session_list, 0);
        MYMPD_LOG_WARN("To many sessions, discarding oldest session");
    }
    return session;
}

bool webserver_session_validate(struct t_list *session_list, const char *session) {
    time_t now = time(NULL);
    struct t_list_node *current = session_list->head;
    unsigned i = 0;
    while (current != NULL) {
        if (current->value_i < now) {
            MYMPD_LOG_DEBUG("Session %s timed out", current->key);
            struct t_list_node *next = current->next;
            list_shift(session_list, i);
            current = next;
        }
        else {
            //validate session
            if (session != NULL && strcmp(current->key, session) == 0) {
                MYMPD_LOG_DEBUG("Extending session \"%s\"", session);
                current->value_i = time(NULL) + 1800;
                return true;
            }
            //skip to next entrie
            i++;
            current = current->next;
        }
    }
    if (session != NULL) {
        MYMPD_LOG_WARN("Session \"%s\" not found", session);
    }
    return false;
}

bool webserver_session_remove(struct t_list *session_list, const char *session) {
    struct t_list_node *current = session_list->head;
    unsigned i = 0;
    while (current != NULL) {
        if (strcmp(current->key, session) == 0) {
            MYMPD_LOG_DEBUG("Session %s removed", current->key);
            list_shift(session_list, i);
            return true;
        }
        current = current->next;
        i++;
    }
    MYMPD_LOG_DEBUG("Session %s not found", session);
    return false;
}
