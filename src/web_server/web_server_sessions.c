/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "web_server_sessions.h"

#include "../log.h"

#include <assert.h>
#include <time.h>

#ifdef ENABLE_SSL
    #include <openssl/rand.h>
#endif

sds new_session(struct list *session_list) {
    sds session = sdsempty();
    #ifdef ENABLE_SSL
    unsigned char *buf = malloc(10 * sizeof(unsigned char));
    assert(buf);
    RAND_bytes(buf, 10);
    for (unsigned i = 0; i < 10; i++) {
        session = sdscatprintf(session, "%02x", buf[i]);
    }
    FREE_PTR(buf);
    #else
    return session;
    #endif
    //timeout old sessions
    //struct    
    //add new session with 30 min timeout
    list_push(session_list, session, (time(NULL) + 1800), NULL, NULL);
    //limit sessions to 10 
    if (session_list->length > 10) {
        list_shift(session_list, 0);
        MYMPD_LOG_WARN("To many sessions, discarding oldest session");
    }
    return session;
}
