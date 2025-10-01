/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD queue API
 */

#include "compile_time.h"
#include "src/mympd_api/queue.h"

#include "src/lib/cache/cache_rax_album.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mympd_api/sticker.h"
#include "src/mympd_api/webradio.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/queue.h"
#include "src/mympd_client/search.h"
#include "src/mympd_client/shortcuts.h"
#include "src/mympd_client/stickerdb.h"
#include "src/mympd_client/tags.h"

#include <limits.h>
#include <string.h>

/**
 * Private definitions
 */
static bool add_queue_search_adv_params(struct t_partition_state *partition_state,
        sds sort, bool sortdesc, unsigned offset, unsigned limit);
sds print_queue_entry(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, const struct t_fields *tagcols, bool print_stickers, struct mpd_song *song);

/**
 * Public functions
 */

/**
 * Saves the queue as a playlist
 * @param partition_state 
 * @param name pointer to partition state
 * @param mode mpd queue save mode
 * @param error pointer to an already allocated sds string for the error message
 * @return bool true on success, else false
 */
bool mympd_api_queue_save(struct t_partition_state *partition_state, sds name, sds mode, sds *error) {
    if (partition_state->mpd_state->feat.advqueue == true) {
        enum mpd_queue_save_mode save_mode = mpd_parse_queue_save_mode(mode);
        if (save_mode == MPD_QUEUE_SAVE_MODE_UNKNOWN) {
            *error = sdscat(*error, "Unknown queue save mode");
            return false;
        }
        mpd_run_save_queue(partition_state->conn, name, save_mode);
        return mympd_check_error_and_recover(partition_state, error, "mpd_run_save_queue");

    }
    mpd_run_save(partition_state->conn, name);
    return mympd_check_error_and_recover(partition_state, error, "mpd_run_save");
}

/**
 * Removes songs defined by id from the queue
 * @param partition_state pointer to partition state
 * @param song_ids list of song_ids to remove
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_rm_song_ids(struct t_partition_state *partition_state, struct t_list *song_ids, sds *error) {
    if (song_ids->length == 0) {
        *error = sdscat(*error, "No MPD queue song ids provided");
        return false;
    }
    if (mpd_command_list_begin(partition_state->conn, false)) {
        struct t_list_node *current;
        while ((current = list_shift_first(song_ids)) != NULL) {
            bool rc = mpd_send_delete_id(partition_state->conn, (unsigned)current->value_i);
            list_node_free(current);
            if (rc == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_delete_id");
                break;
            }
        }
        mympd_client_command_list_end_check(partition_state);
    }
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_delete_id");
}

/**
 * Sets the priority of entries in the queue.
 * The priority has only an effect in random mode.
 * @param partition_state pointer to partition state
 * @param song_ids song ids in the queue
 * @param priority priority to set, max 255
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_prio_set(struct t_partition_state *partition_state, struct t_list *song_ids, unsigned priority, sds *error) {
    if (song_ids->length == 0) {
        *error = sdscat(*error, "No MPD queue song ids provided");
        return false;
    }
    if (mpd_command_list_begin(partition_state->conn, false)) {
        struct t_list_node *current;
        while ((current = list_shift_first(song_ids)) != NULL) {
            bool rc = mpd_send_prio_id(partition_state->conn, priority, (unsigned)current->value_i);
            list_node_free(current);
            if (rc == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_prio_id");
                break;
            }
        }
        mympd_client_command_list_end_check(partition_state);
    }
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_prio_id");
}

/**
 * Sets the priority to the highest value of a song in the queue.
 * The priority has only an effect in random mode.
 * @param partition_state pointer to partition state
 * @param song_ids song ids in the queue
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_prio_set_highest(struct t_partition_state *partition_state, struct t_list *song_ids, sds *error) {
    if (song_ids->length == 0) {
        *error = sdscat(*error, "No MPD queue song ids provided");
        return false;
    }
    //default prio is 0
    unsigned priority = 1;
    int next_song_id = -1;
    //try to get prio of next song
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    if (status != NULL) {
        next_song_id = mpd_status_get_next_song_id(status);
        mpd_status_free(status);
    }
    if (mympd_check_error_and_recover(partition_state, error, "mpd_run_status") == false) {
        return false;
    }
    
    if (next_song_id > -1 ) {
        bool rc = mpd_send_get_queue_song_id(partition_state->conn, (unsigned)next_song_id);
        if (rc == true) {
            struct mpd_song *song = mpd_recv_song(partition_state->conn);
            if (song != NULL) {
                priority = mpd_song_get_prio(song);
                priority++;
                mpd_song_free(song);
            }
        }
        if (mympd_check_error_and_recover(partition_state, error, "mpd_send_get_queue_song_id") == false) {
            return false;
        }
    }
    if (priority > MPD_QUEUE_PRIO_MAX) {
        MYMPD_LOG_WARN(partition_state->name, "MPD queue priority limit reached, setting it to max %d", MPD_QUEUE_PRIO_MAX);
        priority = MPD_QUEUE_PRIO_MAX;
    }
    return mympd_api_queue_prio_set(partition_state, song_ids, priority, error);
}

/**
 * Moves song ids to relative position after current song
 * @param partition_state pointer to partition state
 * @param song_ids song ids to move
 * @param to relative position
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return bool true on success, else false
 */
