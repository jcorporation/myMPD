/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mpd_worker/smartpls.h"

#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/smartpls.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/playlists.h"
#include "src/mpd_client/search.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mpd_client/tags.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

/**
 * Private definitions
 */
static bool mpd_worker_smartpls_per_tag(struct t_mpd_worker_state *mpd_worker_state);
static bool mpd_worker_smartpls_delete(struct t_mpd_worker_state *mpd_worker_state,
        const char *playlist);
static bool mpd_worker_smartpls_update_search(struct t_mpd_worker_state *mpd_worker_state,
        const char *playlist, const char *expression, const char *sort, bool sortdesc);
static bool mpd_worker_smartpls_update_sticker(struct t_mpd_worker_state *mpd_worker_state,
        const char *playlist, const char *sticker, const char *value, const char *op);
static bool mpd_worker_smartpls_update_newest(struct t_mpd_worker_state *mpd_worker_state,
        const char *playlist, int timerange, const char *sort, bool sortdesc);

/**
 * Public functions
 */

/**
 * Updates all smart playlists
 * @param mpd_worker_state pointer to the t_mpd_worker_state struct
 * @param force true = force update
 *              false = only update if needed
 * @return true on success, else false
 */
bool mpd_worker_smartpls_update_all(struct t_mpd_worker_state *mpd_worker_state, bool force) {
    if (mpd_worker_state->partition_state->mpd_state->feat_playlists == false) {
        MYMPD_LOG_DEBUG(NULL, "Playlists are disabled");
        return true;
    }

    mpd_worker_smartpls_per_tag(mpd_worker_state);

    time_t db_mtime = mpd_client_get_db_mtime(mpd_worker_state->partition_state);
    MYMPD_LOG_DEBUG(NULL, "Database mtime: %lld", (long long)db_mtime);

    sds dirname = sdscatfmt(sdsempty(), "%S/%s", mpd_worker_state->config->workdir, DIR_WORK_SMARTPLS);
    errno = 0;
    DIR *dir = opendir (dirname);
    if (dir == NULL) {
        MYMPD_LOG_ERROR(NULL, "Can't open smart playlist directory \"%s\"", dirname);
        MYMPD_LOG_ERRNO(NULL, errno);
        FREE_SDS(dirname);
        return false;
    }
    struct dirent *next_file;
    int updated = 0;
    int skipped = 0;
    while ((next_file = readdir(dir)) != NULL) {
        if (next_file->d_type != DT_REG) {
            continue;
        }
        time_t playlist_mtime = mpd_client_get_playlist_mtime(mpd_worker_state->partition_state, next_file->d_name);
        time_t smartpls_mtime = smartpls_get_mtime(mpd_worker_state->config->workdir, next_file->d_name);
        MYMPD_LOG_DEBUG(NULL, "Playlist %s: playlist mtime %lld, smartpls mtime %lld", next_file->d_name, (long long)playlist_mtime, (long long)smartpls_mtime);
        if (force == true ||
            db_mtime > playlist_mtime ||
            smartpls_mtime > playlist_mtime)
        {
            mpd_worker_smartpls_update(mpd_worker_state, next_file->d_name);
            updated++;
        }
        else {
            MYMPD_LOG_INFO(NULL, "Update of smart playlist %s skipped, already up to date", next_file->d_name);
            skipped++;
        }
    }
    closedir (dir);
    FREE_SDS(dirname);
    MYMPD_LOG_NOTICE(NULL, "%d smart playlists updated, %d already up-to-date", updated, skipped);
    return true;
}

/**
 * Updates a smart playlists
 * @param mpd_worker_state pointer to the t_mpd_worker_state struct
 * @param playlist smart playlist to update
 * @return true on success, else false
 */
