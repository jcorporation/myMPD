/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_shared_search.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "mpd_shared_sticker.h"
#include "mpd_shared_tags.h"

#include <string.h>

//private definitions
static sds _mpd_shared_search(struct t_mpd_state *mpd_state, sds buffer, sds method, long request_id,
                      const char *expression, const char *sort, const bool sortdesc,
                      const char *plist, unsigned to, unsigned whence,
                      const unsigned offset, unsigned limit, const struct t_tags *tagcols, bool adv,
                      const char *searchtag, rax *sticker_cache, bool *result);
//public functions
sds mpd_shared_search(struct t_mpd_state *mpd_state, sds buffer, sds method, long request_id,
                      const char *searchstr, const char *searchtag, const char *plist,
                      const unsigned offset, unsigned limit, const struct t_tags *tagcols,
                      rax *sticker_cache, bool *result)
{
    return _mpd_shared_search(mpd_state, buffer, method, request_id,
                              searchstr, NULL, false, plist, 0, 0, offset, limit,
                              tagcols, false, searchtag, sticker_cache, result);
}

sds mpd_shared_search_adv(struct t_mpd_state *mpd_state, sds buffer, sds method, long request_id,
                          const char *expression, const char *sort, const bool sortdesc,
                          const char *plist, unsigned to, unsigned whence,
                          const unsigned offset, unsigned limit, const struct t_tags *tagcols,
                          rax *sticker_cache, bool *result)
{
    return _mpd_shared_search(mpd_state, buffer, method, request_id,
                              expression, sort, sortdesc, plist, to, whence, offset, limit,
                              tagcols, true, NULL, sticker_cache, result);
}


sds escape_mpd_search_expression(sds buffer, const char *tag, const char *operator, const char *value) {
    buffer = sdscatfmt(buffer, "(%s %s '", tag, operator);
    for (size_t i = 0;  i < strlen(value); i++) {
        if (value[i] == '\\' || value[i] == '\'') {
            buffer = sdscatlen(buffer, "\\", 1);
        }
        buffer = sdscatprintf(buffer, "%c", value[i]);
    }
    buffer = sdscatlen(buffer, "')", 2);
    return buffer;
}

