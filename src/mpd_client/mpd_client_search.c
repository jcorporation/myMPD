/* myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de> This project's
   homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../utility.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
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
        buffer = sdscat(buffer, "[");
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
        buffer = jsonrpc_respond_ok(buffer, method, request_id);
    }

    return buffer;
}


sds mpd_client_search_adv(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                          const char *expression, const char *sort, const bool sortdesc, 
                          const char *grouptag, const char *plist, const unsigned int offset,
                          const t_tags *tagcols)
{
#if LIBMPDCLIENT_CHECK_VERSION(2, 17, 0)
    if (strcmp(plist, "") == 0) {
        if (mpd_search_db_songs(mpd_state->conn, false) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
        buffer = jsonrpc_start_result(buffer, method, request_id);
        buffer = sdscat(buffer, "[");
    }
    else if (strcmp(plist, "queue") == 0) {
        if (mpd_search_add_db_songs(mpd_state->conn, false) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
    }
    else {
        if (mpd_search_add_db_songs_to_playlist(mpd_state->conn, plist) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
    }
    
    if (mpd_search_add_expression(mpd_state->conn, expression) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
    }

    if (strcmp(plist, "") == 0) {
        if (sort != NULL && strcmp(sort, "") != 0 && strcmp(sort, "-") != 0 && mpd_state->feat_tags == true) {
            if (mpd_search_add_sort_name(mpd_state->conn, sort, sortdesc) == false) {
                buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
                return buffer;
            }
        }
        if (grouptag != NULL && strcmp(grouptag, "") != 0 && mpd_state->feat_tags == true) {
            if (mpd_search_add_group_tag(mpd_state->conn, mpd_tag_name_parse(grouptag)) == false) {
                buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
                return buffer;
            }
        }
        if (mpd_search_add_window(mpd_state->conn, offset, offset + mpd_state->max_elements_per_page) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            return buffer;
        }
    }
    
    if (mpd_search_commit(mpd_state->conn) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
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
        buffer = jsonrpc_respond_ok(buffer, method, request_id);
    }
#else
    //prevent unused warnings
    (void)(mpd_state);
    (void)(expression);
    (void)(sort);
    (void)(sortdesc);
    (void)(grouptag);
    (void)(plist);
    (void)(offset);
    (void)(tagcols);
    buffer = jsonrpc_respond_message(buffer, method, request_id, "Advanced search is disabled", true);
#endif
    return buffer;
}