bool mympd_api_queue_move_relative(struct t_partition_state *partition_state, struct t_list *song_ids, unsigned to, unsigned whence, sds *error) {
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat.whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    if (song_ids->length == 0) {
        *error = sdscat(*error, "No MPD queue song ids provided");
        return false;
    }
    if (mpd_command_list_begin(partition_state->conn, false)) {
        struct t_list_node *current = song_ids->head;
        while (current != NULL) {
            if (mpd_send_move_id_whence(partition_state->conn, (unsigned)current->value_i, to, whence) == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_move_id_whence");
                break;
            }
            current = current->next;
            to++;
        }
        mympd_client_command_list_end_check(partition_state);
    }
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_move_id_whence");
}

/**
 * Inserts an uri to the queue and sets tag values for it.
 * @param partition_state pointer to partition state
 * @param uri uri to add to the queue
 * @param tags list of tags with values to set
 * @param to where to insert the uri
 * @param whence How to interprete the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return bool true on success, else false
 */
bool mympd_api_queue_insert_uri_tags(struct t_partition_state *partition_state, sds uri,
        struct t_list *tags, unsigned to, unsigned whence, sds *error)
{
    int id = to == UINT_MAX
        ? mpd_run_add_id(partition_state->conn, uri)
        : mpd_run_add_id_whence(partition_state->conn, uri, to, whence);
    if (id == -1) {
        mympd_check_error_and_recover(partition_state, error, "mpd_run_add_id_whence");
        if (sdslen(*error) == 0) {
            *error = sdscat(*error, "Failure getting inserted song id");
        }
        return false;
    }
    if (mpd_command_list_begin(partition_state->conn, false)) {
        struct t_list_node *current = tags->head;
        while (current != NULL) {
            enum mpd_tag_type tag = mpd_tag_name_iparse(current->key);
            if (mpd_send_add_tag_id(partition_state->conn, (unsigned)id, tag, current->value_p) == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_add_tag_id");
                break;
            }
            current = current->next;
            to++;
        }
        mympd_client_command_list_end_check(partition_state);
    }
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_add_tag_id");
}

/**
 * Appends an uri to the queue and sets tag values for it.
 * @param partition_state pointer to partition state
 * @param uri uri to add to the queue
 * @param tags list of tags with values to set
 * @param error pointer to an already allocated sds string for the error message
 * @return bool true on success, else false
 */