//private functions
static sds _mpd_shared_search(struct t_mpd_state *mpd_state, sds buffer, sds method, long request_id,
                      const char *expression, const char *sort, const bool sortdesc, const char *plist,
                      unsigned to, unsigned whence, const unsigned offset, unsigned limit,
                      const struct t_tags *tagcols, bool adv, const char *searchtag, rax *sticker_cache,
                      bool *result)
{
    *result = false;
    if (strcmp(expression, "") == 0) {
        MYMPD_LOG_ERROR("No search expression defined");
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "mpd", "error", "No search expression defined");
        return buffer;
    }

    if (plist == NULL) {
        //show search results
        bool rc = mpd_search_db_songs(mpd_state->conn, false);
        if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_search_db_songs") == false) {
            mpd_search_cancel(mpd_state->conn);
            return buffer;
        }
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = sdscat(buffer, "\"data\":[");
    }
    else if (strcmp(plist, "queue") == 0) {
        //add search to queue
        bool rc = mpd_search_add_db_songs(mpd_state->conn, false);
        if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_db_songs") == false) {
            mpd_search_cancel(mpd_state->conn);
            return buffer;
        }
    }
    else {
        //add search to playlist
        bool rc = mpd_search_add_db_songs_to_playlist(mpd_state->conn, plist);
        if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_db_songs_to_playlist") == false) {
            mpd_search_cancel(mpd_state->conn);
            return buffer;
        }
    }

    if (adv == true) {
        bool rc = mpd_search_add_expression(mpd_state->conn, expression);
        if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_expression") == false) {
            mpd_search_cancel(mpd_state->conn);
            return buffer;
        }
    }
    else if (searchtag != NULL && strcmp(searchtag, "any") == 0) {
        bool rc = mpd_search_add_any_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, expression);
        if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_any_tag_constraint") == false) {
            mpd_search_cancel(mpd_state->conn);
            return buffer;
        }
    }
    else if (searchtag != NULL) {
        bool rc = mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(searchtag), expression);
        if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_tag_constraint") == false) {
            mpd_search_cancel(mpd_state->conn);
            return buffer;
        }
    }
    else {
        mpd_search_cancel(mpd_state->conn);
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "mpd", "error", "No search tag defined and advanced search is disabled");
        return buffer;
    }

    if (plist == NULL ||
        mpd_connection_cmp_server_version(mpd_state->conn, 0, 22, 0) >= 0)
    {
        if (sort != NULL &&
            strcmp(sort, "") != 0 &&
            strcmp(sort, "-") != 0 &&
            mpd_state->feat_mpd_tags == true)
        {
            enum mpd_tag_type sort_tag = mpd_tag_name_parse(sort);
            if (sort_tag != MPD_TAG_UNKNOWN) {
                sort_tag = get_sort_tag(sort_tag);
                bool rc = mpd_search_add_sort_tag(mpd_state->conn, sort_tag, sortdesc);
                if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_sort_tag") == false) {
                    mpd_search_cancel(mpd_state->conn);
                    return buffer;
                }
            }
            else if (strcmp(sort, "LastModified") == 0) {
                bool rc = mpd_search_add_sort_name(mpd_state->conn, "Last-Modified", sortdesc);
                if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_sort_name") == false) {
                    mpd_search_cancel(mpd_state->conn);
                    return buffer;
                }
            }
            else {
                MYMPD_LOG_WARN("Unknown sort tag: %s", sort);
            }
        }

        unsigned real_limit = limit == 0 ? offset + MPD_PLAYLIST_LENGTH_MAX : offset + limit;
        bool rc = mpd_search_add_window(mpd_state->conn, offset, real_limit);
        if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_window") == false) {
            mpd_search_cancel(mpd_state->conn);
            return buffer;
        }
    }

    if (mpd_state->feat_mpd_whence == true &&
        plist != NULL &&
        to < UINT_MAX)
    {
        //to = UINT_MAX is append
        bool rc = mpd_search_add_position(mpd_state->conn, to, whence);
        if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_position") == false) {
            mpd_search_cancel(mpd_state->conn);
            return buffer;
        }
    }

    bool rc = mpd_search_commit(mpd_state->conn);
    if (check_rc_error_and_recover(mpd_state, &buffer, method, request_id, false, rc, "mpd_search_commit") == false) {
        return buffer;
    }

    if (plist == NULL) {
        struct mpd_song *song;
        unsigned entities_returned = 0;
        while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_char(buffer, "Type", "song", true);
            buffer = get_song_tags(buffer, mpd_state, tagcols, song);
            if (sticker_cache != NULL) {
                buffer = sdscatlen(buffer, ",", 1);
                buffer = mpd_shared_sticker_list(buffer, sticker_cache, mpd_song_get_uri(song));
            }
            buffer = sdscatlen(buffer, "}", 1);
            mpd_song_free(song);
        }
        buffer = sdscatlen(buffer, "],", 2);
        if (offset == 0 &&
            entities_returned < limit)
        {
            buffer = tojson_long(buffer, "totalEntities", entities_returned, true);
        }
        else {
            buffer = tojson_long(buffer, "totalEntities", -1, true);
        }
        buffer = tojson_long(buffer, "offset", offset, true);
        buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
        if (adv == true) {
            buffer = tojson_char(buffer, "expression", expression, true);
            buffer = tojson_char(buffer, "sort", sort, true);
            buffer = tojson_bool(buffer, "sortdesc", sortdesc, false);
        }
        else {
            buffer = tojson_char(buffer, "searchstr", expression, true);
            buffer = tojson_char(buffer, "searchtag", searchtag, false);
        }
        buffer = jsonrpc_result_end(buffer);
    }
    else if (strcmp(plist, "queue") == 0) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, false,
            "queue", "info", "Added songs to queue");
    }
    else {
        buffer = jsonrpc_respond_message_phrase(buffer, method, request_id, false,
            "playlist", "info", "Added songs to %{playlist}", 2, "playlist", plist);
    }

    mpd_response_finish(mpd_state->conn);
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
       return buffer;
    }
    *result = true;
    return buffer;
}
