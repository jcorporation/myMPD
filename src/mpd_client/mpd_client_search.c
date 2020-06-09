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
#include "../mpd_shared.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mpd_client_utility.h"
#include "mpd_client_search.h"

//private definitions
static sds _mpd_client_search(t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id,
                      const char *expression, const char *sort, const bool sortdesc, 
                      const char *grouptag, const char *plist, const unsigned int offset,
                      const t_tags *tagcols, bool adv, const char *searchtag);
//public functions
sds mpd_client_search(t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id,
                      const char *searchstr, const char *searchtag, const char *plist, 
                      const unsigned int offset, const t_tags *tagcols)
{
    return _mpd_client_search(mpd_client_state, buffer, method, request_id, 
                              searchstr, NULL, false,
                              NULL, plist, offset,
                              tagcols, false, searchtag);
}

sds mpd_client_search_adv(t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id,
                          const char *expression, const char *sort, const bool sortdesc, 
                          const char *grouptag, const char *plist, const unsigned int offset,
                          const t_tags *tagcols)
{
    return _mpd_client_search(mpd_client_state, buffer, method, request_id, 
                              expression, sort, sortdesc,
                              grouptag, plist, offset,
                              tagcols, true, NULL);
}

//private functions
static sds _mpd_client_search(t_mpd_client_state *mpd_client_state, sds buffer, sds method, int request_id,
                      const char *expression, const char *sort, const bool sortdesc, 
                      const char *grouptag, const char *plist, const unsigned int offset,
                      const t_tags *tagcols, bool adv, const char *searchtag)
{
    if (strcmp(expression, "") == 0) {
        LOG_ERROR("No search expression defined");
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No search expression defined", true);
        return buffer;
    }

    if (strcmp(plist, "") == 0) {
        bool rc = mpd_search_db_songs(mpd_client_state->mpd_state->conn, false);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_db_songs") == false) {
            mpd_search_cancel(mpd_client_state->mpd_state->conn);
            return buffer;
        }
        buffer = jsonrpc_start_result(buffer, method, request_id);
        buffer = sdscat(buffer, ",\"data\":[");
    }
    else if (strcmp(plist, "queue") == 0) {
        bool rc = mpd_search_add_db_songs(mpd_client_state->mpd_state->conn, false);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_db_songs") == false) {
            mpd_search_cancel(mpd_client_state->mpd_state->conn);
            return buffer;
        }
    }
    else {
        bool rc = mpd_search_add_db_songs_to_playlist(mpd_client_state->mpd_state->conn, plist);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_db_songs_to_playlist") == false) {
            mpd_search_cancel(mpd_client_state->mpd_state->conn);
            return buffer;
        }
    }
    
    if (adv == true) {
        bool rc = mpd_search_add_expression(mpd_client_state->mpd_state->conn, expression);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_expression") == false) {
            mpd_search_cancel(mpd_client_state->mpd_state->conn);
            return buffer;
        }
    }
    else if (searchtag != NULL && strcmp(searchtag, "any") == 0) {
        bool rc = mpd_search_add_any_tag_constraint(mpd_client_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, expression);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_any_tag_constraint") == false) {
            mpd_search_cancel(mpd_client_state->mpd_state->conn);
            return buffer;
        }
    }
    else if (searchtag != NULL) {
        bool rc = mpd_search_add_tag_constraint(mpd_client_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(searchtag), expression);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_tag_constraint") == false) {
            mpd_search_cancel(mpd_client_state->mpd_state->conn);
            return buffer;
        }
    }
    else {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        buffer = jsonrpc_respond_message(buffer, method, request_id, "No search tag defined and advanced search is disabled", true);
        return buffer;
    }

    if (strcmp(plist, "") == 0) {
        if (sort != NULL && strcmp(sort, "") != 0 && strcmp(sort, "-") != 0 && mpd_client_state->mpd_state->feat_tags == true) {
            enum mpd_tag_type sort_tag = mpd_tag_name_parse(sort);
            if (sort_tag != MPD_TAG_UNKNOWN) {
                sort_tag = get_sort_tag(sort_tag);
                bool rc = mpd_search_add_sort_tag(mpd_client_state->mpd_state->conn, sort_tag, sortdesc);
                if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_sort_tag") == false) {
                    mpd_search_cancel(mpd_client_state->mpd_state->conn);
                    return buffer;
                }
            }
            else {
                LOG_WARN("Unknown sort tag: %s", sort);
            }
        }
        if (grouptag != NULL && strcmp(grouptag, "") != 0 && mpd_client_state->mpd_state->feat_tags == true) {
            bool rc = mpd_search_add_group_tag(mpd_client_state->mpd_state->conn, mpd_tag_name_parse(grouptag));
            if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_group_tag") == false) {
                mpd_search_cancel(mpd_client_state->mpd_state->conn);
                return buffer;
            }
        }
        if (mpd_client_state->mpd_state->feat_mpd_searchwindow == true) {
            bool rc = mpd_search_add_window(mpd_client_state->mpd_state->conn, offset, offset + mpd_client_state->max_elements_per_page);
            if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_window") == false) {
                mpd_search_cancel(mpd_client_state->mpd_state->conn);
                return buffer;
            }
        }
    }
    
    bool rc = mpd_search_commit(mpd_client_state->mpd_state->conn);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_commit") == false) {
        return buffer;
    }
    
    if (strcmp(plist, "") == 0) {
        struct mpd_song *song;
        unsigned entities_returned = 0;
        unsigned entity_count = 0;
        while ((song = mpd_recv_song(mpd_client_state->mpd_state->conn)) != NULL) {
            entity_count++;
            if (mpd_client_state->mpd_state->feat_mpd_searchwindow == true || (entity_count > offset && entity_count <= offset + mpd_client_state->max_elements_per_page)) {
                if (entities_returned++) {
                    buffer = sdscat(buffer,",");
                }
                buffer = sdscat(buffer, "{");
                buffer = tojson_char(buffer, "Type", "song", true);
                buffer = put_song_tags(buffer, mpd_client_state, tagcols, song);
                buffer = sdscat(buffer, "}");
            }
            mpd_song_free(song);
        }
        buffer = sdscat(buffer, "],");
        if (mpd_client_state->mpd_state->feat_mpd_searchwindow == true) {
            buffer = tojson_long(buffer, "totalEntities", -1, true);
        }
        else {
            buffer = tojson_long(buffer, "totalEntities", entity_count, true);
        }
        buffer = tojson_long(buffer, "offset", offset, true);
        buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
        if (adv == true) {
            buffer = tojson_char(buffer, "expression", expression, true);
            buffer = tojson_char(buffer, "sort", sort, true);
            buffer = tojson_bool(buffer, "sortdesc", sortdesc, true);
            buffer = tojson_char(buffer, "grouptag", grouptag, false);
        }
        else {
            buffer = tojson_char(buffer, "searchstr", expression, true);
            buffer = tojson_char(buffer, "searchtag", searchtag, false);
        }
        buffer = jsonrpc_end_result(buffer);
    }
    else if (strcmp(plist, "queue") == 0) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Added songs to queue", false);
    }
    else {
        buffer = jsonrpc_start_phrase(buffer, method, request_id, "Added songs to %{playlist}", false);
        buffer = tojson_char(buffer, "playlist", plist, false);
        buffer = jsonrpc_end_phrase(buffer);
    }
    
    mpd_response_finish(mpd_client_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
       return buffer;
    }
    return buffer;
}
