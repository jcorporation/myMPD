/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/jukebox.h"

#include "dist/rax/rax.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mympd_state.h"
#include "src/lib/random.h"
#include "src/lib/sds_extras.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/queue.h"
#include "src/mpd_client/search.h"
#include "src/mpd_client/search_local.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mpd_client/tags.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

//private definitions
static bool jukebox(struct t_partition_state *partition_state);
static void jukebox_get_last_played_add(struct t_partition_state *partition_state,
    struct mpd_song *song, struct t_list *queue_list, enum jukebox_modes jukebox_mode);
static struct t_list *jukebox_get_last_played(struct t_partition_state *partition_state,
        enum jukebox_modes jukebox_mode);
static bool jukebox_run_fill_jukebox_queue(struct t_partition_state *partition_state,
        long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static bool jukebox_fill_jukebox_queue(struct t_partition_state *partition_state,
        long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static bool add_album_to_queue(struct t_partition_state *partition_state, struct mpd_song *album);
static long fill_jukebox_queue_songs(struct t_partition_state *partition_state, long add_songs,
        const char *playlist, bool manual, struct t_list *queue_list, struct t_list *add_list);
static long fill_jukebox_queue_albums(struct t_partition_state *partition_state, long add_albums,
        bool manual, struct t_list *queue_list, struct t_list *add_list);
static bool check_expression(const struct mpd_song *song, struct t_tags *tags,
        struct t_list *include_expr_list, struct t_list *exclude_expr_list);
static bool check_not_hated(rax *stickers_like, const char *uri, bool jukebox_ignore_hated);
static bool check_last_played(rax *stickers_last_played, const char *uri, time_t since);
static long jukebox_unique_tag(struct t_partition_state *partition_state, const char *uri,
        const char *value, bool manual, struct t_list *queue_list);
static long jukebox_unique_album(struct t_partition_state *partition_state, const char *album,
        const char *albumartist, bool manual, struct t_list *queue_list);
static bool add_uri_constraint_or_expression(sds include_expression, struct t_partition_state *partition_state);

enum jukebox_uniq_result {
    JUKEBOX_UNIQ_IN_QUEUE = -2,
    JUKEBOX_UNIQ_IS_UNIQ = -1
};

/**
 * Public functions
 */

/**
 * Parses the string to the jukebox mode
 * @param str string to parse
 * @return jukebox mode
 */
enum jukebox_modes jukebox_mode_parse(const char *str) {
    if (strcmp(str, "off") == 0) {
        return JUKEBOX_OFF;
    }
    if (strcmp(str, "song") == 0) {
        return JUKEBOX_ADD_SONG;
    }
    if (strcmp(str, "album") == 0) {
        return JUKEBOX_ADD_ALBUM;
    }
    return JUKEBOX_UNKNOWN;
}

/**
 * Returns the jukebox mode as string
 * @param mode the jukebox mode
 * @return jukebox mode as string
 */
const char *jukebox_mode_lookup(enum jukebox_modes mode) {
	switch (mode) {
        case JUKEBOX_OFF:
            return "off";
        case JUKEBOX_ADD_SONG:
            return "song";
        case JUKEBOX_ADD_ALBUM:
            return "album";
        default:
            return NULL;
    }
    return NULL;
}

/**
 * Clears the jukebox queue of all partitions.
 * @param mympd_state pointer to central myMPD state.
 */
void jukebox_clear_all(struct t_mympd_state *mympd_state) {
    struct t_partition_state *partition_state = mympd_state->partition_state;
    while (partition_state != NULL) {
        list_clear(&partition_state->jukebox_queue);
        //notify clients
        send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_state->name);
        //next entry
        partition_state = partition_state->next;
    }
}

/**
 * Wrapper for the real jukebox function that retries adding songs or albums three times.
 * @param partition_state pointer to myMPD partition state
 * @return true on success, else false
 */
bool jukebox_run(struct t_partition_state *partition_state) {
    for (int i = 1; i < 3; i++) {
         if (jukebox(partition_state) == true) {
             return true;
         }
         if (partition_state->jukebox_mode == JUKEBOX_OFF) {
             return false;
         }
         MYMPD_LOG_ERROR(partition_state->name, "Jukebox: trying again, attempt %d", i);
    }
    return false;
}

/**
 * The real jukebox function.
 * It determines if a song must be added or not and starts playing.
 * @param partition_state pointer to myMPD partition state
 * @return true on success, else false
 */
static bool jukebox(struct t_partition_state *partition_state) {
    long queue_length = 0;
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status != NULL) {
        queue_length = (long)mpd_status_get_queue_length(status);
        mpd_status_free(status);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_status") == false) {
        return false;
    }

    time_t now = time(NULL);
    time_t add_time = partition_state->song_end_time - (partition_state->crossfade + 10);

    MYMPD_LOG_DEBUG(partition_state->name, "Queue length: %ld", queue_length);
    MYMPD_LOG_DEBUG(partition_state->name, "Min queue length: %ld", partition_state->jukebox_queue_length);

    if (queue_length >= partition_state->jukebox_queue_length && now < add_time) {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: Queue length >= %ld and add_time not reached", partition_state->jukebox_queue_length);
        return true;
    }

    //add song if add_time is reached or queue is empty
    long add_songs = partition_state->jukebox_queue_length > queue_length ? partition_state->jukebox_queue_length - queue_length : 0;

    if (now > add_time &&
        add_time > 0 &&
        queue_length <= partition_state->jukebox_queue_length)
    {
        if (add_songs == 0) {
            add_songs++;
        }
        MYMPD_LOG_DEBUG(partition_state->name, "Time now %lld greater than add_time %lld, adding %ld song(s)", (long long)now, (long long)add_time, add_songs);
    }

    if (add_songs < 1) {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: nothing to do");
        return true;
    }

    if (add_songs > 99) {
        MYMPD_LOG_WARN(partition_state->name, "Jukebox: max songs to add set to %ld, adding max. 99 songs", add_songs);
        add_songs = 99;
    }

    if (partition_state->mpd_state->feat_playlists == false && strcmp(partition_state->jukebox_playlist, "Database") != 0) {
        MYMPD_LOG_WARN(partition_state->name, "Jukebox: Playlists are disabled");
        return true;
    }

    bool rc = jukebox_add_to_queue(partition_state, add_songs, partition_state->jukebox_mode, partition_state->jukebox_playlist, false);

    //update playback state
    mpd_client_queue_status(partition_state, NULL);
    if (partition_state->play_state != MPD_STATE_PLAY) {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: start playback");
        mpd_run_play(partition_state->conn);
        mympd_check_error_and_recover(partition_state, NULL, "mpd_run_play");
    }

    if (rc == true) {
        //notify clients
        send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_state->name);
        return true;
    }

    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox mode: %d", partition_state->jukebox_mode);
    MYMPD_LOG_ERROR(partition_state->name, "Jukebox: Error adding song(s)");
    return false;
}