bool mpd_worker_smartpls_update(struct t_mpd_worker_state *mpd_worker_state, const char *playlist) {
    if (mpd_worker_state->partition_state->mpd_state->feat_playlists == false) {
        MYMPD_LOG_WARN(NULL, "Playlists are disabled");
        return true;
    }

    sds filename = sdscatfmt(sdsempty(), "%S/%s/%s", mpd_worker_state->config->workdir, DIR_WORK_SMARTPLS, playlist);
    sds content = sdsempty();
    int rc_get = sds_getfile(&content, filename, SMARTPLS_SIZE_MAX, true, true);
    if (rc_get <= 0) {
        FREE_SDS(filename);
        FREE_SDS(content);
        return false;
    }

    sds smartpltype = NULL;
    bool rc = true;
    sds sds_buf1 = NULL;
    sds sds_buf2 = NULL;
    sds sds_buf3 = NULL;
    sds sort = NULL;
    bool sortdesc = false;
    int int_buf1;
    unsigned max_entries = 0;

    // first get the type
    if (json_get_string(content, "$.type", 1, 200, &smartpltype, vcb_isalnum, NULL) != true) {
        MYMPD_LOG_ERROR(NULL, "Cant read smart playlist type from \"%s\"", filename);
        FREE_SDS(filename);
        return false;
    }

    // get sort options
    if (json_get_string(content, "$.sort", 0, 100, &sort, vcb_ismpdsort, NULL) == true &&
        strcmp(sort, "shuffle") != 0)
    {
        json_get_bool(content, "$.sortdesc", &sortdesc, NULL);
    }

    // get max entries
    json_get_uint(content, "$.maxentries", 0, MPD_PLAYLIST_LENGTH_MAX, &max_entries, NULL);

    // update the playlists
    if (strcmp(smartpltype, "sticker") == 0 &&
        mpd_worker_state->mpd_state->feat_stickers == true)
    {
        if (json_get_string(content, "$.sticker", 1, NAME_LEN_MAX, &sds_buf1, vcb_isalnum, NULL) == true &&
            json_get_string(content, "$.value", 0, NAME_LEN_MAX, &sds_buf2, vcb_isname, NULL) == true &&
            json_get_string(content, "$.op", 1, 1, &sds_buf3, vcb_iscompareop, NULL) == true)
        {
            rc = mpd_worker_smartpls_update_sticker(mpd_worker_state, playlist, sds_buf1, sds_buf2, sds_buf3);
            if (rc == false) {
                MYMPD_LOG_ERROR(NULL, "Update of smart playlist \"%s\" (sticker) failed.", playlist);
            }
        }
        else {
            MYMPD_LOG_ERROR(NULL, "Can't parse smart playlist file \"%s\" (sticker)", filename);
            rc = false;
        }
    }
    else if (strcmp(smartpltype, "newest") == 0) {
        if (json_get_int(content, "$.timerange", 0, JSONRPC_INT_MAX, &int_buf1, NULL) == true) {
            rc = mpd_worker_smartpls_update_newest(mpd_worker_state, playlist, int_buf1, sort, sortdesc);
            if (rc == false) {
                MYMPD_LOG_ERROR(NULL, "Update of smart playlist \"%s\" failed (newest)", playlist);
            }
        }
        else {
            MYMPD_LOG_ERROR(NULL, "Can't parse smart playlist file \"%s\" (newest)", filename);
            rc = false;
        }
    }
    else if (strcmp(smartpltype, "search") == 0) {
        if (json_get_string(content, "$.expression", 1, 200, &sds_buf1, vcb_isname, NULL) == true) {
            rc = mpd_worker_smartpls_update_search(mpd_worker_state, playlist, sds_buf1, sort, sortdesc);
            if (rc == false) {
                MYMPD_LOG_ERROR(NULL, "Update of smart playlist \"%s\" (search) failed", playlist);
            }
        }
        else {
            MYMPD_LOG_ERROR(NULL, "Can't parse smart playlist file \"%s\" (search)", filename);
            rc = false;
        }
    }
    // sort or shuffle
    if (rc == true) {
        if (strcmp(sort, "shuffle") == 0) {
            rc = mpd_client_playlist_shuffle(mpd_worker_state->partition_state, playlist, NULL);
        }
        else if (strcmp(smartpltype, "sticker") == 0 &&
                 sdslen(sort) > 0)
        {
            // only sticker based smart playlists must be sorted after creation
            rc = mpd_client_playlist_sort(mpd_worker_state->partition_state, playlist, sort, sortdesc, NULL);
        }
    }

    // max entries
    if (rc == true &&
        max_entries > 0 &&
        mpd_worker_state->mpd_state->feat_playlist_rm_range == true)
    {
        mpd_run_playlist_delete_range(mpd_worker_state->partition_state->conn, playlist, max_entries, UINT_MAX);
    }

    FREE_SDS(smartpltype);
    FREE_SDS(sds_buf1);
    FREE_SDS(sds_buf2);
    FREE_SDS(sds_buf3);
    FREE_SDS(content);
    FREE_SDS(filename);
    FREE_SDS(sort);
    return rc;
}

/**
 * Private functions
 */

/**
 * Generates smart playlists for tag values, e.g. one smart playlist for each genre
 * @param mpd_worker_state pointer to the t_mpd_worker_state struct
 * @return true on success, else false
 */
