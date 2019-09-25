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
#include <libgen.h>
#include <pthread.h>
#include <mpd/client.h>
#include <inttypes.h>

#include "../dist/src/sds/sds.h"
#include "utility.h"
#include "api.h"
#include "log.h"
#include "config_defs.h"
#include "search.h"
#include "../dist/src/sds/sds.h"

int mpd_client_search(t_mpd_state *mpd_state, char *buffer, const char *searchstr, const char *filter, const char *plist, const unsigned int offset, const t_tags *tagcols) {
    struct mpd_song *song;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    if (strcmp(plist, "") == 0) {
        if (mpd_send_command(mpd_state->conn, "search", filter, searchstr, NULL) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search");
        len = json_printf(&out, "{type: search, data: [");
    }
    else if (strcmp(plist, "queue") == 0) {
        if (mpd_send_command(mpd_state->conn, "searchadd", filter, searchstr, NULL) == false)
            RETURN_ERROR_AND_RECOVER("mpd_searchadd");
    }
    else {
        if (mpd_send_command(mpd_state->conn, "searchaddpl", plist, filter, searchstr, NULL) == false)
            RETURN_ERROR_AND_RECOVER("mpd_searchaddpl");
    }

    if (strcmp(plist, "") == 0) {
        while ((song = mpd_recv_song(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
                if (entities_returned++) 
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{Type: song, ");
                PUT_SONG_TAG_COLS(tagcols);
                len += json_printf(&out, "}");
            }
            mpd_song_free(song);
        }
    }
    else {
        mpd_response_finish(mpd_state->conn);
    }

    if (strcmp(plist, "") == 0) {
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, searchstr: %Q}",
            entity_count,
            offset,
            entities_returned,
            searchstr
        );
    } 
    else {
        len = json_printf(&out, "{type: result, data: ok}");
    }

    CHECK_RETURN_LEN();
}


static int mpd_client_search_adv(t_mpd_state *mpd_state, char *buffer, const char *expression, const char *sort, const bool sortdesc, const char *grouptag, const char *plist, const unsigned int offset, const t_tags *tagcols) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);    
#if LIBMPDCLIENT_CHECK_VERSION(2, 17, 0)
    struct mpd_song *song;
    unsigned entities_returned = 0;
    
    if (strcmp(plist, "") == 0) {
        if (mpd_search_db_songs(mpd_state->conn, false) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_db_songs");
        len = json_printf(&out, "{type: search, data: [");
    }
    else if (strcmp(plist, "queue") == 0) {
        if (mpd_search_add_db_songs(mpd_state->conn, false) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_db_songs");
    }
    else {
        if (mpd_search_add_db_songs_to_playlist(mpd_state->conn, plist) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_db_songs_to_playlist");
    }
    
    if (mpd_search_add_expression(mpd_state->conn, expression) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_expression");

    if (strcmp(plist, "") == 0) {
        if (sort != NULL && strcmp(sort, "") != 0 && strcmp(sort, "-") != 0 && mpd_state->feat_tags == true) {
            if (mpd_search_add_sort_name(mpd_state->conn, sort, sortdesc) == false)
                RETURN_ERROR_AND_RECOVER("mpd_search_add_sort_name");
        }
        if (grouptag != NULL && strcmp(grouptag, "") != 0 && mpd_state->feat_tags == true) {
            if (mpd_search_add_group_tag(mpd_state->conn, mpd_tag_name_parse(grouptag)) == false)
                RETURN_ERROR_AND_RECOVER("mpd_search_add_group_tag");
        }
        if (mpd_search_add_window(mpd_state->conn, offset, offset + mpd_state->max_elements_per_page) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_window");
    }
    
    if (mpd_search_commit(mpd_state->conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");

    if (strcmp(plist, "") == 0) {
        while ((song = mpd_recv_song(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
            if (entities_returned++) 
                len += json_printf(&out, ", ");
            len += json_printf(&out, "{Type: song, ");
            PUT_SONG_TAG_COLS(tagcols);
            len += json_printf(&out, "}");
            mpd_song_free(song);
        }
    }
    else {
        mpd_response_finish(mpd_state->conn);
    }

    if (strcmp(plist, "") == 0) {
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %u, expression: %Q, "
            "sort: %Q, sortdesc: %B, grouptag: %Q}",
            -1,
            offset,
            entities_returned,
            expression,
            sort,
            sortdesc,
            grouptag
        );
    } 
    else {
        len = json_printf(&out, "{type: result, data: ok}");
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
    len = json_printf(&out, "{type: error, data: %Q}", "Advanced search is disabled");
#endif
    CHECK_RETURN_LEN();
}