/**
 * This functions checks if the jukebox queue is long enough, refills the queue if necessary
 * and adds songs or albums to the queue.
 * @param partition_state pointer to myMPD partition state
 * @param add_songs number of songs to add
 * @param jukebox_mode jukebox mode
 * @param playlist playlist to add songs from
 * @param manual false = normal jukebox operation
 *               true = create separate jukebox queue and add songs to queue once
 * @return true on success, else false
 */
bool jukebox_add_to_queue(struct t_partition_state *partition_state, long add_songs,
        enum jukebox_modes jukebox_mode, const char *playlist, bool manual)
{
    if (manual == false) {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox queue length: %ld", partition_state->jukebox_queue.length);
    }
    if ((manual == false && add_songs > partition_state->jukebox_queue.length) ||
        (manual == true))
    {
        bool rc = jukebox_run_fill_jukebox_queue(partition_state, add_songs, jukebox_mode, playlist, manual);
        if (rc == false) {
            return false;
        }
    }
    long added = 0;
    struct t_list_node *current;
    if (manual == false) {
        current = partition_state->jukebox_queue.head;
    }
    else {
        current = partition_state->jukebox_queue_tmp.head;
    }
    while (current != NULL &&
        added < add_songs)
    {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
            mpd_run_add(partition_state->conn, current->key);
            if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_add") == true) {
                MYMPD_LOG_NOTICE(partition_state->name, "Jukebox adding song: %s", current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR(partition_state->name, "Jukebox adding song %s failed", current->key);
            }
        }
        else {
            bool rc = add_album_to_queue(partition_state, (struct mpd_song *)current->user_data);
            if (rc == true) {
                MYMPD_LOG_NOTICE(partition_state->name, "Jukebox adding album: %s - %s", current->value_p, current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR(partition_state->name, "Jukebox adding album %s - %s failed", current->value_p, current->key);
            }
        }
        if (manual == false) {
            if (list_remove_node(&partition_state->jukebox_queue, 0) == false) {
                MYMPD_LOG_ERROR(partition_state->name, "Error removing first entry from jukebox queue");
            }
            current = partition_state->jukebox_queue.head;
        }
        else {
            if (list_remove_node(&partition_state->jukebox_queue_tmp, 0) == false) {
                MYMPD_LOG_ERROR(partition_state->name, "Error removing first entry from manual jukebox queue");
            }
            current = partition_state->jukebox_queue_tmp.head;
        }
    }
    if (added == 0) {
        MYMPD_LOG_ERROR(partition_state->name, "Error adding song(s)");
        send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_ERROR, partition_state->name, "Adding songs from jukebox to queue failed");
        return false;
    }
    if (manual == false) {
        if ((jukebox_mode == JUKEBOX_ADD_SONG && partition_state->jukebox_queue.length < 25) ||
            (jukebox_mode == JUKEBOX_ADD_ALBUM && partition_state->jukebox_queue.length < 5))
        {
            bool rc = jukebox_run_fill_jukebox_queue(partition_state, add_songs, jukebox_mode, playlist, manual);
            if (rc == false) {
                return false;
            }
        }
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox queue length: %ld", partition_state->jukebox_queue.length);
    }
    return true;
}

