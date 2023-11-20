/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/random_select.h"

#include "src/lib/convert.h"
#include "src/lib/log.h"
#include "src/lib/random.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/search_local.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mpd_client/tags.h"

#include <stdlib.h>
#include <string.h>

/*
 * Private definitions
 */

static bool check_min_duration(const struct mpd_song *song, unsigned min_duration);
static bool check_max_duration(const struct mpd_song *song, unsigned max_duration);
static bool check_expression(const struct mpd_song *song, struct t_tags *tags,
        struct t_list *include_expr_list, struct t_list *exclude_expr_list);
static bool check_not_hated(rax *stickers_like, const char *uri, bool ignore_hated);
static bool check_last_played_album(rax *stickers_last_played, const char *uri, time_t since, enum album_modes album_mode);
static bool check_last_played(rax *stickers_last_played, const char *uri, time_t since);
static long check_uniq_tag(const char *uri, const char *value, struct t_list *queue_list, struct t_list *add_list);
static bool add_uri_constraint_or_expression(const char *include_expression, struct t_partition_state *partition_state);

enum random_add_uniq_result {
    RANDOM_ADD_UNIQ_IN_QUEUE = -2,
    RANDOM_ADD_UNIQ_IS_UNIQ = -1
};

/*
 * Public functions
 */

/**
 * Adds albums to the add_list
 * @param partition_state pointer to myMPD partition state
 * @param stickerdb pointer to stickerdb state
 * @param album_cache pointer to album cache
 * @param add_albums number of albums expected in add_list
 * @param queue_list list of current songs in mpd queue and last played
 * @param add_list list to add the albums
 * @param constraints constraints for album selection
 * @return new length of add_list
 */
unsigned random_select_albums(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        struct t_cache *album_cache, unsigned add_albums, struct t_list *queue_list, struct t_list *add_list,
        struct t_random_add_constraints *constraints)
{
    if (album_cache->cache == NULL) {
        MYMPD_LOG_WARN(partition_state->name, "Album cache is null, can not add random albums");
        return add_list->length;
    }

    unsigned initial_length = add_list->length;
    unsigned add_list_expected_len = add_albums;
    if (add_list->length >= add_albums) {
        return add_list->length;
    }
    add_albums = add_albums - add_list->length;

    MYMPD_LOG_DEBUG(partition_state->name, "Add list current length: %u", initial_length);
    MYMPD_LOG_DEBUG(partition_state->name, "Add list expected length: %u", add_list_expected_len);

    unsigned skipno = 0;
    unsigned lineno = 1;
    time_t since = time(NULL);
    since = since - (time_t)(constraints->last_played * 3600);
    sds albumid = sdsempty();
    rax *stickers_last_played = NULL;
    if (partition_state->config->albums.mode == ALBUM_MODE_ADV &&
        partition_state->mpd_state->feat.stickers == true)
    {
        stickers_last_played = stickerdb_find_stickers_by_name(stickerdb, "lastPlayed");
    }

    //parse mpd search expression
    struct t_list *include_expr_list = constraints->filter_include != NULL
        ? parse_search_expression_to_list(constraints->filter_include)
        : NULL;
    struct t_list *exclude_expr_list = constraints->filter_exclude != NULL
        ? parse_search_expression_to_list(constraints->filter_exclude)
        : NULL;

    sds tag_value = sdsempty();
    raxIterator iter;
    raxStart(&iter, album_cache->cache);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        struct mpd_song *album= (struct mpd_song *)iter.data;
        sdsclear(albumid);
        albumid = sdscatlen(albumid, (char *)iter.key, iter.key_len);
        sdsclear(tag_value);
        tag_value = mpd_client_get_tag_value_string(album, constraints->uniq_tag, tag_value);

