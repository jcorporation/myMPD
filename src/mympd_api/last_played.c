/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/last_played.h"

#include "dist/sds/sds.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/search_local.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/sticker.h"

#include <errno.h>
#include <string.h>

/**
 * Private definitions
 */

static sds get_last_played_obj(struct t_partition_state *partition_state, sds buffer, long entity_count,
        long long last_played, const char *uri, struct t_list *expr_list, const struct t_tags *tagcols);

/**
 * Public functions
 */

/**
 * Saves the last played list from memory to disc and empties the list in memory
 * @param partition_state pointer to partition state
 * @return true on success, else false
 */
bool mympd_api_last_played_file_save(struct t_partition_state *partition_state) {
    MYMPD_LOG_INFO(partition_state->name, "Saving last_played list to disc");
    sds tmp_file = sdscatfmt(sdsempty(), "%S/%S/%s.XXXXXX",
        partition_state->mympd_state->config->workdir, partition_state->state_dir, FILENAME_LAST_PLAYED);
    FILE *fp = open_tmp_file(tmp_file);
    if (fp == NULL) {
        FREE_SDS(tmp_file);
        return false;
    }

    int count = 0;
    //save last_played from mem to disc
    struct t_list_node *current;
    bool write_rc = true;
    sds line = sdsempty();
    while ((current = list_shift_first(&partition_state->last_played)) != NULL) {
        line = sdscatlen(line, "{", 1);
        line = tojson_llong(line, "LastPlayed", current->value_i, true);
        line = tojson_char(line, "uri", current->key, false);
        line = sdscatlen(line, "}\n", 2);

        if (fputs(line, fp) == EOF) {
            MYMPD_LOG_ERROR(partition_state->name, "Could not write last played songs to disc");
            write_rc = false;
            list_node_free(current);
            break;
        }
        count++;
        list_node_free(current);
        sdsclear(line);
    }
    //append current last_played file to tmp file
    sds filepath = sdscatfmt(sdsempty(), "%S/%S/%s",
        partition_state->mympd_state->config->workdir, partition_state->state_dir, FILENAME_LAST_PLAYED);
    if (write_rc == true) {
        errno = 0;
        FILE *fi = fopen(filepath, OPEN_FLAGS_READ);
        if (fi != NULL) {
            while (sds_getline(&line, fi, LINE_LENGTH_MAX) >= 0 &&
                count < partition_state->mympd_state->last_played_count)
            {
                line = sdscatlen(line, "\n", 1);
                if (fputs(line, fp) == EOF) {
                    MYMPD_LOG_ERROR(partition_state->name, "Could not write last played songs to disc");
                    write_rc = false;
                    break;
                }
                count++;
            }
            (void) fclose(fi);
        }
        else {
            //ignore error
            MYMPD_LOG_DEBUG(partition_state->name, "Can not open file \"%s\"", filepath);
            if (errno != ENOENT) {
                MYMPD_LOG_ERRNO(partition_state->name, errno);
            }
        }
    }
    FREE_SDS(line);
    FREE_SDS(filepath);
    bool rc = rename_tmp_file(fp, tmp_file, write_rc);
    FREE_SDS(tmp_file);
    return rc;
}

/**
 * Adds a song from queue with song_id to the last played list in memory
 * @param partition_state pointer to partition state
 * @param song_id the song id to add
 * @return true on success, else false
 */
bool mympd_api_last_played_add_song(struct t_partition_state *partition_state, int song_id) {
    if (song_id == -1 ||                                    // no current song
        partition_state->mympd_state->last_played_count == 0) // last played is disabled
    {
        return true;
    }

    // get current playing song and add it
    struct mpd_song *song = mpd_run_get_queue_song_id(partition_state->conn, (unsigned)song_id);
    if (song != NULL) {
        const char *uri = mpd_song_get_uri(song);
        if (is_streamuri(uri) == true) {
            //Don't add streams to last played list
            mpd_song_free(song);
            return true;
        }
        list_insert(&partition_state->last_played, uri, (long long)time(NULL), NULL, NULL);
        mpd_song_free(song);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover(partition_state, NULL, "mpd_run_get_queue_song_id") == false) {
        return false;
    }

    //write last_played list to disc
    if (partition_state->last_played.length >= LAST_PLAYED_MEM_MAX ||
        partition_state->last_played.length > partition_state->mympd_state->last_played_count)
    {
        mympd_api_last_played_file_save(partition_state);
    }
    //notify clients
    send_jsonrpc_event(JSONRPC_EVENT_UPDATE_LAST_PLAYED, partition_state->name);
    return true;
}

/**
 * Prints a jsonrpc response with the last played songs (memory and disc)
 * @param partition_state pointer to partition state
 * @param buffer already allocated sds string to append the response
 * @param request_id jsonrpc request id
 * @param offset offset
 * @param limit max number of entries to return
 * @param expression mpd search expression
 * @param tagcols columns to print
 * @return pointer to buffer
 */
