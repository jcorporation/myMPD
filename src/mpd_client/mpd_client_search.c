/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mpd_client_utility.h"
#include "mpd_client_search.h"

sds mpd_client_search(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                      const char *searchstr, const char *filter, const char *plist, 
                      const unsigned int offset, const t_tags *tagcols)
{
    if (strcmp(plist, "") == 0) {
        if (mpd_send_command(mpd_state->conn, "search", filter, searchstr, NULL) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
        buffer = jsonrpc_start_result(buffer, method, request_id);
        buffer = sdscat(buffer, ",\"data\":[");
    }
    else if (strcmp(plist, "queue") == 0) {
        if (mpd_send_command(mpd_state->conn, "searchadd", filter, searchstr, NULL) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
    }
    else {
        if (mpd_send_command(mpd_state->conn, "searchaddpl", plist, filter, searchstr, NULL) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
    }
    
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    if (strcmp(plist, "") == 0) {
        struct mpd_song *song;
        unsigned entity_count = 0;
        unsigned entities_returned = 0;
        while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
                if (entities_returned++) {
                    buffer = sdscat(buffer, ",");
                }
                buffer = sdscat(buffer, "{");
                buffer = tojson_char(buffer, "Type", "song", true);
                buffer = put_song_tags(buffer, mpd_state, tagcols, song);
                buffer = sdscat(buffer, "}");
            }
            mpd_song_free(song);
        }
        buffer = sdscat(buffer, "],");
        buffer = tojson_long(buffer, "totalEntities", entity_count, true);
        buffer = tojson_long(buffer, "offset", offset, true);
        buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
        buffer = tojson_char(buffer, "searchstr", searchstr, false);
        buffer = jsonrpc_end_result(buffer);
        

    }
    else {
        mpd_response_finish(mpd_state->conn);
        if (strcmp(plist, "queue") == 0) {
            buffer = jsonrpc_respond_message(buffer, method, request_id, "Added songs to queue", false);
        }
        else {
            buffer = jsonrpc_start_phrase(buffer, method, request_id, "Added songs to %{playlist}", false);
            buffer = tojson_char(buffer, "playlist", plist, false);
            buffer = jsonrpc_end_phrase(buffer);
        }
    }

    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    return buffer;
}


sds mpd_client_search_adv(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                          const char *expression, const char *sort, const bool sortdesc, 
                          const char *grouptag, const char *plist, const unsigned int offset,
                          const t_tags *tagcols)
{
    if (mpd_state->feat_advsearch == false) {
        LOG_ERROR("Advanced search is disabled");
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Advanced search is disabled", true);
        return buffer;
    }

    if (strcmp(expression, "") == 0) {
        LOG_ERROR("No search expression defined");
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No search expression defined", true);
        return buffer;
    }
    if (strcmp(plist, "") == 0) {
        if (mpd_search_db_songs(mpd_state->conn, false) == false) {
            mpd_search_cancel(mpd_state->conn);
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
        buffer = jsonrpc_start_result(buffer, method, request_id);
        buffer = sdscat(buffer, ",\"data\":[");
    }
    else if (strcmp(plist, "queue") == 0) {
        if (mpd_search_add_db_songs(mpd_state->conn, false) == false) {
            mpd_search_cancel(mpd_state->conn);
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
    }
    else {
        if (mpd_search_add_db_songs_to_playlist(mpd_state->conn, plist) == false) {
            mpd_search_cancel(mpd_state->conn);
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
    }
    
    if (mpd_search_add_expression(mpd_state->conn, expression) == false) {
            mpd_search_cancel(mpd_state->conn);
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
    }

    if (strcmp(plist, "") == 0) {
        if (sort != NULL && strcmp(sort, "") != 0 && strcmp(sort, "-") != 0 && mpd_state->feat_tags == true) {
            if (mpd_search_add_sort_name(mpd_state->conn, sort, sortdesc) == false) {
                mpd_search_cancel(mpd_state->conn);
                buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
                return buffer;
            }
        }
        if (grouptag != NULL && strcmp(grouptag, "") != 0 && mpd_state->feat_tags == true) {
            if (mpd_search_add_group_tag(mpd_state->conn, mpd_tag_name_parse(grouptag)) == false) {
                mpd_search_cancel(mpd_state->conn);
                buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
                return buffer;
            }
        }
        if (mpd_search_add_window(mpd_state->conn, offset, offset + mpd_state->max_elements_per_page) == false) {
            mpd_search_cancel(mpd_state->conn);
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
    }
    
    if (mpd_search_commit(mpd_state->conn) == false || check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    
    if (strcmp(plist, "") == 0) {
        struct mpd_song *song;
        unsigned entities_returned = 0;
        while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
            if (entities_returned++) {
                buffer = sdscat(buffer,",");
            }
            buffer = sdscat(buffer, "{");
            buffer = tojson_char(buffer, "Type", "song", true);
            buffer = put_song_tags(buffer, mpd_state, tagcols, song);
            buffer = sdscat(buffer, "}");
            mpd_song_free(song);
        }
        buffer = sdscat(buffer, "],");
        buffer = tojson_long(buffer, "totalEntities", -1, true);
        buffer = tojson_long(buffer, "offset", offset, true);
        buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
        buffer = tojson_char(buffer, "expression", expression, true);
        buffer = tojson_char(buffer, "sort", sort, true);
        buffer = tojson_bool(buffer, "sortdesc", sortdesc, true);
        buffer = tojson_char(buffer, "grouptag", grouptag, false);
        buffer = jsonrpc_end_result(buffer);
        

    }
    else {
        mpd_response_finish(mpd_state->conn);
        if (strcmp(plist, "queue") == 0) {
            buffer = jsonrpc_respond_message(buffer, method, request_id, "Added songs to queue", false);
        }
        else {
            buffer = jsonrpc_start_phrase(buffer, method, request_id, "Added songs to %{playlist}", false);
            buffer = tojson_char(buffer, "playlist", plist, false);
            buffer = jsonrpc_end_phrase(buffer);
        }
    }
    
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
       return buffer;
    }
    return buffer;
}
