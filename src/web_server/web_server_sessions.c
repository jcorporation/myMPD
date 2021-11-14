/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "web_server_sessions.h"

#include "../lib/log.h"
#include "../lib/mem.h"

#include <string.h>
#include <time.h>

#ifdef ENABLE_SSL
    #include <openssl/rand.h>
#endif

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
