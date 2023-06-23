/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"

#include "src/mpd_client/search.h"

#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/sticker.h"

#include <string.h>

//private definitions
static sds append_search_expression_album(enum mpd_tag_type tag_albumartist, struct mpd_song *album, sds expression);
static sds search_songs(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id, long request_id,
        const char *expression, const char *sort, bool sortdesc, const char *plist, unsigned to, unsigned whence,
        unsigned offset, unsigned limit, const struct t_tags *tagcols, struct t_cache *sticker_cache,
        enum response_types response_type, bool *result);

//public functions

/**
 * Searches the mpd database for songs by expression and returns an jsonrpc result
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string to append the result
 * @param request_id jsonrpc request id
 * @param expression mpd search expression
 * @param sort tag to sort
 * @param sortdesc false = ascending, true = descending
 * @param offset result offset
 * @param limit max number of results to return
 * @param tagcols tags to return
 * @param sticker_cache pointer to sticker cache
 * @param result pointer to bool to set returncode
 * @return pointer to buffer
 */
sds mpd_client_search_response(struct t_partition_state *partition_state, sds buffer, long request_id,
        const char *expression, const char *sort, bool sortdesc, unsigned offset, unsigned limit,
        const struct t_tags *tagcols, struct t_cache *sticker_cache, bool *result)
{
    return search_songs(partition_state, buffer, MYMPD_API_DATABASE_SEARCH, request_id,
            expression, sort, sortdesc, NULL, 0, MPD_POSITION_ABSOLUTE, offset, limit,
            tagcols, sticker_cache, RESPONSE_TYPE_JSONRPC_RESPONSE, result);
}

/**
 * Searches the mpd database for songs by expression and adds the result to a playlist
 * @param partition_state pointer to partition specific states
 * @param expression mpd search expression
 * @param plist playlist to create or to append the result
 * @param to position to insert the songs, UINT_MAX to append
 * @param error pointer to already allocated sds string for the error message
 *              or NULL to return no response
 * @return true on success else false
 */
bool mpd_client_search_add_to_plist(struct t_partition_state *partition_state, const char *expression,
        const char *plist, unsigned to, sds *error)
{
    bool result = false;
    sds buffer = sdsempty();
    buffer = search_songs(partition_state, buffer, MYMPD_API_DATABASE_SEARCH, 0,
            expression, NULL, false, plist, to, MPD_POSITION_ABSOLUTE, 0, 0,
            NULL, NULL, RESPONSE_TYPE_PLAIN, &result);
    if (error != NULL &&
        *error != NULL)
    {
        *error = sdscatsds(*error, buffer);
    }
    FREE_SDS(buffer);
    return result;
}

/**
 * Searches the mpd database for songs by expression and adds the result to the queue
 * @param partition_state pointer to partition specific states
 * @param expression mpd search expression
 * @param to position to insert the songs, UINT_MAX to append
 * @param whence enum mpd_position_whence:
 *               0 = MPD_POSITION_ABSOLUTE
 *               1 = MPD_POSITION_AFTER_CURRENT
 *               2 = MPD_POSITION_BEFORE_CURRENT
 * @param error pointer to already allocated sds string for the error message
 *              or NULL to return no response
 * @return true on success else false
 */
bool mpd_client_search_add_to_queue(struct t_partition_state *partition_state, const char *expression,
        unsigned to, enum mpd_position_whence whence, sds *error)
{
    bool result = false;
    sds buffer = sdsempty();
    buffer = search_songs(partition_state, buffer, MYMPD_API_DATABASE_SEARCH, 0,
            expression, NULL, false, "queue", to, whence, 0, 0,
            NULL, NULL, RESPONSE_TYPE_PLAIN, &result);
    if (error != NULL &&
        *error != NULL)
    {
        *error = sdscatsds(*error, buffer);
    }
    FREE_SDS(buffer);
    return result;
}

/**
 * Creates a mpd search expression to find all songs in an album
 * @param tag_albumartist albumartist tag
 * @param album mpd_song struct representing the album
 * @return newly allocated sds string
 */
sds get_search_expression_album(enum mpd_tag_type tag_albumartist, struct mpd_song *album) {
    sds expression = sdsnewlen("(", 1);
    expression = append_search_expression_album(tag_albumartist, album, expression);
    expression = sdscatlen(expression, ")", 1);
    return expression;
}

/**
 * Creates a mpd search expression to find all songs in one cd of an album
 * @param tag_albumartist albumartist tag
 * @param album mpd_song struct representing the album
 * @param disc disc number
 * @return newly allocated sds string
 */