        // we use the song uri in the album cache for enforcing last_played constraint,
        // because we do not know when an album was last played fully
        if (check_last_played_album(stickers_last_played, mpd_song_get_uri(album), since, partition_state->config->albums.mode) == true &&
            check_expression(album, &partition_state->mpd_state->tags_mpd, include_expr_list, exclude_expr_list) == true &&
            check_uniq_tag(albumid, tag_value, queue_list, add_list) == RANDOM_ADD_UNIQ_IS_UNIQ)
        {
            if (randrange(0, lineno) < add_albums) {
                if (add_list->length < add_list_expected_len) {
                    // append to fill the queue
                    if (list_push(add_list, albumid, lineno, tag_value, album) == false) {
                        MYMPD_LOG_ERROR(partition_state->name, "Can't push element to list");
                    }
                }
                else {
                    // replace at initial_length + random position
                    // existing entries should not be touched
                    unsigned pos = add_albums > 1
                        ? initial_length + randrange(0, add_albums)
                        : 0;
                    if (list_replace(add_list, pos, albumid, lineno, tag_value, album) == false) {
                        MYMPD_LOG_ERROR(partition_state->name, "Can't replace list element pos %u", pos);
                    }
                }
            }
            lineno++;
        }
        else {
            skipno++;
        }
    }
    FREE_SDS(albumid);
    FREE_SDS(tag_value);
    raxStop(&iter);
    free_search_expression_list(include_expr_list);
    free_search_expression_list(exclude_expr_list);
    if (stickers_last_played != NULL) {
        stickerdb_free_find_result(stickers_last_played);
    }
    MYMPD_LOG_DEBUG(partition_state->name, "Iterated through %u albums, skipped %u", lineno, skipno);
    return add_list->length;
}

/**
 * Adds songs to the add_list
 * @param partition_state pointer to myMPD partition state
 * @param stickerdb pointer to stickerdb state
 * @param add_songs number of songs expected in add_list
 * @param playlist playlist from which songs are added
 * @param queue_list list of current songs in mpd queue and last played
 * @param add_list list to add the songs
 * @param constraints constraints for song selection
 * @return new length of add_list
 */
unsigned random_select_songs(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        unsigned add_songs, const char *playlist, struct t_list *queue_list, struct t_list *add_list,
        struct t_random_add_constraints *constraints)
{
    unsigned initial_length = add_list->length;
    unsigned add_list_expected_len = add_songs;
    if (add_list->length >= add_songs) {
        return add_list->length;
    }
    add_songs = add_list_expected_len - add_list->length;

    MYMPD_LOG_DEBUG(partition_state->name, "Add list current length: %u", initial_length);
    MYMPD_LOG_DEBUG(partition_state->name, "Add list expected length: %u", add_list_expected_len);

    unsigned start = 0;
    unsigned end = start + MPD_RESULTS_MAX;
    unsigned skipno = 0;
    unsigned lineno = 1;
    time_t since = time(NULL) - (time_t)(constraints->last_played * 3600);

    bool from_database = strcmp(playlist, "Database") == 0
        ? true
        : false;
    sds tag_value = sdsempty();
    rax *stickers_last_played = NULL;
    rax *stickers_like = NULL;
    if (partition_state->mpd_state->feat.stickers == true) {
        MYMPD_LOG_DEBUG(partition_state->name, "Fetching lastPlayed stickers");
        stickers_last_played = stickerdb_find_stickers_by_name(stickerdb, "lastPlayed");
        if (constraints->ignore_hated == true) {
            MYMPD_LOG_DEBUG(partition_state->name, "Fetching stickers for hated songs");
            stickers_like = stickerdb_find_stickers_by_name_value(stickerdb, "like", MPD_STICKER_OP_EQ, "0");
        }
    }

    //parse mpd search expression
    struct t_list *include_expr_list = constraints->filter_include != NULL && strlen(constraints->filter_include) > 0
        ? parse_search_expression_to_list(constraints->filter_include)
        : NULL;
    struct t_list *exclude_expr_list = constraints->filter_exclude != NULL && strlen(constraints->filter_exclude) > 0
        ? parse_search_expression_to_list(constraints->filter_exclude)
        : NULL;

    if (include_expr_list == NULL) {
        MYMPD_LOG_DEBUG(NULL, "Include expression is empty");
    }
    if (exclude_expr_list == NULL) {
        MYMPD_LOG_DEBUG(NULL, "Exclude expression is empty");
    }

