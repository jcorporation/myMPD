/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_client/jukebox.h"

#include "dist/utf8/utf8.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/mympd_state.h"
#include "src/lib/random.h"
#include "src/lib/sds_extras.h"
#include "src/lib/sticker_cache.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/search.h"
#include "src/mpd_client/search_local.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/queue.h"
#include "src/mympd_api/sticker.h"


#include <errno.h>
#include <string.h>

//private definitions
static bool jukebox(struct t_partition_state *partition_state);
static struct t_list *jukebox_get_last_played(struct t_partition_state *partition_state,
        enum jukebox_modes jukebox_mode);
static bool jukebox_run_fill_jukebox_queue(struct t_partition_state *partition_state,
        long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static bool jukebox_fill_jukebox_queue(struct t_partition_state *partition_state,
        long add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static long jukebox_unique_tag(struct t_partition_state *partition_state, const char *uri,
        const char *value, bool manual, struct t_list *queue_list);
static long jukebox_unique_album(struct t_partition_state *partition_state, const char *album,
        const char *albumartist, bool manual, struct t_list *queue_list);
static bool add_album_to_queue(struct t_partition_state *partition_state, struct mpd_song *album);
static long fill_jukebox_queue_songs(struct t_partition_state *partition_state, long add_songs,
        const char *playlist, bool manual, struct t_list *queue_list, struct t_list *add_list);
static long fill_jukebox_queue_albums(struct t_partition_state *partition_state, long add_albums,
        bool manual, struct t_list *queue_list, struct t_list *add_list);

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
 * Removes an entry from the jukebox queue.
 * @param list the jukebox queue
 * @param pos position to remove
 * @param partition_name name of the partition
 * @return true on success, else false
 */
bool jukebox_rm_entry(struct t_list *list, long pos, sds partition_name) {
    struct t_list_node *node = list_node_at(list, pos);
    if (node == NULL) {
        return false;
    }
    node->user_data = NULL;
    bool rc = list_remove_node(list, pos);
    //notify clients
    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_name);
    return rc;
}

/**
 * Clears the jukebox queue of all partitions.
 * @param mympd_state pointer to central myMPD state.
 * @param partition_name name of the partition
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
 * Clears the jukebox queue.
 * This is a simple wrapper around list_clear.
 * @param list the jukebox queue
 */
void jukebox_clear(struct t_list *list, sds partition_name) {
    list_clear(list);
    //notify clients
    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_name);
}

/**
 * Prints the jukebox queue as an jsonrpc response
 * @param partition_state pointer to myMPD partition state
 * @param buffer already allocated sds string to append the result
 * @param cmd_id jsonrpc method
 * @param request_id jsonrpc request id
 * @param offset offset for printing
 * @param limit max entries to print
 * @param searchstr string to search
 * @param tagcols columns to print
 * @return pointer to buffer
 */