sds get_search_expression_album_disc(enum mpd_tag_type tag_albumartist, struct mpd_song *album, const char *disc) {
    sds expression = sdsnewlen("(", 1);
    expression = append_search_expression_album(tag_albumartist, album, expression);
    //and for cd
    expression = sdscat(expression, " AND ");
    expression = escape_mpd_search_expression(expression, "Disc", "==", disc);
    expression = sdscatlen(expression, ")", 1);
    return expression;
}

/**
 * Escapes a mpd search expression
 * @param buffer already allocated sds string to append
 * @param tag search tag
 * @param operator search operator
 * @param value search value
 * @return pointer to buffer
 */
sds escape_mpd_search_expression(sds buffer, const char *tag, const char *operator, const char *value) {
    buffer = sdscatfmt(buffer, "(%s %s '", tag, operator);
    for (size_t i = 0;  i < strlen(value); i++) {
        if (value[i] == '\\' || value[i] == '\'') {
            buffer = sdscatlen(buffer, "\\", 1);
        }
        buffer = sds_catchar(buffer, value[i]);
    }
    buffer = sdscatlen(buffer, "')", 2);
    return buffer;
}

//private functions

/**
 * Creates a mpd search expression to find all songs in one album
 * @param tag_albumartist albumartist tag
 * @param album mpd_song struct representing the album
 * @param expression already allocated sds string to append the expression
 * @return pointer to expression
 */
static sds append_search_expression_album(enum mpd_tag_type tag_albumartist, struct mpd_song *album, sds expression) {
    unsigned count = 0;
    const char *value;
    //search for all artists
    while ((value = mpd_song_get_tag(album, tag_albumartist, count)) != NULL) {
        expression = escape_mpd_search_expression(expression, mpd_tag_name(tag_albumartist), "==", value);
        expression = sdscat(expression, " AND ");
        count++;
    }
    //and for album
    expression = escape_mpd_search_expression(expression, "Album", "==", mpd_song_get_tag(album, MPD_TAG_ALBUM, 0));
    return expression;
}

/**
 * Searches the mpd database for songs by expression and returns a jsonrpc result
 * @param partition_state pointer to partition specific states
 * @param buffer already allocated sds string to append the result
 * @param cmd_id jsonrpc method
 * @param request_id jsonrpc request id
 * @param expression mpd search expression
 * @param sort tag to sort
 * @param sortdesc false = ascending, true = descending
 * @param plist playlist to create or to append the result
 *              NULL = print the result
 *              queue = add to queue
 *              other string = playlist name
 * @param to position to insert the songs, UINT_MAX to append
 * @param whence enum mpd_position_whence:
 *               0 = MPD_POSITION_ABSOLUTE
 *               1 = MPD_POSITION_AFTER_CURRENT
 *               2 = MPD_POSITION_BEFORE_CURRENT
 * @param offset result offset
 * @param limit max number of results to return
 * @param tagcols tags to return
 * @param sticker_cache pointer to sticker cache
 * @param response_type response type
 * @param result pointer to bool for the returncode
 * @return pointer to buffer
 */