/**
 * Private functions
 */

/**
 * Adds a complete album to the queue
 * @param partition_state pointer to myMPD partition state
 * @param album album to add
 * @return true on success, else false
 */
static bool add_album_to_queue(struct t_partition_state *partition_state, struct mpd_song *album) {
    sds expression = get_search_expression_album(partition_state->mpd_state->tag_albumartist, album);
    if (mpd_search_add_db_songs(partition_state->conn, true) &&
        mpd_search_add_expression(partition_state->conn, expression))
    {
        mpd_search_commit(partition_state->conn);
    }
    else {
        mpd_search_cancel(partition_state->conn);
        MYMPD_LOG_ERROR(partition_state->name, "Error creating MPD search command");
    }
    FREE_SDS(expression);
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, NULL, "mpd_search_commit");
}

/**
 * Ads an album or song to the jukebox last played list
 * @param partition_state pointer to myMPD partition state
 * @param song mpd_song struct to append
 * @param queue_list list to append the entry
 * @param jukebox_mode the jukebox mode
 */
static void jukebox_get_last_played_add(struct t_partition_state *partition_state,
    struct mpd_song *song, struct t_list *queue_list, enum jukebox_modes jukebox_mode)
{
    if (song == NULL) {
        return;
    }
    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        sds tag_value = sdsempty();
        if (partition_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
            tag_value = mpd_client_get_tag_value_string(song, partition_state->jukebox_unique_tag.tags[0], tag_value);
        }
        list_push(queue_list, mpd_song_get_uri(song), 0, tag_value, NULL);
        FREE_SDS(tag_value);
    }
    else {
        // JUKEBOX_ADD_ALBUM
        sds album = mpd_client_get_tag_value_string(song, MPD_TAG_ALBUM, sdsempty());
        sds albumartist = mpd_client_get_tag_value_string(song, partition_state->mpd_state->tag_albumartist, sdsempty());
        list_push(queue_list, album, 0, albumartist, NULL);
        FREE_SDS(album);
        FREE_SDS(albumartist);
    }
    mpd_song_free(song);
}

/**
 * Gets the song list from queue and last played.
 * This list is used to enforce the uniq tag constraint
 * @param partition_state pointer to myMPD partition state
 * @param jukebox_mode the jukebox mode
 * @return a newly allocated list with songs or albums
 */
static struct t_list *jukebox_get_last_played(struct t_partition_state *partition_state, enum jukebox_modes jukebox_mode) {
    struct mpd_song *song;
    struct t_list *queue_list = list_new();

    // get current queue
    if (mpd_send_list_queue_meta(partition_state->conn)) {
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            jukebox_get_last_played_add(partition_state, song, queue_list, jukebox_mode);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_queue_meta") == false) {
        FREE_PTR(queue_list);
        return NULL;
    }

    // append last_played from memory to queue list
    struct t_list_node *current = partition_state->last_played.head;
    while (current != NULL) {
        if (mpd_send_list_meta(partition_state->conn, current->key)) {
            song = mpd_recv_song(partition_state->conn);
            jukebox_get_last_played_add(partition_state, song, queue_list, jukebox_mode);
        }
        mpd_response_finish(partition_state->conn);
        mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_meta");
        current = current->next;
    }