sds jukebox_list(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id, long request_id,
        long offset, long limit, sds searchstr, const struct t_tags *tagcols)
{
    long entity_count = 0;
    long entities_returned = 0;
    long real_limit = offset + limit;

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    if (partition_state->jukebox_mode == JUKEBOX_ADD_SONG) {
        struct t_list_node *current = partition_state->jukebox_queue.head;
        while (current != NULL) {
            bool rc = mpd_send_list_meta(partition_state->conn, current->key);
            if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_send_list_meta") == true) {
                struct mpd_song *song;
                if ((song = mpd_recv_song(partition_state->conn)) != NULL) {
                    if (search_mpd_song(song, searchstr, tagcols) == true) {
                        if (entity_count >= offset &&
                            entity_count < real_limit)
                        {
                            if (entities_returned++) {
                                buffer = sdscatlen(buffer, ",", 1);
                            }
                            buffer = sdscatlen(buffer, "{", 1);
                            buffer = tojson_long(buffer, "Pos", entity_count, true);
                            buffer = get_song_tags(buffer, partition_state->mpd_state->feat_tags, tagcols, song);
                            if (partition_state->mpd_state->feat_stickers == true &&
                                partition_state->mpd_state->sticker_cache.cache != NULL)
                            {
                                buffer = sdscatlen(buffer, ",", 1);
                                buffer = mympd_api_sticker_get_print(buffer, &partition_state->mpd_state->sticker_cache, mpd_song_get_uri(song));
                            }
                            buffer = sdscatlen(buffer, "}", 1);
                        }
                        entity_count++;
                    }
                    mpd_song_free(song);
                }
            }
            mpd_response_finish(partition_state->conn);
            mympd_check_error_and_recover(partition_state);
            current = current->next;
        }
    }
    else if (partition_state->jukebox_mode == JUKEBOX_ADD_ALBUM) {
        struct t_list_node *current = partition_state->jukebox_queue.head;
        while (current != NULL) {
            if (utf8casestr(current->key, searchstr) != NULL ||
                utf8casestr(current->value_p, searchstr) != NULL)
            {
                if (entity_count >= offset &&
                    entity_count < real_limit)
                {
                    if (entities_returned++) {
                        buffer = sdscatlen(buffer, ",", 1);
                    }
                    struct mpd_song *album = (struct mpd_song *)current->user_data;
                    buffer = sdscatlen(buffer, "{", 1);
                    buffer = tojson_long(buffer, "Pos", entity_count, true);
                    buffer = tojson_char(buffer, "uri", "Album", true);
                    buffer = tojson_char(buffer, "Title", "", true);
                    buffer = tojson_char(buffer, "Album", current->key, true);
                    buffer = sdscat(buffer, "\"AlbumArtist\":");
                    buffer = mpd_client_get_tag_values(album, MPD_TAG_ALBUM_ARTIST, buffer);
                    buffer = sdscat(buffer, ",\"Artist\":");
                    buffer = mpd_client_get_tag_values(album, MPD_TAG_ARTIST, buffer);
                    buffer = sdscatlen(buffer, "}", 1);
                }
                entity_count++;
            }
            current = current->next;
        }
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "jukeboxMode", partition_state->jukebox_mode, true);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);

    return buffer;
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
         MYMPD_LOG_ERROR("\"%s\": Jukebox: trying again, attempt %d", partition_state->name, i);
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
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status == NULL) {
        mympd_check_error_and_recover(partition_state);
        return false;
    }
    long queue_length = (long)mpd_status_get_queue_length(status);
    mpd_status_free(status);

    time_t now = time(NULL);
    time_t add_time = partition_state->song_end_time - (partition_state->crossfade + 10);

    MYMPD_LOG_DEBUG("\"%s\": Queue length: %ld", partition_state->name, queue_length);
    MYMPD_LOG_DEBUG("\"%s\": Min queue length: %ld", partition_state->name, partition_state->jukebox_queue_length);

    if (queue_length >= partition_state->jukebox_queue_length && now < add_time) {
        MYMPD_LOG_DEBUG("\"%s\": Jukebox: Queue length >= %ld and add_time not reached", partition_state->name, partition_state->jukebox_queue_length);
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
        MYMPD_LOG_DEBUG("\"%s\": Time now %lld greater than add_time %lld, adding %ld song(s)", partition_state->name, (long long)now, (long long)add_time, add_songs);
    }

    if (add_songs < 1) {
        MYMPD_LOG_DEBUG("\"%s\": Jukebox: nothing to do", partition_state->name);
        return true;
    }

    if (add_songs > 99) {
        MYMPD_LOG_WARN("\"%s\": Jukebox: max songs to add set to %ld, adding max. 99 songs", partition_state->name, add_songs);
        add_songs = 99;
    }

    if (partition_state->mpd_state->feat_playlists == false && strcmp(partition_state->jukebox_playlist, "Database") != 0) {
        MYMPD_LOG_WARN("\"%s\": Jukebox: Playlists are disabled", partition_state->name);
        return true;
    }

    bool rc = jukebox_add_to_queue(partition_state, add_songs, partition_state->jukebox_mode, partition_state->jukebox_playlist, false);

    //update playback state
    mympd_api_queue_status(partition_state, NULL);
    if (partition_state->play_state != MPD_STATE_PLAY) {
        MYMPD_LOG_DEBUG("\"%s\": Jukebox: start playback", partition_state->name);
        rc = mpd_run_play(partition_state->conn);
        mympd_check_rc_error_and_recover(partition_state, rc, "mpd_run_play");    
    }

    if (rc == true) {
        //notify clients
        send_jsonrpc_event(JSONRPC_EVENT_UPDATE_JUKEBOX, partition_state->name);
        return true;
    }

    MYMPD_LOG_DEBUG("\"%s\": Jukebox mode: %d", partition_state->name, partition_state->jukebox_mode);
    MYMPD_LOG_ERROR("\"%s\": Jukebox: Error adding song(s)", partition_state->name);
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
        MYMPD_LOG_DEBUG("\"%s\": Jukebox queue length: %ld", partition_state->name, partition_state->jukebox_queue.length);
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
	        bool rc = mpd_run_add(partition_state->conn, current->key);
            if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_run_add") == true) {
	            MYMPD_LOG_NOTICE("\"%s\": Jukebox adding song: %s", partition_state->name, current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR("\"%s\": Jukebox adding song %s failed", partition_state->name, current->key);
            }
        }
        else {
            bool rc = add_album_to_queue(partition_state, (struct mpd_song *)current->user_data);
            if (rc == true) {
                MYMPD_LOG_NOTICE("\"%s\": Jukebox adding album: %s - %s", partition_state->name, current->value_p, current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR("\"%s\": Jukebox adding album %s - %s failed", partition_state->name, current->value_p, current->key);
            }
        }
        if (manual == false) {
            partition_state->jukebox_queue.head->user_data = NULL;
            jukebox_rm_entry(&partition_state->jukebox_queue, 0, partition_state->name);
            current = partition_state->jukebox_queue.head;
        }
        else {
            partition_state->jukebox_queue_tmp.head->user_data = NULL;
            jukebox_rm_entry(&partition_state->jukebox_queue_tmp, 0, partition_state->name);
            current = partition_state->jukebox_queue_tmp.head;
        }
    }
    if (added == 0) {
        MYMPD_LOG_ERROR("\"%s\": Error adding song(s)", partition_state->name);
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
        MYMPD_LOG_DEBUG("\"%s\": Jukebox queue length: %ld", partition_state->name, partition_state->jukebox_queue.length);
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
    bool rc = mpd_search_add_db_songs(partition_state->conn, true);
    if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_search_add_db_songs") == false) {
        mpd_search_cancel(partition_state->conn);
        return false;
    }

    const char *value = NULL;
    unsigned i = 0;
    sds expression = sdsnewlen("(", 1);
    while ((value = mpd_song_get_tag(album, partition_state->mpd_state->tag_albumartist, i)) != NULL) {
        expression = escape_mpd_search_expression(expression, mpd_tag_name(partition_state->mpd_state->tag_albumartist), "==", value);
        expression = sdscat(expression, " AND ");
        i++;
    }
    expression = escape_mpd_search_expression(expression, "Album", "==", mpd_song_get_tag(album, MPD_TAG_ALBUM, 0));
    expression = sdscatlen(expression, ")", 1);

    rc = mpd_search_add_expression(partition_state->conn, expression);
    FREE_SDS(expression);
    if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_search_add_expression") == false) {
        mpd_search_cancel(partition_state->conn);
        return false;
    }
    rc = mpd_search_commit(partition_state->conn);
    mpd_response_finish(partition_state->conn);
    return mympd_check_rc_error_and_recover(partition_state, rc, "mpd_search_commit");
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

    bool rc = mpd_send_list_queue_meta(partition_state->conn);
    if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_send_list_queue_meta") == false) {
        FREE_PTR(queue_list);
        return NULL;
    }

    sds tag_value = sdsempty();
    sds album = sdsempty();
    sds albumartist = sdsempty();

    while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
            if (partition_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
                tag_value = mpd_client_get_tag_value_string(song, partition_state->jukebox_unique_tag.tags[0], tag_value);
            }
            list_push(queue_list, mpd_song_get_uri(song), 0, tag_value, NULL);
            sdsclear(tag_value);
        }
        else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
            album = mpd_client_get_tag_value_string(song, MPD_TAG_ALBUM, album);
            albumartist = mpd_client_get_tag_value_string(song, partition_state->mpd_state->tag_albumartist, albumartist);
            list_push(queue_list, album, 0, albumartist, NULL);
            sdsclear(album);
            sdsclear(albumartist);
        }
        mpd_song_free(song);
    }

    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state);

    //append last_played to queue list
    struct t_list_node *current = partition_state->last_played.head;
    while (current != NULL) {
        rc = mpd_send_list_meta(partition_state->conn, current->key);
        if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_send_list_meta") == true) {
            song = mpd_recv_song(partition_state->conn);
            if (song != NULL) {
                if (jukebox_mode == JUKEBOX_ADD_SONG) {
                    if (partition_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
                        tag_value = mpd_client_get_tag_value_string(song, partition_state->jukebox_unique_tag.tags[0], tag_value);
                    }
                    list_push(queue_list, current->key, 0, tag_value, NULL);
                    sdsclear(tag_value);
                }
                else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
                    album = mpd_client_get_tag_value_string(song, MPD_TAG_ALBUM, album);
                    albumartist = mpd_client_get_tag_value_string(song, partition_state->mpd_state->tag_albumartist, albumartist);
                    list_push(queue_list, album, 0, albumartist, NULL);
                    sdsclear(album);
                    sdsclear(albumartist);
                }
                mpd_song_free(song);
            }
        }
        mpd_response_finish(partition_state->conn);
        mympd_check_error_and_recover(partition_state);
        current = current->next;
    }

    //get last_played from disc
    sds lp_file = sdscatfmt(sdsempty(), "%S/%S/%s",
        partition_state->mympd_state->config->workdir, partition_state->state_dir, FILENAME_LAST_PLAYED);
    errno = 0;
    FILE *fp = fopen(lp_file, OPEN_FLAGS_READ);
    if (fp != NULL) {
        sds line = sdsempty();
        while (sds_getline(&line, fp, LINE_LENGTH_MAX) >= 0 &&
                queue_list->length < 50)
        {
            sds uri = NULL;
            if (json_get_string_max(line, "$.uri", &uri, vcb_isfilepath, NULL) == true) {
                rc = mpd_send_list_meta(partition_state->conn, uri);
                if (mympd_check_rc_error_and_recover(partition_state, rc, "mpd_send_list_meta") == true) {
                    song = mpd_recv_song(partition_state->conn);
                    if (song != NULL) {
                        if (jukebox_mode == JUKEBOX_ADD_SONG) {
                            if (partition_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
                                tag_value = mpd_client_get_tag_value_string(song, partition_state->jukebox_unique_tag.tags[0], tag_value);
                            }
                            list_push(queue_list, uri, 0, tag_value, NULL);
                            sdsclear(tag_value);
                        }
                        else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
                            album = mpd_client_get_tag_value_string(song, MPD_TAG_ALBUM, album);
                            albumartist = mpd_client_get_tag_value_string(song, partition_state->mpd_state->tag_albumartist, albumartist);
                            list_push(queue_list, album, 0, albumartist, NULL);
                            sdsclear(album);
                            sdsclear(albumartist);
                        }
                        mpd_song_free(song);
                    }
                }
                mpd_response_finish(partition_state->conn);
                mympd_check_error_and_recover(partition_state);
            }
            else {
                MYMPD_LOG_ERROR("\"%s\": Reading last_played line failed", partition_state->name);
                MYMPD_LOG_DEBUG("\"%s\": Erroneous line: %s", partition_state->name, line);
            }
            FREE_SDS(uri);
        }
        (void) fclose(fp);
        FREE_SDS(line);
    }
    else {
        MYMPD_LOG_DEBUG("Can not open \"%s\"", lp_file);
        if (errno != ENOENT) {
            //ignore missing last_played file
            MYMPD_LOG_ERRNO(errno);
        }
    }
    FREE_SDS(lp_file);

    FREE_SDS(album);
    FREE_SDS(albumartist);
    FREE_SDS(tag_value);
    MYMPD_LOG_DEBUG("\"%s\": Jukebox last_played list length: %ld", partition_state->name, queue_list->length);
    return queue_list;
}