static sds search_songs(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id, long request_id,
        const char *expression, const char *sort, bool sortdesc, const char *plist, unsigned to, unsigned whence,
        unsigned offset, unsigned limit, const struct t_tags *tagcols, struct t_cache *sticker_cache,
        enum response_types response_type, bool *result)
{
    *result = false;
    if (expression[0] == '\0') {
        return response_type == RESPONSE_TYPE_PLAIN
            ? sdscat(buffer, "No search expression defined")
            : jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "No search expression defined");
    }

    if (plist == NULL) {
        //show search results
        if (mpd_search_db_songs(partition_state->conn, false) == false) {
            mpd_search_cancel(partition_state->conn);
            return  response_type == RESPONSE_TYPE_PLAIN
                ? sdscat(buffer, "Error creating MPD search command")
                : jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
        }
    }
    else if (strcmp(plist, "queue") == 0) {
        //add search to queue
        if (mpd_search_add_db_songs(partition_state->conn, false) == false) {
            mpd_search_cancel(partition_state->conn);
            return  response_type == RESPONSE_TYPE_PLAIN
                ? sdscat(buffer, "Error creating MPD search command")
                : jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
        }
    }
    else {
        //add search to playlist
        if (mpd_search_add_db_songs_to_playlist(partition_state->conn, plist) == false) {
            mpd_search_cancel(partition_state->conn);
            return  response_type == RESPONSE_TYPE_PLAIN
                ? sdscat(buffer, "Error creating MPD search command")
                : jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
        }
    }

    if (mpd_search_add_expression(partition_state->conn, expression) == false) {
        mpd_search_cancel(partition_state->conn);
        return  response_type == RESPONSE_TYPE_PLAIN
            ? sdscat(buffer, "Error creating MPD search command")
            : jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
    }

    if (plist == NULL ||
        mpd_connection_cmp_server_version(partition_state->conn, 0, 22, 0) >= 0)
    {
        //sorting is supported
        if (sort != NULL &&
            strcmp(sort, "") != 0 &&
            strcmp(sort, "-") != 0 &&
            partition_state->mpd_state->feat_tags == true)
        {
            enum mpd_tag_type sort_tag = mpd_tag_name_parse(sort);
            if (sort_tag != MPD_TAG_UNKNOWN) {
                sort_tag = get_sort_tag(sort_tag, &partition_state->mpd_state->tags_mpd);
                if (mpd_search_add_sort_tag(partition_state->conn, sort_tag, sortdesc) == false) {
                    mpd_search_cancel(partition_state->conn);
                    return  response_type == RESPONSE_TYPE_PLAIN
                        ? sdscat(buffer, "Error creating MPD search command")
                        : jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
                }
            }
            else if (strcmp(sort, "LastModified") == 0) {
                //swap order
                sortdesc = sortdesc == false ? true : false;
                if (mpd_search_add_sort_name(partition_state->conn, "Last-Modified", sortdesc) == false) {
                    mpd_search_cancel(partition_state->conn);
                    return  response_type == RESPONSE_TYPE_PLAIN
                        ? sdscat(buffer, "Error creating MPD search command")
                        : jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
                }
            }
            else {
                MYMPD_LOG_WARN(partition_state->name, "Unknown sort tag: %s", sort);
            }
        }

        unsigned real_limit = limit == 0 ? offset + MPD_PLAYLIST_LENGTH_MAX : offset + limit;
        if (mpd_search_add_window(partition_state->conn, offset, real_limit) == false) {
            mpd_search_cancel(partition_state->conn);
            return  response_type == RESPONSE_TYPE_PLAIN
                ? sdscat(buffer, "Error creating MPD search command")
                : jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
        }
    }

    if (partition_state->mpd_state->feat_whence == true &&
        plist != NULL &&
        to < UINT_MAX) //to = UINT_MAX is append
    {
        if (mpd_search_add_position(partition_state->conn, to, whence) == false) {
            mpd_search_cancel(partition_state->conn);
            return  response_type == RESPONSE_TYPE_PLAIN
                ? sdscat(buffer, "Error creating MPD search command")
                : jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
        }
    }

    if (mpd_search_commit(partition_state->conn) &&
        plist == NULL)
    {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        struct mpd_song *song;
        unsigned entities_returned = 0;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_char(buffer, "Type", "song", true);
            buffer = get_song_tags(buffer, partition_state->mpd_state->feat_tags, tagcols, song);
            if (sticker_cache != NULL) {
                buffer = sdscatlen(buffer, ",", 1);
                buffer = mympd_api_sticker_get_print(buffer, sticker_cache, mpd_song_get_uri(song));
            }
            buffer = sdscatlen(buffer, "}", 1);
            mpd_song_free(song);
        }
        buffer = sdscatlen(buffer, "],", 2);
        if (offset == 0 &&
            entities_returned < limit)
        {
            buffer = tojson_uint(buffer, "totalEntities", entities_returned, true);
        }
        else {
            buffer = tojson_long(buffer, "totalEntities", -1, true);
        }
        buffer = tojson_uint(buffer, "offset", offset, true);
        buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
        buffer = tojson_char(buffer, "expression", expression, true);
        buffer = tojson_char(buffer, "sort", sort, true);
        buffer = tojson_bool(buffer, "sortdesc", sortdesc, false);
        buffer = jsonrpc_end(buffer);
    }
    mpd_response_finish(partition_state->conn);
    if (response_type == RESPONSE_TYPE_PLAIN) {
        if (mympd_check_error_and_recover(partition_state, &buffer, "mpd_search_db_songs") == false) {
            return buffer;
        }
    }
    else {
        if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_search_db_songs") == false) {
            return buffer;
        }
    }
    *result = true;
    return buffer;
}
