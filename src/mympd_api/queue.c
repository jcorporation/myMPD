/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "dist/sds/sds.h"
#include "src/lib/mympd_state.h"
#include "src/mympd_api/queue.h"

#include "src/lib/album_cache.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/queue.h"
#include "src/mpd_client/search.h"
#include "src/mpd_client/shortcuts.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/sticker.h"
#include "src/mympd_api/webradios.h"

#include <limits.h>
#include <string.h>

/**
 * Private definitions
 */
 static bool add_queue_search_adv_params(struct t_partition_state *partition_state,
        sds sort, bool sortdesc, unsigned offset, unsigned limit);
sds print_queue_entry(struct t_partition_state *partition_state, sds buffer,
        const struct t_tags *tagcols, struct mpd_song *song);

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
    if (partition_state->mpd_state->feat_advqueue == true) {
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
        mpd_client_command_list_end_check(partition_state);
    }
    mpd_response_finish(partition_state->conn);
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
        mpd_client_command_list_end_check(partition_state);
    }
    mpd_response_finish(partition_state->conn);
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
    mpd_response_finish(partition_state->conn);
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
        mpd_response_finish(partition_state->conn);
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
        partition_state->mpd_state->feat_whence == false)
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
        mpd_client_command_list_end_check(partition_state);
    }
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_move_id_whence");
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
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat_whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    if (uris->length == 0) {
        *error = sdscat(*error, "No uris provided");
        return false;
    }
    if (mpd_command_list_begin(partition_state->conn, false)) {
        struct t_list_node *current;
        while ((current = list_shift_first(uris)) != NULL) {
            bool rc = to == UINT_MAX
                ? mpd_send_add(partition_state->conn, current->key)
                : mpd_send_add_whence(partition_state->conn, current->key, to, whence);
            list_node_free(current);
            if (rc == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_add");
                break;
            }
            if (to != UINT_MAX) {
                to++;
            }
        }
        mpd_client_command_list_end_check(partition_state);
    }
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_add_whence");
}