/**
 * Wrapper function for the real jukebox queue filling function.
 * It keeps track of enabling/disabling tags.
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
    MYMPD_LOG_DEBUG("\"%s\": Jukebox queue to small, adding entities", partition_state->name);
    if (partition_state->mpd_state->feat_tags == true) {
        if (partition_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
            enable_mpd_tags(partition_state, &partition_state->jukebox_unique_tag);
        }
        else {
            disable_all_mpd_tags(partition_state);
        }
    }
    bool rc = jukebox_fill_jukebox_queue(partition_state, add_songs, jukebox_mode, playlist, manual);
    if (partition_state->mpd_state->feat_tags == true) {
        enable_mpd_tags(partition_state, &partition_state->mpd_state->tags_mympd);
    }

    if (rc == false) {
        MYMPD_LOG_ERROR("\"%s\": Filling jukebox queue failed, disabling jukebox", partition_state->name);
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

    struct t_list *add_list = manual == false ? &partition_state->jukebox_queue : &partition_state->jukebox_queue_tmp;

    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        added = fill_jukebox_queue_songs(partition_state, add_songs, playlist, manual, queue_list, add_list);
    }
    else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
        added = fill_jukebox_queue_albums(partition_state, add_songs, manual, queue_list, add_list);
    }

    if (added < add_songs) {
        MYMPD_LOG_WARN("\"%s\": Jukebox queue didn't contain %ld entries", partition_state->name, add_songs);
        if (partition_state->jukebox_enforce_unique == true) {
            MYMPD_LOG_WARN("\"%s\": Disabling jukebox unique constraints temporarily", partition_state->name);
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
        MYMPD_LOG_WARN("\"%s\": Album cache is null, jukebox can not add albums", partition_state->name);
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
    long nkeep = 0;
    long lineno = 1;
    time_t since = time(NULL);
    since = since - (partition_state->jukebox_last_played * 3600);
    raxIterator iter;
    raxStart(&iter, partition_state->mpd_state->album_cache.cache);
    raxSeek(&iter, "^", NULL, 0);
    sds tag_album = sdsempty();
    sds tag_albumartist = sdsempty();
    while (raxNext(&iter)) {
        struct mpd_song *album= (struct mpd_song *)iter.data;
        sdsclear(tag_album);
        sdsclear(tag_albumartist);
        tag_album = mpd_client_get_tag_value_string(album, MPD_TAG_ALBUM, tag_album);
        tag_albumartist = mpd_client_get_tag_value_string(album, partition_state->mpd_state->tag_albumartist, tag_albumartist);
        long is_uniq = JUKEBOX_UNIQ_IS_UNIQ;

        //we use the song uri in the album cache for enforcing last_played constraint
        //because we do not know if an album was last played fully
        const char *uri = mpd_song_get_uri(album);
        struct t_sticker *sticker = get_sticker_from_cache(&partition_state->mpd_state->sticker_cache, uri);
        time_t last_played = sticker != NULL ? sticker->last_played : 0;

        if (last_played > since) {
            //album was played too recently
            is_uniq = JUKEBOX_UNIQ_IN_QUEUE;
        }
        else if (partition_state->jukebox_enforce_unique == true) {
            is_uniq = jukebox_unique_album(partition_state, tag_album, tag_albumartist, manual, queue_list);
        }

        if (is_uniq == JUKEBOX_UNIQ_IS_UNIQ) {
            if (randrange(0, lineno) < add_albums) {
                if (nkeep < add_albums) {
                    if (list_push(add_list, tag_album, lineno, tag_albumartist, album) == true) {
                        nkeep++;
                    }
                    else {
                        MYMPD_LOG_ERROR("\"%s\": Can't push jukebox_queue element", partition_state->name);
                    }
                }
                else {
                    long pos = add_albums > 1 ? start_length + randrange(0, add_albums -1) : 0;
                    struct t_list_node *node = list_node_at(add_list, pos);
                    if (node != NULL) {
                        node->user_data = NULL;
                        if (list_replace(add_list, pos, tag_album, lineno, tag_albumartist, album) == false) {
                            MYMPD_LOG_ERROR("\"%s\": Can't replace jukebox_queue element pos %ld", partition_state->name, pos);
                        }
                    }
                    else {
                        MYMPD_LOG_ERROR("\"%s\": Can't replace jukebox_queue element pos %ld", partition_state->name, pos);
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
    MYMPD_LOG_DEBUG("\"%s\": Jukebox iterated through %ld albums, skipped %ld", partition_state->name, lineno, skipno);
    return (int)nkeep;
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
    long nkeep = 0;
    long lineno = 1;
    time_t since = time(NULL);
    since = since - (partition_state->jukebox_last_played * 3600);

    if (partition_state->mpd_state->sticker_cache.cache == NULL) {
        MYMPD_LOG_WARN("\"%s\": Sticker cache is null, jukebox doesn't respect last played constraint", partition_state->name);
    }

    long start_length = 0;
    if (manual == false) {
        start_length = partition_state->jukebox_queue.length;
        add_songs = (long)50 - start_length;
        if (add_songs <= 0) {
            return 0;
        }
    }
    bool from_database = strcmp(playlist, "Database") == 0 ? true : false;
    sds tag_value = sdsempty();
    do {
        MYMPD_LOG_DEBUG("\"%s\": Jukebox: iterating through source, start: %u", partition_state->name, start);

        if (from_database == true) {
            if (mpd_search_db_songs(partition_state->conn, false) == false) {
                MYMPD_LOG_ERROR("\"%s\": Error in response to command: mpd_search_db_songs", partition_state->name);
            }
            else if (mpd_search_add_uri_constraint(partition_state->conn, MPD_OPERATOR_DEFAULT, "") == false) {
                MYMPD_LOG_ERROR("\"%s\": Error in response to command: mpd_search_add_uri", partition_state->name);
                mpd_search_cancel(partition_state->conn);
            }
            else if (mpd_search_add_window(partition_state->conn, start, end) == false) {
                MYMPD_LOG_ERROR("\"%s\": Error in response to command: mpd_search_add_window", partition_state->name);
                mpd_search_cancel(partition_state->conn);
            }
            else if (mpd_search_commit(partition_state->conn) == false) {
                MYMPD_LOG_ERROR("\"%s\": Error in response to command: mpd_search_commit", partition_state->name);
                mpd_search_cancel(partition_state->conn);
            }
        }
        else {
            if (mpd_send_list_playlist_meta(partition_state->conn, playlist) == false) {
                MYMPD_LOG_ERROR("\"%s\": Error in response to command: mpd_send_list_playlist_meta", partition_state->name);
            }
        }

        if (mympd_check_error_and_recover(partition_state) == false) {
            FREE_SDS(tag_value);
            return -1;
        }
        struct mpd_song *song;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            sdsclear(tag_value);
            tag_value = mpd_client_get_tag_value_string(song, partition_state->jukebox_unique_tag.tags[0], tag_value);

            const char *uri = mpd_song_get_uri(song);
            struct t_sticker *sticker = get_sticker_from_cache(&partition_state->mpd_state->sticker_cache, uri);
            time_t last_played = sticker != NULL
                ? sticker->last_played
                : 0;
            bool is_hated = sticker != NULL &&
                sticker->like == STICKER_LIKE_HATE
                    ? partition_state->jukebox_ignore_hated == true
                    : false;

            long is_uniq = JUKEBOX_UNIQ_IS_UNIQ;
            if (last_played > since) {
                //song was played too recently
                is_uniq = JUKEBOX_UNIQ_IN_QUEUE;
            }
            else if (partition_state->jukebox_enforce_unique == true) {
                is_uniq = jukebox_unique_tag(partition_state, uri, tag_value, manual, queue_list);
            }

            if (is_uniq == JUKEBOX_UNIQ_IS_UNIQ &&
                is_hated == false)
            {
                if (randrange(0, lineno) < add_songs) {
                    if (nkeep < add_songs) {
                        if (list_push(add_list, uri, lineno, tag_value, NULL) == true) {
                            nkeep++;
                        }
                        else {
                            MYMPD_LOG_ERROR("\"%s\": Can't push jukebox_queue element", partition_state->name);
                        }
                    }
                    else {
                        long pos = add_songs > 1 ? start_length + randrange(0, add_songs - 1) : 0;
                        if (list_replace(add_list, pos, uri, lineno, tag_value, NULL) == false) {
                            MYMPD_LOG_ERROR("\"%s\": Can't replace jukebox_queue element pos %ld", partition_state->name, pos);
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
        if (mympd_check_error_and_recover(partition_state) == false) {
            FREE_SDS(tag_value);
            return -1;
        }
        start = end;
        end = end + MPD_RESULTS_MAX;
    } while (from_database == true && lineno + skipno > (long)start);
    FREE_SDS(tag_value);
    MYMPD_LOG_DEBUG("\"%s\": Jukebox iterated through %ld songs, skipped %ld", partition_state->name, lineno, skipno);
    return (int)nkeep;
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

    current = manual == false ? partition_state->jukebox_queue.head : partition_state->jukebox_queue_tmp.head;
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
    struct t_list_node *current = queue_list->head;
    while (current != NULL) {
        if (strcmp(current->key, album) == 0 &&
            strcmp(current->value_p, albumartist) == 0)
        {
            return JUKEBOX_UNIQ_IN_QUEUE;
        }
        current = current->next;
    }

    current = manual == false ? partition_state->jukebox_queue.head : partition_state->jukebox_queue_tmp.head;
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