sds mympd_api_last_played_list(struct t_partition_state *partition_state, sds buffer,
        long request_id, long offset, long limit, sds expression, const struct t_tags *tagcols)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_LAST_PLAYED_LIST;
    long entity_count = 0;
    long entities_returned = 0;
    long entities_found = 0;

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    sds obj = sdsempty();

    long real_limit = offset + limit;
    struct t_list *expr_list = parse_search_expression_to_list(expression);
    // first get entries from memory
    if (partition_state->mpd_state->feat_stickers == true &&
        tagcols->stickers_len > 0)
    {
        stickerdb_exit_idle(partition_state->mympd_state->stickerdb);
    }
    if (offset < partition_state->last_played.length) {
        struct t_list_node *current = partition_state->last_played.head;
        while (current != NULL) {
            obj = get_last_played_obj(partition_state, obj, entity_count, current->value_i,
                current->key, expr_list, tagcols);
            if (sdslen(obj) > 0) {
                if (entities_found >= offset) {
                    if (entities_returned++) {
                        buffer = sdscatlen(buffer, ",", 1);
                    }
                    buffer = sdscatsds(buffer, obj);
                }
                sdsclear(obj);
                entities_found++;
                if (entities_returned == real_limit) {
                    break;
                }
            }
            entity_count++;
            current = current->next;
        }
    }
    else {
        entity_count = partition_state->last_played.length;
    }

    // get entries from disk
    if (entities_returned < real_limit) {
        sds lp_file = sdscatfmt(sdsempty(), "%S/%S/%s",
            partition_state->mympd_state->config->workdir, partition_state->state_dir, FILENAME_LAST_PLAYED);
        errno = 0;
        FILE *fp = fopen(lp_file, OPEN_FLAGS_READ);
        if (fp != NULL) {
            sds line = sdsempty();
            while (sds_getline(&line, fp, LINE_LENGTH_MAX) >= 0) {
                sds uri = NULL;
                long long last_played = 0;
                if (json_get_string_max(line, "$.uri", &uri, vcb_isfilepath, NULL) == true &&
                    json_get_llong_max(line, "$.LastPlayed", &last_played, NULL) == true)
                {
                    obj = get_last_played_obj(partition_state, obj, entity_count, last_played, uri, expr_list, tagcols);
                    FREE_SDS(uri);
                    if (sdslen(obj) > 0) {
                        if (entities_found >= offset) {
                            if (entities_returned++) {
                                buffer = sdscatlen(buffer, ",", 1);
                            }
                            buffer = sdscatsds(buffer, obj);
                        }
                        sdsclear(obj);
                        entities_found++;
                        if (entities_returned == real_limit) {
                            break;
                        }
                    }
                }
                else {
                    MYMPD_LOG_ERROR(partition_state->name, "Reading last_played line failed");
                    MYMPD_LOG_DEBUG(partition_state->name, "Erroneous line: %s", line);
                    FREE_SDS(uri);
                }
                entity_count++;
            }
            (void) fclose(fp);
            FREE_SDS(line);
        }
        else {
            MYMPD_LOG_DEBUG(partition_state->name, "Can not open file \"%s\"", lp_file);
            if (errno != ENOENT) {
                //ignore missing last_played file
                MYMPD_LOG_ERRNO(partition_state->name, errno);
            }
        }
        FREE_SDS(lp_file);
    }
    FREE_SDS(obj);
    if (partition_state->mpd_state->feat_stickers == true &&
        tagcols->stickers_len > 0)
    {
        stickerdb_enter_idle(partition_state->mympd_state->stickerdb);
    }
    free_search_expression_list(expr_list);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "totalEntities", -1, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end(buffer);

    return buffer;
}

/**
 * Private functions
 */

/**
 * Gets the song and searches for searchstr and prints it as json object
 * @param partition_state pointer to partition state
 * @param buffer already allocated buffer to append the result
 * @param entity_count position in the list
 * @param last_played songs last played time as unix timestamp
 * @param uri uri of the song
 * @param expr_list list of search expressions
 * @param tagcols columns to print
 * @return pointer to buffer
 */
static sds get_last_played_obj(struct t_partition_state *partition_state, sds buffer, long entity_count,
        long long last_played, const char *uri, struct t_list *expr_list, const struct t_tags *tagcols)
{
    if (mpd_send_list_meta(partition_state->conn, uri)) {
        struct mpd_song *song;
        if ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            if (search_song_expression(song, expr_list, tagcols) == true) {
                buffer = sdscat(buffer, "{\"Type\": \"song\",");
                buffer = tojson_long(buffer, "Pos", entity_count, true);
                buffer = tojson_llong(buffer, "LastPlayed", last_played, true);
                buffer = print_song_tags(buffer, partition_state->mpd_state->feat_tags, tagcols, song, &partition_state->mympd_state->config->albums);
                if (partition_state->mpd_state->feat_stickers == true &&
                    tagcols->stickers_len > 0)
                {
                    buffer = mympd_api_sticker_get_print_batch(buffer, partition_state->mympd_state->stickerdb, mpd_song_get_uri(song), tagcols);
                }
                buffer = sdscatlen(buffer, "}", 1);
            }
            mpd_song_free(song);
        }
    }
    mpd_response_finish(partition_state->conn);
    mympd_check_error_and_recover(partition_state, NULL, "mpd_send_list_meta");
    return buffer;
}