/**
 * Appends uris to the queue
 * @param partition_state pointer to partition state
 * @param uris uris to append
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append(struct t_partition_state *partition_state, struct t_list *uris, sds *error) {
    return mympd_api_queue_insert(partition_state, uris, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with uris
 * @param partition_state pointer to partition state
 * @param uris uris to add
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace(struct t_partition_state *partition_state, struct t_list *uris, sds *error) {
    return mpd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append(partition_state, uris, error);
}

/**
 * Inserts search results into the queue
 * @param partition_state pointer to partition state
 * @param expression mpd search expression
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_insert_search(struct t_partition_state *partition_state, sds expression, unsigned to, unsigned whence, sds *error) {
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat_whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    const char *sort = NULL;
    bool sortdesc = false;
    return mpd_client_search_add_to_queue(partition_state, expression, to, whence, sort, sortdesc, error);
}

/**
 * Appends the search results to the queue
 * @param partition_state pointer to partition state
 * @param expression mpd search expression
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append_search(struct t_partition_state *partition_state, sds expression, sds *error) {
    return mympd_api_queue_insert_search(partition_state, expression, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with the search result
 * @param partition_state pointer to partition state
 * @param expression mpd search expression
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace_search(struct t_partition_state *partition_state, sds expression, sds *error) {
    return mpd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_search(partition_state, expression, error);
}

/**
 * Inserts albums into the queue
 * @param partition_state pointer to partition state
 * @param albumids album ids to insert
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_insert_albums(struct t_partition_state *partition_state, struct t_list *albumids, unsigned to, unsigned whence, sds *error) {
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat_whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    if (albumids->length == 0) {
        *error = sdscat(*error, "No album ids provided");
        return false;
    }
    struct t_list_node *current = albumids->head;
    bool rc = true;
    while (current != NULL) {
        struct mpd_song *mpd_album = album_cache_get_album(&partition_state->mpd_state->album_cache, current->key);
        if (mpd_album == NULL) {
            *error = sdscat(*error, "Album not found");
            return false;
        }
        sds expression = get_search_expression_album(partition_state->mpd_state->tag_albumartist, mpd_album);
        const char *sort = NULL;
        bool sortdesc = false;
        rc = mpd_client_search_add_to_queue(partition_state, expression, to, whence, sort, sortdesc, error);
        FREE_SDS(expression);
        if (rc == false) {
            break;
        }
        current = current->next;
    }
    return rc;
}

/**
 * Appends albums to the queue
 * @param partition_state pointer to partition state
 * @param albumids album ids to append
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append_albums(struct t_partition_state *partition_state, struct t_list *albumids, sds *error) {
    return mympd_api_queue_insert_albums(partition_state, albumids, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with albums
 * @param partition_state pointer to partition state
 * @param albumids album ids to insert
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace_albums(struct t_partition_state *partition_state, struct t_list *albumids, sds *error) {
    return mpd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_albums(partition_state, albumids, error);
}

/**
 * Inserts one disc of an album into the queue
 * @param partition_state pointer to partition state
 * @param albumid album id to insert
 * @param disc disc to insert
 * @param to position to insert
 * @param whence how to interpret the to parameter
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_insert_album_disc(struct t_partition_state *partition_state, sds albumid, sds disc, unsigned to, unsigned whence, sds *error) {
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat_whence == false)
    {
        *error = sdscat(*error, "Method not supported");
        return false;
    }
    struct mpd_song *mpd_album = album_cache_get_album(&partition_state->mpd_state->album_cache, albumid);
    if (mpd_album == NULL) {
        *error = sdscat(*error, "Album not found");
        return false;
    }
    sds expression = get_search_expression_album_disc(partition_state->mpd_state->tag_albumartist, mpd_album, disc);
    const char *sort = NULL;
    bool sortdesc = false;
    bool rc = mpd_client_search_add_to_queue(partition_state, expression, to, whence, sort, sortdesc, error);
    FREE_SDS(expression);
    return rc;
}

/**
 * Appends one disc of an album to the queue
 * @param partition_state pointer to partition state
 * @param albumid album id to append
 * @param disc disc to append
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append_album_disc(struct t_partition_state *partition_state, sds albumid, sds disc, sds *error) {
    return mympd_api_queue_insert_album_disc(partition_state, albumid, disc, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with one disc of an album
 * @param partition_state pointer to partition state
 * @param albumid album id to insert
 * @param disc disc to insert
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace_album_disc(struct t_partition_state *partition_state, sds albumid, sds disc, sds *error) {
    return mpd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_album_disc(partition_state, albumid, disc, error);
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
bool mympd_api_queue_insert_plist(struct t_partition_state *partition_state, struct t_list *plists, unsigned to, unsigned whence, sds *error) {
    if (whence != MPD_POSITION_ABSOLUTE &&
        partition_state->mpd_state->feat_whence == false)
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
            current->key = resolv_mympd_uri(current->key, partition_state->mpd_state->mpd_host, partition_state->mympd_state->config);
            bool rc = to == UINT_MAX
                ? mpd_send_load(partition_state->conn, current->key)
                : mpd_send_load_range_to(partition_state->conn, current->key, 0, UINT_MAX, to, whence);
            if (rc == false) {
                mympd_set_mpd_failure(partition_state, "Error adding command to command list mpd_send_load_range_to");
                break;
            }
            current = current->next;
        }
        mpd_client_command_list_end_check(partition_state);
    }
    mpd_response_finish(partition_state->conn);
    return mympd_check_error_and_recover(partition_state, error, "mpd_send_load_range_to");
}

/**
 * Appends playlists to the queue
 * @param partition_state pointer to partition state
 * @param plists playlists to append
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_append_plist(struct t_partition_state *partition_state, struct t_list *plists, sds *error) {
    return mympd_api_queue_insert_plist(partition_state, plists, UINT_MAX, MPD_POSITION_ABSOLUTE, error);
}

/**
 * Replaces the queue with playlists
 * @param partition_state pointer to partition state
 * @param plists playlists to add
 * @param error pointer to an already allocated sds string for the error message
 * @return true on success, else false
 */
bool mympd_api_queue_replace_plist(struct t_partition_state *partition_state, struct t_list *plists, sds *error) {
    return mpd_client_queue_clear(partition_state, error) &&
        mympd_api_queue_append_plist(partition_state, plists, error);
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
sds mympd_api_queue_crop(struct t_partition_state *partition_state, sds buffer, enum mympd_cmd_ids cmd_id, long request_id, bool or_clear) {
    struct mpd_status *status = mpd_run_status(partition_state->conn);
    unsigned length = 0;
    int playing_song_pos = 0;
    if (status != NULL) {
        length = mpd_status_get_queue_length(status) - 1;
        playing_song_pos = mpd_status_get_song_pos(status);
        mpd_status_free(status);
    }
    mpd_response_finish(partition_state->conn);
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
            mpd_client_command_list_end_check(partition_state);
        }
        mpd_response_finish(partition_state->conn);
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
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc id
 * @param offset offset for the list
 * @param limit maximum entries to print
 * @param tagcols columns to print
 * @return pointer to buffer
 */
sds mympd_api_queue_list(struct t_partition_state *partition_state, sds buffer, long request_id,
        unsigned offset, unsigned limit, const struct t_tags *tagcols)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_QUEUE_SEARCH;
    //update the queue status
    mpd_client_queue_status(partition_state, NULL);
    //Check offset
    if (offset >= partition_state->queue_length) {
        offset = 0;
    }
    //list the queue
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
            buffer = print_queue_entry(partition_state, buffer, tagcols, song);
            total_time += mpd_song_get_duration(song);
            mpd_song_free(song);
        }

        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "totalTime", total_time, true);
        buffer = tojson_llong(buffer, "totalEntities", partition_state->queue_length, true);
        buffer = tojson_uint(buffer, "offset", offset, true);
        buffer = tojson_uint(buffer, "returnedEntities", entities_returned, false);
        buffer = jsonrpc_end(buffer);
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_list_queue_range_meta");
    return buffer;
}