bool mympd_api_queue_append_uri_tags(struct t_partition_state *partition_state, sds uri, struct t_list *tags, sds *error) {
    return mympd_api_queue_insert_uri_tags(partition_state, uri, tags, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with the uri and sets tag values for it.
 * @param partition_state pointer to partition state
 * @param uri uri to add to the queue
 * @param tags list of tags with values to set
 * @param error pointer to an already allocated sds string for the error message
 * @return bool true on success, else false
 */
bool mympd_api_queue_replace_uri_tags(struct t_partition_state *partition_state, sds uri, struct t_list *tags, sds *error) {
    return mympd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_uri_tags(partition_state, uri, tags, error);
}

/**
 * Inserts an uri to the queue and resumes playback.
 * @param partition_state pointer to partition state
 * @param stickerdb pointer to stickerdb
 * @param uri uri to add to the queue
 * @param to where to insert the uri
 * @param whence How to interprete the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return bool true on success, else false
 */
bool mympd_api_queue_insert_uri_resume(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        sds uri, unsigned to, unsigned whence, sds *error)
{
    int64_t elapsed = stickerdb_get_int64(stickerdb, STICKER_TYPE_SONG, uri, "elapsed");
    int id = to == UINT_MAX
        ? mpd_run_add_id(partition_state->conn, uri)
        : mpd_run_add_id_whence(partition_state->conn, uri, to, whence);
    if (id == -1) {
        mympd_check_error_and_recover(partition_state, error, "mpd_run_add_id_whence");
        if (sdslen(*error) == 0) {
            *error = sdscat(*error, "Failure getting inserted song id");
        }
        return false;
    }
    if (mpd_command_list_begin(partition_state->conn, false)) {
        mpd_send_play_id(partition_state->conn, (unsigned)id);
        if (elapsed > 0) {
            mpd_send_seek_id(partition_state->conn, (unsigned)id, (unsigned)elapsed);
        }
        mympd_client_command_list_end_check(partition_state);
    }
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_seek_id");
}

/**
 * Appends an uri to the queue and resumes playback.
 * @param partition_state pointer to partition state
 * @param stickerdb pointer to stickerdb
 * @param uri uri to add to the queue
 * @param error pointer to an already allocated sds string for the error message
 * @return bool true on success, else false
 */
bool mympd_api_queue_append_uri_resume(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        sds uri, sds *error)
{
    return mympd_api_queue_insert_uri_resume(partition_state, stickerdb, uri, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with the uri and resumes playback.
 * @param partition_state pointer to partition state
 * @param stickerdb pointer to stickerdb
 * @param uri uri to add to the queue
 * @param error pointer to an already allocated sds string for the error message
 * @return bool true on success, else false
 */
bool mympd_api_queue_replace_uri_resume(struct t_partition_state *partition_state, struct t_stickerdb_state *stickerdb,
        sds uri, sds *error)
{
    return mympd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_uri_resume(partition_state, stickerdb, uri, error);
}

/**
 * Insert uris into the queue
 * @param partition_state pointer to partition state
 * @param uris uris to insert
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_insert(struct t_partition_state *partition_state, struct t_list *uris, unsigned to, unsigned whence, sds *error) {
    return mympd_client_add_uris_to_queue(partition_state, uris, to, whence, error);
}

/**
 * Appends uris to the queue
 * @param partition_state pointer to partition state
 * @param uris uris to append
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append(struct t_partition_state *partition_state, struct t_list *uris, sds *error) {
    return mympd_client_add_uris_to_queue(partition_state, uris, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with uris
 * @param partition_state pointer to partition state
 * @param uris uris to add
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace(struct t_partition_state *partition_state, struct t_list *uris, sds *error) {
    return mympd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append(partition_state, uris, error);
}

/**
 * Inserts search results into the queue
 * @param partition_state pointer to partition state
 * @param expression mpd search expression
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param sort sort by tag
 * @param sort_desc sort descending?
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_insert_search(struct t_partition_state *partition_state, sds expression,
        unsigned to, unsigned whence, const char *sort, bool sort_desc, sds *error)
{
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat.whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    return mympd_client_search_add_to_queue(partition_state, expression, to, whence, sort, sort_desc, error);
}

/**
 * Appends the search results to the queue
 * @param partition_state pointer to partition state
 * @param expression mpd search expression
 * @param sort sort by tag
 * @param sort_desc sort descending?
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append_search(struct t_partition_state *partition_state, sds expression,
        const char *sort, bool sort_desc, sds *error)
{
    return mympd_api_queue_insert_search(partition_state, expression, UINT_MAX, MPD_POSITION_ABSOLUTE, sort, sort_desc, error);
}

/**
 * Replaces the queue with the search result
 * @param partition_state pointer to partition state
 * @param expression mpd search expression
 * @param sort sort by tag
 * @param sort_desc sort descending?
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace_search(struct t_partition_state *partition_state, sds expression,
        const char *sort, bool sort_desc, sds *error)
{
    return mympd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_search(partition_state, expression, sort, sort_desc, error);
}

/**
 * Inserts albums into the queue
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
 * @param albumids album ids to insert
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_insert_albums(struct t_partition_state *partition_state, struct t_cache *album_cache,
    struct t_list *albumids, unsigned to, unsigned whence, sds *error)
{
    return mympd_client_add_albums_to_queue(partition_state, album_cache, albumids, to, whence, error);
}

/**
 * Appends albums to the queue
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
 * @param albumids album ids to append
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append_albums(struct t_partition_state *partition_state, struct t_cache *album_cache,
        struct t_list *albumids, sds *error)
{
    return mympd_client_add_albums_to_queue(partition_state, album_cache, albumids, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with albums
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
 * @param albumids album ids to insert
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace_albums(struct t_partition_state *partition_state, struct t_cache *album_cache,
        struct t_list *albumids, sds *error)
{
    return mympd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_albums(partition_state, album_cache, albumids, error);
}

/**
 * Inserts songs of an album filtered by tag into the queue
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
 * @param albumid album id to insert
 * @param tag MPD tag
 * @param value MPD tag value
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_insert_album_tag(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds albumid, enum mpd_tag_type tag, sds value, unsigned to, unsigned whence, sds *error)
{
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat.whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    struct t_album *mpd_album = album_cache_get_album(album_cache, albumid);
    if (mpd_album == NULL) {
        *error = sdscat(*error, "Album not found");
        return false;
    }
    sds expression = get_search_expression_album_tag(sdsempty(), partition_state->mpd_state->tag_albumartist,
        mpd_album, tag, value, &partition_state->config->albums);
    const char *sort = NULL;
    bool sortdesc = false;
    bool rc = mympd_client_search_add_to_queue(partition_state, expression, to, whence, sort, sortdesc, error);
    FREE_SDS(expression);
    return rc;
}

/**
 * Appends songs of an album filtered by tag to the queue
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
 * @param albumid album id to append
 * @param tag MPD tag
 * @param value MPD tag value
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append_album_tag(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds albumid, enum mpd_tag_type tag, sds value, sds *error)
{
    return mympd_api_queue_insert_album_tag(partition_state, album_cache, albumid, tag, value, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with an album filted by tag
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
 * @param albumid album id to insert
 * @param tag MPD tag
 * @param value MPD tag value
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace_album_tag(struct t_partition_state *partition_state,struct t_cache *album_cache,
        sds albumid, enum mpd_tag_type tag, sds value, sds *error)
{
    return mympd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_album_tag(partition_state, album_cache, albumid, tag, value, error);
}

/**
 * Inserts a range of song from an album into the queue
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
 * @param albumid album id to insert
 * @param start start of the range (including)
 * @param end end of the range (excluded)
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_insert_album_range(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds albumid, unsigned start, int end, unsigned to, unsigned whence, sds *error)
{
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat.whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    struct t_album *mpd_album = album_cache_get_album(album_cache, albumid);
    if (mpd_album == NULL) {
        *error = sdscat(*error, "Album not found");
        return false;
    }
    unsigned end_uint = end == -1
        ? UINT_MAX
        : (unsigned)end;
    sds expression = get_search_expression_album(sdsempty(), partition_state->mpd_state->tag_albumartist,
        mpd_album, &partition_state->config->albums);
    const char *sort = "Disc";
    bool sortdesc = false;
    bool rc = mympd_client_search_add_to_queue_window(partition_state, expression, to, whence,
        sort, sortdesc, start, end_uint, error);
    FREE_SDS(expression);
    return rc;
}

/**
 * Appends a range of song from an album to the queue
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
 * @param albumid album id to append
 * @param start start of the range (including)
 * @param end end of the range (excluded)
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append_album_range(struct t_partition_state *partition_state, struct t_cache *album_cache,
        sds albumid, unsigned start, int end, sds *error)
{
    return mympd_api_queue_insert_album_range(partition_state, album_cache, albumid, start, end, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with a range of song from an album
 * @param partition_state pointer to partition state
 * @param album_cache pointer to album cache
 * @param albumid album id to insert
 * @param start start of the range (including)
 * @param end end of the range (excluded)
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace_album_range(struct t_partition_state *partition_state,struct t_cache *album_cache,
        sds albumid, unsigned start, int end, sds *error)
{
    return mympd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_album_range(partition_state, album_cache, albumid, start, end, error);
}

/**
 * Inserts a playlist range into the queue
 * @param partition_state pointer to partition state
 * @param plist playlist to insert
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param start start of the range (including)
 * @param end end of the range (excluded)
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_insert_plist_range(struct t_partition_state *partition_state, sds plist,
        unsigned to, unsigned whence, unsigned start, int end, sds *error) {
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat.whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    plist = resolv_mympd_uri(plist, partition_state->mpd_state->mpd_host, partition_state->config, false);
    unsigned end_uint = end == -1
        ? UINT_MAX
        : (unsigned)end;
    if (to == UINT_MAX) {
        mpd_send_load_range(partition_state->conn, plist, start, end_uint);
    }
    else {
        mpd_send_load_range_to(partition_state->conn, plist, start, end_uint, to, whence);
    }
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_load_range_to");
}

/**
 * Appends a playlist range to the queue
 * @param partition_state pointer to partition state
 * @param plist playlist to append
 * @param start start of the range (including)
 * @param end end of the range (excluded)
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append_plist_range(struct t_partition_state *partition_state, sds plist,
        unsigned start, int end, sds *error)
{
    return mympd_api_queue_insert_plist_range(partition_state, plist, UINT_MAX, MPD_POSITION_ABSOLUTE, start, end, error);
}

/**
 * Replaces the queue with a playlist range
 * @param partition_state pointer to partition state
 * @param plist playlist to add
 * @param start start of the range (including)
 * @param end end of the range (excluded)
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace_plist_range(struct t_partition_state *partition_state, sds plist,
        unsigned start, int end, sds *error)
{
    return mympd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_plist_range(partition_state, plist, start, end, error);
}

/**
 * Insert playlists into the queue
 * @param partition_state pointer to partition state
 * @param plists playlists to insert
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_insert_plists(struct t_partition_state *partition_state, struct t_list *plists, unsigned to, unsigned whence, sds *error) {
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat.whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    if (plists->length == 0) {
        *error = sdscat(*error, "No playlists provided");
        return false;
    }
    if (mpd_command_list_begin(partition_state->conn, false)) {
        struct t_list_node *current = plists->head;
        while (current != NULL) {
            current->key = resolv_mympd_uri(current->key, partition_state->mpd_state->mpd_host, partition_state->config, false);
            bool rc = to == UINT_MAX
                ? mpd_send_load(partition_state->conn, current->key)
                : mpd_send_load_range_to(partition_state->conn, current->key, 0, UINT_MAX, to, whence);
            if (rc == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_load_range_to");
                break;
            }
            current = current->next;
        }
        mympd_client_command_list_end_check(partition_state);
    }
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_load_range_to");
}

/**
 * Appends playlists to the queue
 * @param partition_state pointer to partition state
 * @param plists playlists to append
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append_plists(struct t_partition_state *partition_state, struct t_list *plists, sds *error) {
    return mympd_api_queue_insert_plists(partition_state, plists, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with playlists
 * @param partition_state pointer to partition state
 * @param plists playlists to add
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace_plists(struct t_partition_state *partition_state, struct t_list *plists, sds *error) {
    return mympd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_plists(partition_state, plists, error);
}

/**
 * Crops (removes all but playing song) or clears the queue if no song is playing
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param cmd_id jsonrpc method
 * @param request_id jsonrpc id
 * @param or_clear if true: clears the queue if there is no current playing or paused song
 * @return pointer to buffer
 */
sds mympd_api_queue_crop(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id, unsigned request_id, bool or_clear) {
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    unsigned length = 0;
    int playing_song_pos = 0;
    if (status != NULL) {
        length = mpd_status_get_queue_length(status) - 1;
        playing_song_pos = mpd_status_get_song_pos(status);
        mpd_status_free(status);
    }
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_run_status") == false) {
        return buffer;
    }

    if (playing_song_pos > -1 &&
        length > 1)
    {
        //there is a current song, crop the queue
        if (mpd_command_list_begin(partition_state->conn, false)) {
            //remove all songs behind current song
            unsigned pos_after = (unsigned)playing_song_pos + 1;
            if (pos_after < length) {
                if (mpd_send_delete_range(partition_state->conn, pos_after, UINT_MAX) == false) {
                    mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_delete_range");
                }
            }
            //remove all songs before current song
            if (playing_song_pos > 0) {
                if (mpd_send_delete_range(partition_state->conn, 0, (unsigned)playing_song_pos) == false) {
                    mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_delete_range");
                }
            }
            mympd_client_command_list_end_check(partition_state);
        }
        if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_delete_range") == false) {
            return buffer;
        }
        buffer = jsonrpc_respond_ok(buffer, cmd_id, request_id, JSONRPC_FACILITY_QUEUE);
    }
    else if (or_clear == true) {
        //no current song
        mpd_run_clear(partition_state->conn);
        if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_run_clear") == true) {
            buffer = jsonrpc_respond_ok(buffer, cmd_id, request_id, JSONRPC_FACILITY_QUEUE);
        }
    }
    else {
        //queue can not be cropped
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_QUEUE, JSONRPC_SEVERITY_ERROR, "Can not crop the queue");
        MYMPD_LOG_ERROR(partition_state->name, "Can not crop the queue");
    }

    return buffer;
}

