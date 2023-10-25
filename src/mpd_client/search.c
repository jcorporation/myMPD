/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/search.h"

#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/tags.h"

#include <string.h>

//private definitions
static sds append_search_expression_album(enum mpd_tag_type tag_albumartist, struct mpd_song *album, sds expression);
static bool add_search_whence_param(struct t_partition_state *partition_state, unsigned to, unsigned whence);

//public functions

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
    if (mpd_search_add_db_songs_to_playlist(partition_state->conn, plist) == false ||
        mpd_search_add_expression(partition_state->conn, expression) == false ||
        mpd_client_add_search_sort_param(partition_state, sort, sortdesc, true) == false ||
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
        mpd_connection_cmp_server_version(partition_state->conn, 0, 22, 0) < 0)
    {
        //silently ignore sort, MPD is too old
        return true;
    }
    if (sort != NULL &&
        sort[0] != '\0' &&
        sort[0] != '-' &&
        partition_state->mpd_state->feat_tags == true)
    {
        enum mpd_tag_type sort_tag = mpd_tag_name_parse(sort);
        if (sort_tag != MPD_TAG_UNKNOWN) {
            sort_tag = get_sort_tag(sort_tag, &partition_state->mpd_state->tags_mpd);
            return mpd_search_add_sort_tag(partition_state->conn, sort_tag, sortdesc);
        }
        if (strcmp(sort, "Last-Modified") == 0) {
            //swap order
            sortdesc = sortdesc == false ? true : false;
            return mpd_search_add_sort_name(partition_state->conn, "Last-Modified", sortdesc);
        }
        MYMPD_LOG_WARN(partition_state->name, "Unknown sort tag: %s", sort);
        return false;
    }
    return true;
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
    if (partition_state->mpd_state->feat_whence == true &&
        to < UINT_MAX) //to = UINT_MAX is append
    {
        return mpd_search_add_position(partition_state->conn, to, whence);
    }
    return true;
}