    do {
        MYMPD_LOG_DEBUG(partition_state->name, "Iterating through source, start: %u", start);
        if (from_database == true) {
            if (mpd_search_db_songs(partition_state->conn, false) == false ||
                add_uri_constraint_or_expression(constraints->filter_include, partition_state) == false ||
                mpd_search_add_window(partition_state->conn, start, end) == false)
            {
                MYMPD_LOG_ERROR(partition_state->name, "Error creating MPD search command");
                mpd_search_cancel(partition_state->conn);
            }
            else {
                mpd_search_commit(partition_state->conn);
            }
        }
        else {
            if (mpd_send_list_playlist_meta(partition_state->conn, playlist) == false) {
                MYMPD_LOG_ERROR(partition_state->name, "Error in response to command: mpd_send_list_playlist_meta");
            }
        }
        struct mpd_song *song;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            sdsclear(tag_value);
            tag_value = mpd_client_get_tag_value_string(song, constraints->uniq_tag, tag_value);
            const char *uri = mpd_song_get_uri(song);

            if (check_min_duration(song, constraints->min_song_duration) == true &&
                check_max_duration(song, constraints->max_song_duration) == true &&
                check_last_played(stickers_last_played, uri, since) == true &&
                check_not_hated(stickers_like, uri, constraints->ignore_hated) == true &&
                check_expression(song, &partition_state->mpd_state->tags_mpd, include_expr_list, exclude_expr_list) == true &&
                check_uniq_tag(uri, tag_value, queue_list, add_list) == RANDOM_ADD_UNIQ_IS_UNIQ)
            {
                if (randrange(0, lineno) < add_songs) {
                    if (add_list->length < add_list_expected_len) {
                        // append to fill the queue
                        if (list_push(add_list, uri, lineno, tag_value, NULL) == false) {
                            MYMPD_LOG_ERROR(partition_state->name, "Can't push element to list");
                        }
                    }
                    else {
                        // replace at initial_length + random position
                        // existing entries should not be touched
                        unsigned pos = add_songs > 1
                            ? initial_length + randrange(0, add_songs)
                            : 0;
                        if (list_replace(add_list, pos, uri, lineno, tag_value, NULL) == false) {
                            MYMPD_LOG_ERROR(partition_state->name, "Can't replace list element pos %u", pos);
                        }
                    }
                }
                lineno++;
            }
            else {
                skipno++;
            }
            mpd_song_free(song);
        }
        mpd_response_finish(partition_state->conn);
        if (mympd_check_error_and_recover(partition_state, NULL, "mpd_search_db_songs") == false) {
            break;
        }
        start = end;
        end = end + MPD_RESULTS_MAX;
    } while (from_database == true && lineno + skipno > start);
    stickerdb_free_find_result(stickers_last_played);
    stickerdb_free_find_result(stickers_like);
    free_search_expression_list(include_expr_list);
    free_search_expression_list(exclude_expr_list);
    FREE_SDS(tag_value);
    MYMPD_LOG_DEBUG(partition_state->name, "Iterated through %u songs, skipped %u", lineno, skipno);
    return add_list->length;
}

/**
 * Private functions
 */

/**
 * Checks for minimum duration constraint for songs
 * @param song mpd song struct to check
 * @param min_duration the minimum duration, 0 for no limit
 * @return if song is longer then min_duration true, else false
 */
static bool check_min_duration(const struct mpd_song *song, unsigned min_duration) {
    return min_duration > 0
        ? mpd_song_get_duration(song) > min_duration
        : true;
}

/**
 * Checks for maximum duration constraint for songs
 * @param song mpd song struct to check
 * @param max_duration the maximum duration, 0 for no limit
 * @return if song is shorter then min_duration true, else false
 */
static bool check_max_duration(const struct mpd_song *song, unsigned max_duration) {
    return max_duration > 0
        ? mpd_song_get_duration(song) < max_duration
        : true;
}

/**
 * Checks for the uniq tag constraint for songs
 * @param uri song uri or albumid
 * @param value tag value to check
 * @param queue_list list of current songs in mpd queue and last played
 * @param add_list list to add the entries
 * @return value matches in the mpd queue: RANDOM_ADD_UNIQ_IN_QUEUE
 *         value matches in the add_list: position in the add_list
 *         value does not match, is uniq: RANDOM_ADD_UNIQ_IS_UNIQ
 */