/**
 * Lists the queue, this is faster for older MPD servers than the search function below.
 * @param mympd_state pointer to mympd_state
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc id
 * @param offset offset for the list
 * @param limit maximum entries to print
 * @param tagcols columns to print
 * @return pointer to buffer
 */
sds mympd_api_queue_list(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, unsigned offset, unsigned limit, const struct t_fields *tagcols)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_QUEUE_SEARCH;
    //update the queue status
    mympd_client_queue_status_update(partition_state);
    //Check offset
    if (offset >= partition_state->queue_length) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "totalTime", 0, true);
        buffer = tojson_uint(buffer, "totalEntities", partition_state->queue_length, true);
        buffer = tojson_uint(buffer, "offset", offset, true);
        buffer = tojson_uint(buffer, "returnedEntities", 0, false);
        buffer = jsonrpc_end(buffer);
        return buffer;
    }

    //list the queue
    bool print_stickers = check_get_sticker(partition_state->mpd_state->feat.stickers, &tagcols->stickers);
    if (print_stickers == true) {
        stickerdb_exit_idle(mympd_state->stickerdb);
    }
    unsigned real_limit = offset + limit;
    if (mpd_send_list_queue_range_meta(partition_state->conn, offset, real_limit) == true) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        unsigned total_time = 0;
        unsigned entities_returned = 0;
        struct mpd_song *song;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = print_queue_entry(mympd_state, partition_state, buffer, tagcols, print_stickers, song);
            total_time += mpd_song_get_duration(song);
            mpd_song_free(song);
        }

        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "totalTime", total_time, true);
        buffer = tojson_uint(buffer, "totalEntities", partition_state->queue_length, true);
        buffer = tojson_uint(buffer, "offset", offset, true);
        buffer = tojson_uint(buffer, "returnedEntities", entities_returned, false);
        buffer = jsonrpc_end(buffer);
    }
    if (print_stickers == true) {
        stickerdb_enter_idle(mympd_state->stickerdb);
    }
    mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_list_queue_range_meta");
    return buffer;
}