static bool mpd_worker_smartpls_per_tag(struct t_mpd_worker_state *mpd_worker_state) {
    for (unsigned i = 0; i < mpd_worker_state->smartpls_generate_tag_types.tags_len; i++) {
        enum mpd_tag_type tag = mpd_worker_state->smartpls_generate_tag_types.tags[i];

        if (mpd_search_db_tags(mpd_worker_state->partition_state->conn, tag) == false) {
            mpd_search_cancel(mpd_worker_state->partition_state->conn);
            MYMPD_LOG_ERROR(NULL, "Error creating MPD search command");
            return false;
        }
        struct t_list tag_list;
        list_init(&tag_list);
        if (mpd_search_commit(mpd_worker_state->partition_state->conn)) {
            struct mpd_pair *pair;
            while ((pair = mpd_recv_pair_tag(mpd_worker_state->partition_state->conn, tag)) != NULL) {
                if (strlen(pair->value) > 0) {
                    list_push(&tag_list, pair->value, 0, NULL, NULL);
                }
                mpd_return_pair(mpd_worker_state->partition_state->conn, pair);
            }
        }
        mpd_response_finish(mpd_worker_state->partition_state->conn);
        if (mympd_check_error_and_recover(mpd_worker_state->partition_state, NULL, "mpd_search_commit") == false) {
            list_clear(&tag_list);
            return false;
        }
        struct t_list_node *current;
        while ((current = list_shift_first(&tag_list)) != NULL) {
            const char *tagstr = mpd_tag_name(tag);
            sds filename = sdsdup(current->key);
            sanitize_filename(filename);
            sds playlist = sdscatfmt(sdsempty(), "%S%s%s-%s", mpd_worker_state->smartpls_prefix, (sdslen(mpd_worker_state->smartpls_prefix) > 0 ? "-" : ""), tagstr, filename);
            sds plpath = sdscatfmt(sdsempty(), "%S/%s/%s", mpd_worker_state->config->workdir, DIR_WORK_SMARTPLS, playlist);
            if (testfile_read(plpath) == false) {
                //file does not exist, create it
                sds expression = sdsnewlen("(", 1);
                expression = escape_mpd_search_expression(expression, tagstr, "==", current->key);
                expression = sdscatlen(expression, ")", 1);
                bool rc = smartpls_save_search(mpd_worker_state->config->workdir, playlist, expression, mpd_worker_state->smartpls_sort, false, 0);
                FREE_SDS(expression);
                if (rc == true) {
                    MYMPD_LOG_INFO(NULL, "Created smart playlist \"%s\"", playlist);
                }
                else {
                    MYMPD_LOG_ERROR(NULL, "Creation of smart playlist \"%s\" failed", playlist);
                }
            }
            FREE_SDS(playlist);
            FREE_SDS(plpath);
            FREE_SDS(filename);
            list_node_free(current);
        }
    }
    return true;
}

/**
 * Deletes playlists if it exists
 * @param mpd_worker_state pointer to the t_mpd_worker_state struct
 * @param playlist playlist to delete
 * @return true on success, else false
 */
static bool mpd_worker_smartpls_delete(struct t_mpd_worker_state *mpd_worker_state, const char *playlist) {
    struct mpd_playlist *pl;
    bool exists = false;

    //first check if playlist exists
    if (mpd_send_list_playlists(mpd_worker_state->partition_state->conn)) {
        while ((pl = mpd_recv_playlist(mpd_worker_state->partition_state->conn)) != NULL) {
            const char *plpath = mpd_playlist_get_path(pl);
            if (strcmp(playlist, plpath) == 0) {
                exists = true;
            }
            mpd_playlist_free(pl);
            if (exists == true) {
                break;
            }
        }
    }
    mpd_response_finish(mpd_worker_state->partition_state->conn);
    if (mympd_check_error_and_recover(mpd_worker_state->partition_state, NULL, "mpd_send_list_playlists") == false) {
        return false;
    }

    //delete playlist if exists
    if (exists == true) {
        mpd_run_rm(mpd_worker_state->partition_state->conn, playlist);
        return mympd_check_error_and_recover(mpd_worker_state->partition_state, NULL, "mpd_run_rm");
    }
    return true;
}

/**
 * Updates a search based smart playlist
 * @param mpd_worker_state pointer to the t_mpd_worker_state struct
 * @param playlist playlist to delete
 * @param expression mpd search expression
 * @param sort sort by tag
 * @param sortdesc sort descending?
 * @return true on success, else false
 */