/**
 * Searches the queue
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
sds mympd_api_queue_search(struct t_partition_state *partition_state, sds buffer, long request_id,
        sds expression, sds sort, bool sortdesc, unsigned offset, unsigned limit,
        const struct t_tags *tagcols)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_QUEUE_SEARCH;
    //update the queue status
    mpd_client_queue_status(partition_state, NULL);

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
    if (mpd_search_commit(partition_state->conn)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");
        struct mpd_song *song;
        unsigned total_time = 0;
        const unsigned real_limit = offset + limit;
        unsigned entities_returned = 0;
        unsigned entity_count = 0;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            if (partition_state->mpd_state->feat_advqueue == true ||
                entity_count >= offset)
            {
                if (entities_returned++) {
                    buffer= sdscatlen(buffer, ",", 1);
                }
                buffer = print_queue_entry(partition_state, buffer, tagcols, song);
                total_time += mpd_song_get_duration(song);
            }
            mpd_song_free(song);
            if (partition_state->mpd_state->feat_advqueue == false) {
                entity_count++;
                if (entity_count == real_limit) {
                    break;
                }
            }
        }
        buffer = sdscatlen(buffer, "],", 2);
        buffer = tojson_uint(buffer, "totalTime", total_time, true);
        if (sdslen(expression) == 0) {
            buffer = tojson_llong(buffer, "totalEntities", partition_state->queue_length, true);
        }
        if (entities_returned < limit) {
            buffer = tojson_uint(buffer, "totalEntities", (offset + entities_returned), true);
        }
        else {
            buffer = tojson_long(buffer, "totalEntities", -1, true);
        }
        buffer = tojson_uint(buffer, "offset", offset, true);
        buffer = tojson_uint(buffer, "returnedEntities", entities_returned, false);
        buffer = jsonrpc_end(buffer);
    }
    mpd_response_finish(partition_state->conn);
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
    if (partition_state->mpd_state->feat_advqueue == false) {
        return true;
    }

    enum mpd_tag_type sort_tag = mpd_tag_name_parse(sort);
    if (sort_tag != MPD_TAG_UNKNOWN) {
        sort_tag = get_sort_tag(sort_tag, &partition_state->mpd_state->tags_mpd);
        if (mpd_search_add_sort_tag(partition_state->conn, sort_tag, sortdesc) == false) {
            return false;
        }
    }
    else if (strcmp(sort, "LastModified") == 0) {
        //swap order
        sortdesc = sortdesc == false ? true : false;
        if (mpd_search_add_sort_name(partition_state->conn, "Last-Modified", sortdesc) == false) {
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
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param tagcols columns to print
 * @param song pointer to mpd song struct
 * @return pointer to buffer
 */
sds print_queue_entry(struct t_partition_state *partition_state, sds buffer,
        const struct t_tags *tagcols, struct mpd_song *song)
{
    buffer = sdscatlen(buffer, "{", 1);
    buffer = tojson_uint(buffer, "id", mpd_song_get_id(song), true);
    buffer = tojson_uint(buffer, "Pos", mpd_song_get_pos(song), true);
    buffer = tojson_uint(buffer, "Priority", mpd_song_get_prio(song), true);
    const struct mpd_audio_format *audioformat = mpd_song_get_audio_format(song);
    buffer = printAudioFormat(buffer, audioformat);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = print_song_tags(buffer, partition_state->mpd_state->feat_tags, tagcols, song);
    const char *uri = mpd_song_get_uri(song);
    buffer = sdscatlen(buffer, ",", 1);
    if (is_streamuri(uri) == true) {
        sds webradio = get_webradio_from_uri(partition_state->mympd_state->config->workdir, uri);
        if (sdslen(webradio) > 0) {
            buffer = sdscat(buffer, "\"webradio\":{");
            buffer = sdscatsds(buffer, webradio);
            buffer = sdscatlen(buffer, "},", 2);
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
    if (partition_state->mpd_state->feat_stickers == true &&
        tagcols->stickers_len > 0)
    {
        buffer = sdscatlen(buffer, ",", 1);
        buffer = mympd_api_sticker_get_print(buffer, partition_state->mympd_state->stickerdb, uri, tagcols);
    }
    buffer = sdscatlen(buffer, "}", 1);
    return buffer;
}