/**
 * Searches the queue
 * @param mympd_state pointer to mympd_state
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc id
 * @param expression mpd filter expression
 * @param sort tag to sort - only relevant for feat_advqueue
 * @param sortdesc false = ascending, true = descending sort - only relevant for feat_advqueue
 * @param offset offset for the list - only relevant for feat_advqueue
 * @param limit maximum entries to print - only relevant for feat_advqueue
 * @param tagcols columns to print
 * @return pointer to buffer
 */
sds mympd_api_queue_search(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, sds expression, sds sort, bool sortdesc, unsigned offset, unsigned limit,
        const struct t_fields *tagcols)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_QUEUE_SEARCH;
    //update the queue status
    mympd_client_queue_status_update(partition_state);

    sds real_expression = sdslen(expression) == 0
        ? sdsnew("(base '')")
        : sdsdup(expression);

    if (mpd_search_queue_songs(partition_state->conn, false) == false ||
        mpd_search_add_expression(partition_state->conn, real_expression) == false ||
        add_queue_search_adv_params(partition_state, sort, sortdesc, offset, limit) == false)
    {
        mpd_search_cancel(partition_state->conn);
        FREE_SDS(real_expression);
        return jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE,
            JSONRPC_SEVERITY_ERROR, "Error creating MPD search queue command");
    }
    FREE_SDS(real_expression);
    bool print_stickers = check_get_sticker(partition_state->mpd_state->feat.stickers, &tagcols->stickers);
    if (print_stickers == true) {
        stickerdb_exit_idle(mympd_state->stickerdb);
    }
    if (mpd_search_commit(partition_state->conn)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        struct mpd_song *song;
        unsigned total_time = 0;
        const unsigned real_limit = offset + limit;
        unsigned entities_returned = 0;
        unsigned entity_count = 0;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            if (partition_state->mpd_state->feat.advqueue == true ||
                entity_count >= offset)
            {
                if (entities_returned++) {
                    buffer= sdscatlen(buffer, ",", 1);
                }
                buffer = print_queue_entry(mympd_state, partition_state, buffer, tagcols, print_stickers, song);
                total_time += mpd_song_get_duration(song);
            }
            mpd_song_free(song);
            if (partition_state->mpd_state->feat.advqueue == false) {
                entity_count++;
                if (entity_count == real_limit) {
                    break;
                }
            }
        }
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "totalTime", total_time, true);
        if (sdslen(expression) == 0) {
            buffer = tojson_uint(buffer, "totalEntities", partition_state->queue_length, true);
        }
        if (entities_returned < limit) {
            buffer = tojson_uint(buffer, "totalEntities", (offset + entities_returned), true);
        }
        else {
            buffer = tojson_int(buffer, "totalEntities", -1, true);
        }
        buffer = tojson_uint(buffer, "offset", offset, true);
        buffer = tojson_uint(buffer, "returnedEntities", entities_returned, false);
        buffer = jsonrpc_end(buffer);
    }
    if (print_stickers == true) {
        stickerdb_enter_idle(mympd_state->stickerdb);
    }
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_search_queue_songs") == false) {
        return buffer;
    }
    return buffer;
}