static bool mpd_worker_smartpls_update_search(struct t_mpd_worker_state *mpd_worker_state,
        const char *playlist, const char *expression, const char *sort, bool sortdesc)
{
    if (mpd_worker_smartpls_delete(mpd_worker_state, playlist) == false) {
        return false;
    }
    sds error = sdsempty();
    bool rc = mpd_client_search_add_to_plist(mpd_worker_state->partition_state,
        expression, playlist, UINT_MAX, sort, sortdesc, &error);
    if (rc == true) {
        MYMPD_LOG_INFO(NULL, "Updated smart playlist \"%s\"", playlist);
    }
    else {
        MYMPD_LOG_ERROR(NULL, "Updating smart playlist \"%s\" failed: %s", playlist, error);
    }
    FREE_SDS(error);
    return rc;
}

/**
 * Updates a sticker based smart playlist (numeric stickers only)
 * @param mpd_worker_state pointer to the t_mpd_worker_state struct
 * @param playlist playlist to delete
 * @param sticker sticker evaluate
 * @param value sticker value
 * @param op compare operator
 * @return true on success, else false
 */
static bool mpd_worker_smartpls_update_sticker(struct t_mpd_worker_state *mpd_worker_state,
        const char *playlist, const char *sticker, const char *value, const char *op)
{
    rax *add_list = stickerdb_find_stickers_by_name_value(mpd_worker_state->stickerdb, sticker, op, value);
    if (add_list == NULL) {
        MYMPD_LOG_ERROR(NULL, "Could not fetch stickers for \"%s\"", sticker);
        return false;
    }
    
    int i = 0;
    sds uri = sdsempty();
    if (mpd_command_list_begin(mpd_worker_state->partition_state->conn, false)) {
        raxIterator iter;
        raxStart(&iter, add_list);
        raxSeek(&iter, "^", NULL, 0);
        while (raxNext(&iter)) {
            uri = sds_replacelen(uri, (char *)iter.key, iter.key_len);
            if (mpd_send_playlist_add(mpd_worker_state->partition_state->conn, playlist, uri) == false) {
                mympd_set_mpd_failure(mpd_worker_state->partition_state, "Error adding command to command list mpd_send_playlist_add");
                break;
            }
            i++;
        }
        mpd_command_list_end(mpd_worker_state->partition_state->conn);
        raxStop(&iter);
    }
    FREE_SDS(uri);
    stickerdb_free_find_result(add_list);
    mpd_response_finish(mpd_worker_state->partition_state->conn);
    if (mympd_check_error_and_recover(mpd_worker_state->partition_state, NULL, "mpd_send_playlist_add") == true) {
        MYMPD_LOG_INFO(NULL, "Updated smart playlist \"%s\" with %d songs", playlist, i);
        return true;
    }
    return false;
}

/**
 * Updates a newest song smart playlist
 * @param mpd_worker_state pointer to the t_mpd_worker_state struct
 * @param playlist playlist to delete
 * @param timerange timerange in seconds since last database update
 * @param sort sort by tag
 * @param sortdesc sort descending?
 * @return true on success, else false
 */
static bool mpd_worker_smartpls_update_newest(struct t_mpd_worker_state *mpd_worker_state,
        const char *playlist, int timerange, const char *sort, bool sortdesc)
{
    unsigned long value_max = 0;
    struct mpd_stats *stats = mpd_run_stats(mpd_worker_state->partition_state->conn);
    if (stats != NULL) {
        value_max = mpd_stats_get_db_update_time(stats);
        mpd_stats_free(stats);
    }
    mpd_response_finish(mpd_worker_state->partition_state->conn);
    if (mympd_check_error_and_recover(mpd_worker_state->partition_state, NULL, "mpd_run_stats") == false) {
        return false;
    }

    if (mpd_worker_smartpls_delete(mpd_worker_state, playlist) == false) {
        return false;
    }

    //prevent overflow
    if (timerange < 0 ||
        (unsigned long)timerange > value_max) {
        return false;
    }
    value_max = value_max - (unsigned long)timerange;

    sds error = sdsempty();
    sds expression = sdscatfmt(sdsempty(), "(modified-since '%U')", value_max);
    bool rc = mpd_client_search_add_to_plist(mpd_worker_state->partition_state, expression,
        playlist, UINT_MAX, sort, sortdesc, &error);
    FREE_SDS(expression);
    
    if (rc == true) {
        MYMPD_LOG_INFO(NULL, "Updated smart playlist \"%s\"", playlist);
    }
    else {
        MYMPD_LOG_ERROR(NULL, "Updating smart playlist \"%s\" failed: %s", playlist, error);
    }
    FREE_SDS(error);
    return rc;
}