static long check_uniq_tag(const char *uri, const char *value, struct t_list *queue_list, struct t_list *add_list)
{
    if (queue_list == NULL) {
        return RANDOM_ADD_UNIQ_IS_UNIQ;
    }
    // check mpd queue and last_played
    struct t_list_node *current = queue_list->head;
    while(current != NULL) {
        if (strcmp(current->key, uri) == 0) {
            return RANDOM_ADD_UNIQ_IN_QUEUE;
        }
        if (value != NULL &&
            strcmp(current->value_p, value) == 0)
        {
            return RANDOM_ADD_UNIQ_IN_QUEUE;
        }
        current = current->next;
    }

    // check add_list
    current = add_list->head;
    long i = 0;
    while (current != NULL) {
        if (strcmp(current->key, uri) == 0) {
            return i;
        }
        if (value != NULL &&
            strcmp(current->value_p, value) == 0)
        {
            return i;
        }
        current = current->next;
        i++;
    }
    return RANDOM_ADD_UNIQ_IS_UNIQ;
}

/**
 * Checks if the song matches the expression lists
 * @param song song to apply the expressions
 * @param tags tags to search
 * @param include_expr_list include expression list
 * @param exclude_expr_list exclude expression list
 * @return true if song should be included, else false
 */
static bool check_expression(const struct mpd_song *song, struct t_tags *tags,
        struct t_list *include_expr_list, struct t_list *exclude_expr_list)
{
    // first check exclude expression
    if (exclude_expr_list != NULL &&
        search_song_expression(song, exclude_expr_list, tags) == true)
    {
        // exclude expression matches
        return false;
    }
    // exclude expression not matched, try include expression
    if (include_expr_list != NULL) {
        // exclude overwrites include
        return search_song_expression(song, include_expr_list, tags);
    }
    // no include expression, include all
    return true;
}

/**
 * Checks if the song is not hated and ignore_hated is set to true
 * @param stickers_like like stickers
 * @param uri uri to check against the stickers_like
 * @param ignore_hated ignore hated value
 * @return true if song is not hated, else false
 */
static bool check_not_hated(rax *stickers_like, const char *uri, bool ignore_hated) {
    if (stickers_like == NULL ||
        ignore_hated == false)
    {
        return true;
    }
    void *sticker_value_hated = raxFind(stickers_like, (unsigned char *)uri, strlen(uri));
    return sticker_value_hated == raxNotFound
        ? true
        : ((sds)sticker_value_hated)[0] == '0'
            ? false
            : true;
}

/**
 * Checks if the songs last_played time is older then since
 * @param stickers_last_played last_played stickers
 * @param uri uri to check against the stickers_last_played
 * @param since timestamp to check against
 * @return true if songs last_played time is older then since, else false
 */
static bool check_last_played(rax *stickers_last_played, const char *uri, time_t since) {
    if (stickers_last_played == NULL) {
        return true;
    }
    void *sticker_value_last_played = raxFind(stickers_last_played, (unsigned char *)uri, strlen(uri));
    if (sticker_value_last_played == raxNotFound) {
        return true;
    }
    int64_t sticker_last_played;
    enum str2int_errno rc = str2int64(&sticker_last_played, (sds)sticker_value_last_played);
    if (rc != STR2INT_SUCCESS) {
        return true;
    }
    return sticker_last_played < since;
}

/**
 * Checks if the album last_played time is older then since
 * @param stickers_last_played last_played stickers
 * @param uri uri to check against the stickers_last_played
 * @param since timestamp to check against
 * @param album_mode myMPD album mode
 * @return true if songs last_played time is older then since, else false
 */
static bool check_last_played_album(rax *stickers_last_played, const char *uri, time_t since, enum album_modes album_mode) {
    if (album_mode == ALBUM_MODE_SIMPLE) {
        // this only supported in advanced album mode
        return true;
    }
    return check_last_played(stickers_last_played, uri, since);
}

/**
 * Adds an expression if not empty, else adds an empty uri constraint to match all songs
 * @param expression include expression
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
static bool add_uri_constraint_or_expression(const char *expression, struct t_partition_state *partition_state) {
    if (expression == NULL ||
        strlen(expression) == 0)
    {
        return mpd_search_add_uri_constraint(partition_state->conn, MPD_OPERATOR_DEFAULT, "");
    }
    return mpd_search_add_expression(partition_state->conn, expression);
}