//private functions

/**
 * Adds sort and window parameter to queue search, if feat_advqueue is true
 * @param partition_state pointer to partition state
 * @param sort tag to sort
 * @param sortdesc sort descending?
 * @param offset offset for the list - only relevant for feat_advqueue
 * @param limit maximum entries to print - only relevant for feat_advqueue
 * @return true on success, else false
 */
static bool add_queue_search_adv_params(struct t_partition_state *partition_state, 
        sds sort, bool sortdesc, unsigned offset, unsigned limit)
{
    if (partition_state->mpd_state->feat.advqueue == false) {
        return true;
    }

    enum mpd_tag_type sort_tag = mpd_tag_name_parse(sort);
    if (sort_tag != MPD_TAG_UNKNOWN) {
        sort_tag = get_sort_tag(sort_tag, &partition_state->mpd_state->tags_mpd);
        if (mpd_search_add_sort_tag(partition_state->conn, sort_tag, sortdesc) == false) {
            return false;
        }
    }
    else if (strcmp(sort, "Last-Modified") == 0) {
        //swap order
        sortdesc = sortdesc == false
            ? true
            : false;
        if (mpd_search_add_sort_name(partition_state->conn, "Last-Modified", sortdesc) == false) {
            return false;
        }
    }
    else if (strcmp(sort, "Added") == 0) {
        //swap order
        sortdesc = sortdesc == false
            ? true
            : false;
        if (mpd_search_add_sort_name(partition_state->conn, "Added", sortdesc) == false) {
            return false;
        }
    }
    else if (strcmp(sort, "Priority") == 0) {
        if (mpd_search_add_sort_name(partition_state->conn, "prio", sortdesc) == false) {
            return false;
        }
    }
    else {
        MYMPD_LOG_WARN(partition_state->name, "Unknown sort tag: %s", sort);
    }

    unsigned real_limit = limit == 0 ? offset + MPD_PLAYLIST_LENGTH_MAX : offset + limit;
    if (mpd_search_add_window(partition_state->conn, offset, real_limit) == false) {
        return false;
    }
    return true;
}

