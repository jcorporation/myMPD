/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Helper functions for MPD search
 */

#include "compile_time.h"
#include "src/mpd_client/search.h"

#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/tags.h"

#include <string.h>

//private definitions
static sds append_search_expression_album(enum mpd_tag_type tag_albumartist, struct mpd_song *album,
        const struct t_albums_config *album_config, sds expression);
static bool add_search_whence_param(struct t_partition_state *partition_state, unsigned to, unsigned whence);
static bool add_search_window_param(struct t_partition_state *partition_state, unsigned start, unsigned end);

//public functions

/**
 * Searches the mpd database for songs by expression and adds the result to a playlist
 * @param partition_state pointer to partition specific states
 * @param expression mpd search expression
 * @param plist playlist to create or to append the result
 * @param to position to insert the songs, UINT_MAX to append
 * @param sort tag to sort
 * @param sortdesc false = ascending, true = descending
 * @param start window start (including)
 * @param end window end (excluding), UINT_MAX for open end
 * @param error pointer to already allocated sds string for the error message
 *              or NULL to return no response
 * @return true on success else false
 */
bool mpd_client_search_add_to_plist_window(struct t_partition_state *partition_state, const char *expression,
        const char *plist, unsigned to, const char *sort, bool sortdesc, unsigned start, unsigned end, sds *error)
{
    if (mpd_search_add_db_songs_to_playlist(partition_state->conn, plist) == false ||
        mpd_search_add_expression(partition_state->conn, expression) == false ||
        mpd_client_add_search_sort_param(partition_state, sort, sortdesc, true) == false ||
        add_search_window_param(partition_state, start, end) == false ||
        add_search_whence_param(partition_state, to, MPD_POSITION_ABSOLUTE) == false)
    {
        mpd_search_cancel(partition_state->conn);
        *error = sdscat(*error, "Error creating MPD search command");
        return false;
    }
    mpd_search_commit(partition_state->conn);
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, error, "mpd_search_add_db_songs_to_playlist");
}

/**
 * Searches the mpd database for songs by expression and adds the result to a playlist
 * @param partition_state pointer to partition specific states
 * @param expression mpd search expression
 * @param plist playlist to create or to append the result
 * @param to position to insert the songs, UINT_MAX to append
 * @param sort tag to sort
 * @param sortdesc false = ascending, true = descending
 * @param error pointer to already allocated sds string for the error message
 *              or NULL to return no response
 * @return true on success else false
 */
bool mpd_client_search_add_to_plist(struct t_partition_state *partition_state, const char *expression,
        const char *plist, unsigned to, const char *sort, bool sortdesc, sds *error)
{
    return mpd_client_search_add_to_plist_window(partition_state, expression, plist, to, sort, sortdesc, 0, UINT_MAX, error);
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
 * @param sort tag to sort
 * @param sortdesc false = ascending, true = descending
 * @param error pointer to already allocated sds string for the error message
 *              or NULL to return no response
 * @return true on success else false
 */
bool mpd_client_search_add_to_queue(struct t_partition_state *partition_state, const char *expression,
        unsigned to, enum mpd_position_whence whence, const char *sort, bool sortdesc, sds *error)
{
    if (mpd_search_add_db_songs(partition_state->conn, false) == false ||
        mpd_search_add_expression(partition_state->conn, expression) == false ||
        mpd_client_add_search_sort_param(partition_state, sort, sortdesc, true) == false ||
        add_search_whence_param(partition_state, to, whence) == false)
    {
        mpd_search_cancel(partition_state->conn);
        *error = sdscat(*error, "Error creating MPD search command");
        return false;
    }
    mpd_search_commit(partition_state->conn);
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, error, "mpd_search_add_db_songs");
}

/**
 * Searches the mpd database for songs by expression and adds the result window to the queue
 * @param partition_state pointer to partition specific states
 * @param expression mpd search expression
 * @param to position to insert the songs, UINT_MAX to append
 * @param whence enum mpd_position_whence:
 *               0 = MPD_POSITION_ABSOLUTE
 *               1 = MPD_POSITION_AFTER_CURRENT
 *               2 = MPD_POSITION_BEFORE_CURRENT
 * @param sort tag to sort
 * @param sortdesc false = ascending, true = descending
 * @param start Start of the range (including)
 * @param end End of the range (excluding), use UINT_MAX for open end
 * @param error pointer to already allocated sds string for the error message
 *              or NULL to return no response
 * @return true on success else false
 */
bool mpd_client_search_add_to_queue_window(struct t_partition_state *partition_state, const char *expression,
        unsigned to, enum mpd_position_whence whence, const char *sort, bool sortdesc,
        unsigned start, unsigned end, sds *error)
{
    if (mpd_search_add_db_songs(partition_state->conn, false) == false ||
        mpd_search_add_expression(partition_state->conn, expression) == false ||
        mpd_client_add_search_sort_param(partition_state, sort, sortdesc, true) == false ||
        mpd_search_add_window(partition_state->conn, start, end) == false ||
        add_search_whence_param(partition_state, to, whence) == false)
    {
        mpd_search_cancel(partition_state->conn);
        *error = sdscat(*error, "Error creating MPD search command");
        return false;
    }
    mpd_search_commit(partition_state->conn);
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, error, "mpd_search_add_db_songs");
}

/**
 * Creates a mpd search expression to find all songs in an album
 * @param buffer Buffer to set the search expression
 * @param tag_albumartist albumartist tag
 * @param album mpd_song struct representing the album
 * @param album_config album configuration
 * @return Pointer to buffer
 */
sds get_search_expression_album(sds buffer, enum mpd_tag_type tag_albumartist, struct mpd_song *album,
        const struct t_albums_config *album_config)
{
    sdsclear(buffer);
    buffer = sdscatlen(buffer, "(", 1);
    buffer = append_search_expression_album(tag_albumartist, album, album_config, buffer);
    buffer = sdscatlen(buffer, ")", 1);
    return buffer;
}