    if (queue_list->length < JUKEBOX_UNIQ_RANGE) {
        // append last_played from disc to queue list
        sds lp_file = sdscatfmt(sdsempty(), "%S/%S/%s",
            partition_state->mympd_state->config->workdir, partition_state->state_dir, FILENAME_LAST_PLAYED);
        errno = 0;
        FILE *fp = fopen(lp_file, OPEN_FLAGS_READ);
        if (fp != NULL) {
            sds line = sdsempty();
            while (sds_getline(&line, fp, LINE_LENGTH_MAX) >= 0 &&
                    queue_list->length < JUKEBOX_UNIQ_RANGE)
            {
                sds uri = NULL;
                if (json_get_string_max(line, "$.uri", &uri, vcb_isfilepath, NULL) == true) {
                    if (mpd_send_list_meta(partition_state->conn, uri)) {
                        song = mpd_recv_song(partition_state->conn);
                        jukebox_get_last_played_add(partition_state, song, queue_list, jukebox_mode);
                    }
                    mpd_response_finish(partition_state->conn);
                    mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_meta");
                }
                else {
                    MYMPD_LOG_ERROR(partition_state->name, "Reading last_played line failed");
                    MYMPD_LOG_DEBUG(partition_state->name, "Erroneous line: %s", line);
                }
                FREE_SDS(uri);
            }
            (void) fclose(fp);
            FREE_SDS(line);
        }
        else {
            MYMPD_LOG_DEBUG(partition_state->name, "Can not open \"%s\"", lp_file);
            if (errno != ENOENT) {
                //ignore missing last_played file
                MYMPD_LOG_ERRNO(partition_state->name, errno);
            }
        }
        FREE_SDS(lp_file);
    }

    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox last_played list length: %ld", queue_list->length);
    return queue_list;
}

/**
 * Wrapper function for the real jukebox queue filling function.
 * @param partition_state pointer to myMPD partition state
 * @param add_songs number of songs or albums to add
 * @param jukebox_mode the jukebox mode
 * @param playlist playlist to add songs from
 * @param manual false = normal jukebox operation
 *               true = create separate jukebox queue and add songs to queue once
 * @return true on success, else false
 */
static bool jukebox_run_fill_jukebox_queue(struct t_partition_state *partition_state,
        long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual)
{
    send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_INFO, partition_state->name, "Filling jukebox queue");
    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox queue to small, adding entities");
    bool rc = jukebox_fill_jukebox_queue(partition_state, add_songs, jukebox_mode, playlist, manual);
    if (rc == false) {
        MYMPD_LOG_ERROR(partition_state->name, "Filling jukebox queue failed, disabling jukebox");
        send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_ERROR, partition_state->name, "Filling jukebox queue failed, disabling jukebox");
        partition_state->jukebox_mode = JUKEBOX_OFF;
        return false;
    }
    return true;
}

/**
 * The real jukebox queue filling function.
 * @param partition_state pointer to myMPD partition state
 * @param add_songs number of songs or albums to add
 * @param jukebox_mode the jukebox mode
 * @param playlist playlist to add songs from
 * @param manual false = normal jukebox operation
 *               true = create separate jukebox queue and add songs to queue once
 * @return true on success, else false
 */
static bool jukebox_fill_jukebox_queue(struct t_partition_state *partition_state,
        long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual)
{
    long added = 0;

    if (manual == true) {
        //do not use jukebox_clear wrapper to prevent obsolet notification
        list_clear(&partition_state->jukebox_queue_tmp);
    }

    //get last_played and current queue
    struct t_list *queue_list = jukebox_get_last_played(partition_state, jukebox_mode);
    if (queue_list == NULL) {
        return false;
    }

    struct t_list *add_list = manual == false ?
        &partition_state->jukebox_queue :
        &partition_state->jukebox_queue_tmp;

    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        added = fill_jukebox_queue_songs(partition_state, add_songs, playlist, manual, queue_list, add_list);
    }
    else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
        added = fill_jukebox_queue_albums(partition_state, add_songs, manual, queue_list, add_list);
    }

    if (added < add_songs) {
        MYMPD_LOG_WARN(partition_state->name, "Jukebox queue didn't contain %ld entries", add_songs);
        if (partition_state->jukebox_enforce_unique == true) {
            MYMPD_LOG_WARN(partition_state->name, "Disabling jukebox unique constraints temporarily");
            partition_state->jukebox_enforce_unique = false;
            send_jsonrpc_notify(JSONRPC_FACILITY_JUKEBOX, JSONRPC_SEVERITY_WARN, partition_state->name, "Playlist to small, disabling jukebox unique constraints temporarily");
        }
    }

    list_free(queue_list);
    return true;
}