/**
 * Prints a queue entry as an json object string
 * @param mympd_state pointer to mympd_state
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param tagcols columns to print
 * @param print_stickers Print stickers?
 * @param song pointer to mpd song struct
 * @return pointer to buffer
 */
sds print_queue_entry(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, const struct t_fields *tagcols, bool print_stickers, struct mpd_song *song)
{
    buffer = sdscatlen(buffer, "{", 1);
    buffer = tojson_uint(buffer, "id", mpd_song_get_id(song), true);
    buffer = tojson_uint(buffer, "Pos", mpd_song_get_pos(song), true);
    buffer = tojson_uint(buffer, "Priority", mpd_song_get_prio(song), true);
    const struct mpd_audio_format *audioformat = mpd_song_get_audio_format(song);
    buffer = printAudioFormat(buffer, audioformat);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = print_song_tags(buffer, partition_state->mpd_state, &tagcols->mpd_tags, song);
    const char *uri = mpd_song_get_uri(song);
    buffer = sdscatlen(buffer, ",", 1);
    if (is_streamuri(uri) == true) {
        sds webradio = mympd_api_webradio_from_uri_tojson(mympd_state, uri);
        if (sdslen(webradio) > 0) {
            buffer = sdscat(buffer, "\"webradio\":");
            buffer = sdscatsds(buffer, webradio);
            buffer = sdscatlen(buffer, ",", 1);
            buffer = tojson_char(buffer, "Type", "webradio", false);
        }
        else {
            buffer = tojson_char(buffer, "Type", "stream", false);
        }
        FREE_SDS(webradio);
    }
    else {
        buffer = tojson_char(buffer, "Type", "song", false);
    }
    if (print_stickers == true) {
        buffer = mympd_api_sticker_get_print_batch(buffer, mympd_state->stickerdb, STICKER_TYPE_SONG, uri, &tagcols->stickers);
    }
    buffer = sdscatlen(buffer, "}", 1);
    return buffer;
}