/**
 * Creates a mpd search expression to find all songs of specified disc of an album
 * @param buffer Buffer to set the search expression
 * @param tag_albumartist albumartist tag
 * @param album mpd_song struct representing the album
 * @param tag MPD tag
 * @param tag_value MPD tag value
 * @param album_config album configuration
 * @return Pointer to buffer
 */
sds get_search_expression_album_tag(sds buffer, enum mpd_tag_type tag_albumartist, struct mpd_song *album,
        enum mpd_tag_type tag, const char *tag_value, const struct t_albums_config *album_config)
{
    sdsclear(buffer);
    buffer = sdscatlen(buffer, "(", 1);
    buffer = append_search_expression_album(tag_albumartist, album, album_config, buffer);
    //and for cd
    buffer = sdscat(buffer, " AND ");
    buffer = escape_mpd_search_expression(buffer, mpd_tag_name(tag), "==", tag_value);
    buffer = sdscatlen(buffer, ")", 1);
    return buffer;
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

/**
 * Adds the sort parameter to the search command
 * @param partition_state pointer to partition state
 * @param sort tag to sort
 * @param sortdesc descending sort in reverse order?
 * @param check_version checks if sort is supported (checks for mpd version >= 0.22.0)
 * @return true on success, else false
 */
bool mpd_client_add_search_sort_param(struct t_partition_state *partition_state, const char *sort, bool sortdesc, bool check_version) {
    if (check_version == true &&
        partition_state->mpd_state->feat.search_add_sort_window == false)
    {
        //silently ignore sort, MPD is too old
        return true;
    }
    if (sort != NULL &&
        sort[0] != '\0' &&
        partition_state->mpd_state->feat.tags == true)
    {
        enum mpd_tag_type sort_tag = mpd_tag_name_parse(sort);
        if (sort_tag != MPD_TAG_UNKNOWN) {
            sort_tag = get_sort_tag(sort_tag, &partition_state->mpd_state->tags_mpd);
            return mpd_search_add_sort_tag(partition_state->conn, sort_tag, sortdesc);
        }
        if (strcmp(sort, "Last-Modified") == 0 ||
            strcmp(sort, "Added") == 0)
        {
            //swap order
            sortdesc = sortdesc == false ? true : false;
            return mpd_search_add_sort_name(partition_state->conn, sort, sortdesc);
        }
        MYMPD_LOG_WARN(partition_state->name, "Unknown sort tag: %s", sort);
        return false;
    }
    return true;
}

/**
 * Adds the group parameter to the search command.
 * Ignores MPD_TAG_UNKNOWN.
 * @param conn mpd connection
 * @param tag group tag to add
 * @return true on success or MPD_TAG_UNKNOWN, else false
 */
bool mpd_client_add_search_group_param(struct mpd_connection *conn, enum mpd_tag_type tag) {
    return tag != MPD_TAG_UNKNOWN
        ? mpd_search_add_group_tag(conn, tag)
        : true;
}

//private functions

/**
 * Creates a mpd search expression to find all songs in one album
 * @param tag_albumartist albumartist tag
 * @param album mpd_song struct representing the album
 * @param album_config album configuration
 * @param expression already allocated sds string to append the expression
 * @return pointer to expression
 */
static sds append_search_expression_album(enum mpd_tag_type tag_albumartist, struct mpd_song *album,
        const struct t_albums_config *album_config, sds expression)
{
    unsigned count = 0;
    const char *value;
    //search for all artists
    while ((value = mpd_song_get_tag(album, tag_albumartist, count)) != NULL) {
        expression = escape_mpd_search_expression(expression, mpd_tag_name(tag_albumartist), "==", value);
        expression = sdscat(expression, " AND ");
        count++;
    }
    //and for album
    value = mpd_song_get_tag(album, MPD_TAG_ALBUM, 0);
    if (value != NULL) {
        expression = escape_mpd_search_expression(expression, "Album", "==", value);
    }
    else {
        expression = escape_mpd_search_expression(expression, "Album", "==", "");
    }
    //optionally append group tag
    if (album_config->group_tag != MPD_TAG_UNKNOWN) {
        value = mpd_song_get_tag(album, album_config->group_tag, 0);
        if (value != NULL) {
            expression = sdscat(expression, " AND ");
            expression = escape_mpd_search_expression(expression, mpd_tag_name(album_config->group_tag),
                "==", value);
        }
    }
    return expression;
}

/**
 * Adds the position parameter to the search command
 * @param partition_state pointer to partition state
 * @param to position to insert the songs, UINT_MAX to append
 * @param whence enum mpd_position_whence:
 *               0 = MPD_POSITION_ABSOLUTE
 *               1 = MPD_POSITION_AFTER_CURRENT
 *               2 = MPD_POSITION_BEFORE_CURRENT
 * @return true on success, else false
 */
static bool add_search_whence_param(struct t_partition_state *partition_state, unsigned to, unsigned whence) {
    if (partition_state->mpd_state->feat.whence == true &&
        to < UINT_MAX) //to = UINT_MAX is append
    {
        return mpd_search_add_position(partition_state->conn, to, whence);
    }
    return true;
}

/**
 * Adds the window parameter to the search command
 * @param partition_state pointer to partition state
 * @param start start of the window (including)
 * @param end end of the window (excluding)
 * @return true on success, else false
 */
static bool add_search_window_param(struct t_partition_state *partition_state, unsigned start, unsigned end) {
    if (partition_state->mpd_state->feat.search_add_sort_window == true) {
        return mpd_search_add_window(partition_state->conn, start, end);
    }
    return true;
}