/**
 * Adds albums to the jukebox queue
 * @param partition_state pointer to myMPD partition state
 * @param add_albums number of albums to add
 * @param manual false = normal jukebox operation
 *               true = create separate jukebox queue and add songs to queue once
 * @param queue_list list of current songs in mpd queue and last played
 * @param add_list jukebox queue to add the albums
 * @return true on success, else false
 */
static long fill_jukebox_queue_albums(struct t_partition_state *partition_state, long add_albums,
        bool manual, struct t_list *queue_list, struct t_list *add_list)
{
    if (partition_state->mpd_state->album_cache.cache == NULL) {
        MYMPD_LOG_WARN(partition_state->name, "Album cache is null, jukebox can not add albums");
        return -1;
    }

    long start_length = 0;
    if (manual == false) {
        start_length = partition_state->jukebox_queue.length;
        add_albums = 10 - partition_state->jukebox_queue.length;
        if (add_albums <= 0) {
            return 0;
        }
    }

    long skipno = 0;
    long lineno = 1;
    time_t since = time(NULL);
    since = since - (partition_state->jukebox_last_played * 3600);
    sds tag_album = sdsempty();
    sds tag_albumartist = sdsempty();
    rax *stickers_last_played = NULL;
    if (partition_state->mpd_state->feat_stickers == true) {
        stickers_last_played = stickerdb_find_stickers_by_name(partition_state->mympd_state->stickerdb, "lastPlayed");
    }

    //parse mpd search expression
    struct t_list *include_expr_list = sdslen(partition_state->jukebox_filter_include) > 0
        ? parse_search_expression_to_list(partition_state->jukebox_filter_include)
        : NULL;
    struct t_list *exclude_expr_list = sdslen(partition_state->jukebox_filter_exclude) > 0
        ? parse_search_expression_to_list(partition_state->jukebox_filter_exclude)
        : NULL;

    raxIterator iter;
    raxStart(&iter, partition_state->mpd_state->album_cache.cache);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        struct mpd_song *album= (struct mpd_song *)iter.data;
        sdsclear(tag_album);
        sdsclear(tag_albumartist);
        tag_album = mpd_client_get_tag_value_string(album, MPD_TAG_ALBUM, tag_album);
        tag_albumartist = mpd_client_get_tag_value_string(album, partition_state->mpd_state->tag_albumartist, tag_albumartist);

        // we use the song uri in the album cache for enforcing last_played constraint,
        // because we do not know when an album was last played fully
        const char *uri = mpd_song_get_uri(album);
        if (check_last_played(stickers_last_played, uri, since) == true &&
            check_expression(album, &partition_state->mpd_state->tags_mpd, include_expr_list, exclude_expr_list) == true &&
            jukebox_unique_album(partition_state, tag_album, tag_albumartist, manual, queue_list) == JUKEBOX_UNIQ_IS_UNIQ)
        {
            if (randrange(0, lineno) < add_albums) {
                if (add_list->length < add_albums) {
                    if (list_push(add_list, tag_album, lineno, tag_albumartist, album) == false) {
                        MYMPD_LOG_ERROR(partition_state->name, "Can't push jukebox_queue element");
                    }
                }
                else {
                    long pos = add_albums > 1 ? start_length + randrange(0, add_albums -1) : 0;
                    if (list_replace(add_list, pos, tag_album, lineno, tag_albumartist, album) == false) {
                        MYMPD_LOG_ERROR(partition_state->name, "Can't replace jukebox_queue element pos %ld", pos);
                    }
                }
            }
            lineno++;
        }
        else {
            skipno++;
        }
    }
    FREE_SDS(tag_album);
    FREE_SDS(tag_albumartist);
    raxStop(&iter);
    free_search_expression_list(include_expr_list);
    free_search_expression_list(exclude_expr_list);
    if (stickers_last_played != NULL) {
        stickerdb_free_find_result(stickers_last_played);
    }
    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox iterated through %ld albums, skipped %ld", lineno, skipno);
    return (int)add_list->length;
}

/**
 * Adds songs to the jukebox queue
 * @param partition_state pointer to myMPD partition state
 * @param add_songs number of songs to add
 * @param playlist playlist from which songs are added
 * @param manual false = normal jukebox operation
 *               true = create separate jukebox queue and add songs to queue once
 * @param queue_list list of current songs in mpd queue and last played
 * @param add_list jukebox queue to add the songs
 * @return true on success, else false
 */
static long fill_jukebox_queue_songs(struct t_partition_state *partition_state, long add_songs, const char *playlist,
        bool manual, struct t_list *queue_list, struct t_list *add_list)
{
    unsigned start = 0;
    unsigned end = start + MPD_RESULTS_MAX;
    long skipno = 0;
    long lineno = 1;
    time_t since = time(NULL) - (partition_state->jukebox_last_played * 3600);

    long start_length = 0;
    if (manual == false) {
        start_length = partition_state->jukebox_queue.length;
        add_songs = (long)50 - start_length;
        if (add_songs <= 0) {
            return 0;
        }
    }
    bool from_database = strcmp(playlist, "Database") == 0
        ? true
        : false;
    sds tag_value = sdsempty();
    rax *stickers_last_played = NULL;
    rax *stickers_like = NULL;
    if (partition_state->mpd_state->feat_stickers == true) {
        stickers_last_played = stickerdb_find_stickers_by_name(partition_state->mympd_state->stickerdb, "lastPlayed");
        if (partition_state->jukebox_ignore_hated == true) {
            stickers_like = stickerdb_find_stickers_by_name(partition_state->mympd_state->stickerdb, "like");
        }
    }
    //parse mpd search expression
    struct t_list *include_expr_list = sdslen(partition_state->jukebox_filter_include) > 0
        ? parse_search_expression_to_list(partition_state->jukebox_filter_include)
        : NULL;
    struct t_list *exclude_expr_list = sdslen(partition_state->jukebox_filter_exclude) > 0
        ? parse_search_expression_to_list(partition_state->jukebox_filter_exclude)
        : NULL;

    if (include_expr_list == NULL) {
        MYMPD_LOG_DEBUG(NULL, "Include expression is empty");
    }
    if (exclude_expr_list == NULL) {
        MYMPD_LOG_DEBUG(NULL, "Exclude expression is empty");
    }

    do {
        MYMPD_LOG_DEBUG(partition_state->name, "Jukebox: iterating through source, start: %u", start);
        if (from_database == true) {
            if (mpd_search_db_songs(partition_state->conn, false) == false ||
                add_uri_constraint_or_expression(partition_state->jukebox_filter_include, partition_state) == false ||
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
            tag_value = mpd_client_get_tag_value_string(song, partition_state->jukebox_unique_tag.tags[0], tag_value);
            const char *uri = mpd_song_get_uri(song);

            if (mpd_song_get_duration(song) > partition_state->jukebox_min_song_duration &&
                check_last_played(stickers_last_played, uri, since) == true &&
                check_not_hated(stickers_like, uri, partition_state->jukebox_ignore_hated) == true &&
                check_expression(song, &partition_state->mpd_state->tags_mpd, include_expr_list, exclude_expr_list) == true &&
                jukebox_unique_tag(partition_state, uri, tag_value, manual, queue_list) == JUKEBOX_UNIQ_IS_UNIQ)
            {
                if (randrange(0, lineno) < add_songs) {
                    if (add_list->length < add_songs) {
                        if (list_push(add_list, uri, lineno, tag_value, NULL) == false) {
                            MYMPD_LOG_ERROR(partition_state->name, "Can't push jukebox_queue element");
                        }
                    }
                    else {
                        long pos = add_songs > 1 ? start_length + randrange(0, add_songs - 1) : 0;
                        if (list_replace(add_list, pos, uri, lineno, tag_value, NULL) == false) {
                            MYMPD_LOG_ERROR(partition_state->name, "Can't replace jukebox_queue element pos %ld", pos);
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
            FREE_SDS(tag_value);
            return -1;
        }
        start = end;
        end = end + MPD_RESULTS_MAX;
    } while (from_database == true && lineno + skipno > (long)start);
    if (stickers_last_played != NULL) {
        stickerdb_free_find_result(stickers_last_played);
    }
    if (stickers_like != NULL) {
        stickerdb_free_find_result(stickers_like);
    }
    free_search_expression_list(include_expr_list);
    free_search_expression_list(exclude_expr_list);
    FREE_SDS(tag_value);
    MYMPD_LOG_DEBUG(partition_state->name, "Jukebox iterated through %ld songs, skipped %ld", lineno, skipno);
    return (int)add_list->length;
}

/**
 * Checks for the uniq tag constraint for songs
 * @param partition_state pointer to myMPD partition state
 * @param uri song uri
 * @param value tag value to check
 * @param manual false = normal jukebox operation
 *               true = create separate jukebox queue and add songs to queue once
 * @param queue_list list of current songs in mpd queue and last played
 * @return value matches in the mpd queue: JUKEBOX_UNIQ_IN_QUEUE
 *         value matches in the jukebox queue: position in the jukebox queue
 *         value does not match, is uniq: JUKEBOX_UNIQ_IS_UNIQ
 */
static long jukebox_unique_tag(struct t_partition_state *partition_state, const char *uri,
        const char *value, bool manual, struct t_list *queue_list)
{
    if (partition_state->jukebox_enforce_unique == false) {
        return JUKEBOX_UNIQ_IS_UNIQ;
    }
    struct t_list_node *current = queue_list->head;
    while(current != NULL) {
        if (strcmp(current->key, uri) == 0) {
            return JUKEBOX_UNIQ_IN_QUEUE;
        }
        if (value != NULL &&
            strcmp(current->value_p, value) == 0)
        {
            return JUKEBOX_UNIQ_IN_QUEUE;
        }
        current = current->next;
    }

    current = manual == false
        ? partition_state->jukebox_queue.head
        : partition_state->jukebox_queue_tmp.head;
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
    return JUKEBOX_UNIQ_IS_UNIQ;
}

/**
 * Checks for the uniq tag constraint for albums
 * @param partition_state pointer to myMPD partition state
 * @param album album name
 * @param albumartist albumartist string
 * @param manual false = normal jukebox operation
 *               true = create separate jukebox queue and add songs to queue once
 * @param queue_list list of current songs in mpd queue and last played
 * @return value matches in the mpd queue: JUKEBOX_UNIQ_IN_QUEUE
 *         value matches in the jukebox queue: position in the jukebox queue
 *         value does not match, is uniq: JUKEBOX_UNIQ_IS_UNIQ
 */
static long jukebox_unique_album(struct t_partition_state *partition_state, const char *album,
        const char *albumartist, bool manual, struct t_list *queue_list)
{
    if (partition_state->jukebox_enforce_unique == false) {
        return JUKEBOX_UNIQ_IS_UNIQ;
    }
    struct t_list_node *current = queue_list->head;
    while (current != NULL) {
        if (strcmp(current->key, album) == 0 &&
            strcmp(current->value_p, albumartist) == 0)
        {
            return JUKEBOX_UNIQ_IN_QUEUE;
        }
        current = current->next;
    }

    current = manual == false
        ? partition_state->jukebox_queue.head
        : partition_state->jukebox_queue_tmp.head;
    long i = 0;
    while (current != NULL) {
        if (strcmp(current->key, album) == 0 &&
            strcmp(current->value_p, albumartist) == 0)
        {
            return i;
        }
        current = current->next;
        i++;
    }
    return JUKEBOX_UNIQ_IS_UNIQ;
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
 * Checks if the song is not hated and jukebox_ignore_hated is true
 * @param stickers_like like stickers
 * @param uri uri to check against the stickers_like
 * @param jukebox_ignore_hated ignore hated value
 * @return true if song is not hated, else false
 */
static bool check_not_hated(rax *stickers_like, const char *uri, bool jukebox_ignore_hated) {
    if (stickers_like == NULL ||
        jukebox_ignore_hated == false)
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
    time_t sticker_last_played = sticker_value_last_played == raxNotFound
        ? 0
        : strtol((sds)sticker_value_last_played, NULL, 10);
    return sticker_last_played < since;
}

/**
 * Adds an expression if not empty, else adds an empty uri constraint to match all songs
 * @param include_expression include expression
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
static bool add_uri_constraint_or_expression(sds include_expression, struct t_partition_state *partition_state) {
    if (sdslen(include_expression) == 0) {
        return mpd_search_add_uri_constraint(partition_state->conn, MPD_OPERATOR_DEFAULT, "");
    }
    return mpd_search_add_expression(partition_state->conn, include_expression);
}
