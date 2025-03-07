/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Smart playlist generation
 */

#include "compile_time.h"
#include "src/mympd_worker/smartpls.h"

#include "src/lib/datetime.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/sds_extras.h"
#include "src/lib/smartpls.h"
#include "src/lib/utility.h"
#include "src/lib/validate.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/playlists.h"
#include "src/mympd_client/search.h"
#include "src/mympd_client/shortcuts.h"
#include "src/mympd_client/stickerdb.h"
#include "src/mympd_client/tags.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

/**
 * Private definitions
 */
static bool mympd_worker_smartpls_per_tag(struct t_mympd_worker_state *mympd_worker_state);
static bool mympd_worker_smartpls_delete(struct t_mympd_worker_state *mympd_worker_state,
        const char *playlist);
static bool mympd_worker_smartpls_update_search(struct t_mympd_worker_state *mympd_worker_state,
        const char *playlist, const char *expression, const char *sort, bool sortdesc, unsigned max_entries);
static bool mympd_worker_smartpls_update_sticker(struct t_mympd_worker_state *mympd_worker_state,
        const char *playlist, const char *sticker, const char *value, const char *op,
        const char *sort, bool sort_desc, unsigned max_entries);
static bool mympd_worker_smartpls_update_newest(struct t_mympd_worker_state *mympd_worker_state,
        const char *playlist, unsigned timerange, const char *sort, bool sortdesc, unsigned max_entries);

/**
 * Public functions
 */

/**
 * Updates all smart playlists
 * @param mympd_worker_state pointer to the t_mympd_worker_state struct
 * @param force true = force update
 *              false = only update if needed
 * @return true on success, else false
 */
bool mympd_worker_smartpls_update_all(struct t_mympd_worker_state *mympd_worker_state, bool force) {
    if (mympd_worker_state->partition_state->mpd_state->feat.playlists == false) {
        MYMPD_LOG_DEBUG(NULL, "Playlists are disabled");
        return true;
    }

    mympd_worker_smartpls_per_tag(mympd_worker_state);

    time_t db_mtime = mympd_client_get_db_mtime(mympd_worker_state->partition_state);
    #ifdef MYMPD_DEBUG
        char fmt_db_mtime[32];
        readable_time(fmt_db_mtime, db_mtime);
        MYMPD_LOG_DEBUG(NULL, "Database mtime: %s", fmt_db_mtime);
    #endif

    sds dirname = sdscatfmt(sdsempty(), "%S/%s", mympd_worker_state->config->workdir, DIR_WORK_SMARTPLS);
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
        time_t playlist_mtime = mympd_client_get_playlist_mtime(mympd_worker_state->partition_state, next_file->d_name);
        time_t smartpls_mtime = smartpls_get_mtime(mympd_worker_state->config->workdir, next_file->d_name);
        #ifdef MYMPD_DEBUG
            char fmt_time_playlist[32];
            readable_time(fmt_time_playlist, playlist_mtime);
            char fmt_time_smartpls[32];
            readable_time(fmt_time_smartpls, smartpls_mtime);
            MYMPD_LOG_DEBUG(NULL, "Playlist %s: playlist mtime %s, smartpls mtime %s",
                next_file->d_name, fmt_time_playlist, fmt_time_smartpls);
        #endif
        if (force == true ||
            db_mtime > playlist_mtime ||
            smartpls_mtime > playlist_mtime)
        {
            if (mympd_worker_smartpls_update(mympd_worker_state, next_file->d_name) == true) {
                updated++;
            }
            else {
                MYMPD_LOG_WARN(NULL, "Removing invalid smart playlist %s", next_file->d_name);
                sds filename = sdscatfmt(sdsempty(), "%S/%s/%s", mympd_worker_state->config->workdir, DIR_WORK_SMARTPLS, next_file->d_name);
                rm_file(filename);
                FREE_SDS(filename);
            }
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
 * Updates a smart playlist
 * @param mympd_worker_state pointer to the t_mympd_worker_state struct
 * @param playlist smart playlist to update
 * @return true on success, else false
 */
bool mympd_worker_smartpls_update(struct t_mympd_worker_state *mympd_worker_state, const char *playlist) {
    if (mympd_worker_state->partition_state->mpd_state->feat.playlists == false) {
        MYMPD_LOG_WARN(NULL, "Playlists are disabled");
        return true;
    }

    sds filename = sdscatfmt(sdsempty(), "%S/%s/%s", mympd_worker_state->config->workdir, DIR_WORK_SMARTPLS, playlist);
    int nread = 0;
    sds content = sds_getfile(sdsempty(), filename, SMARTPLS_SIZE_MAX, true, true, &nread);
    if (nread <= 0) {
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
    unsigned uint_buf1;
    unsigned max_entries = 0;

    // first get the type
    if (json_get_string(content, "$.type", 1, 200, &smartpltype, vcb_isalnum, NULL) != true) {
        MYMPD_LOG_ERROR(NULL, "Cant read smart playlist type from \"%s\"", filename);
        FREE_SDS(filename);
        FREE_SDS(content);
        return false;
    }

    // get sort options
    if (json_get_string(content, "$.sort", 0, SORT_LEN_MAX, &sort, vcb_ismpd_sticker_sort, NULL) == true &&
        strcmp(sort, "shuffle") != 0)
    {
        json_get_bool(content, "$.sortdesc", &sortdesc, NULL);
    }
    if (sort == NULL) {
        sort = sdsempty();
    }

    // get max entries
    json_get_uint(content, "$.maxentries", 0, MPD_PLAYLIST_LENGTH_MAX, &max_entries, NULL);

    // delete the playlist
    mympd_worker_smartpls_delete(mympd_worker_state, playlist);

    // recreate the playlist
    if (strcmp(smartpltype, "sticker") == 0 &&
        mympd_worker_state->mpd_state->feat.stickers == true)
    {
        if (json_get_string(content, "$.sticker", 1, NAME_LEN_MAX, &sds_buf1, vcb_isname, NULL) == true &&
            json_get_string(content, "$.value", 0, NAME_LEN_MAX, &sds_buf2, vcb_isname, NULL) == true &&
            json_get_string(content, "$.op", 1, STICKER_OP_LEN_MAX, &sds_buf3, vcb_isstickerop, NULL) == true)
        {
            rc = mympd_worker_smartpls_update_sticker(mympd_worker_state, playlist, sds_buf1, sds_buf2, sds_buf3, sort, sortdesc, max_entries);
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
        if (json_get_uint(content, "$.timerange", 0, JSONRPC_INT_MAX, &uint_buf1, NULL) == true) {
            rc = mympd_worker_smartpls_update_newest(mympd_worker_state, playlist, uint_buf1, sort, sortdesc, max_entries);
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
            rc = mympd_worker_smartpls_update_search(mympd_worker_state, playlist, sds_buf1, sort, sortdesc, max_entries);
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
    if (rc == true &&
        sdslen(sort) > 0)
    {
        if (strcmp(sort, "shuffle") == 0) {
            rc = mympd_client_playlist_shuffle(mympd_worker_state->partition_state, playlist, NULL);
        }
        else if (strcmp(smartpltype, "sticker") == 0 &&
                 sticker_sort_parse(sort) == MPD_STICKER_SORT_UNKOWN)
        {
            // resort sticker based smart playlists by sort tag
            rc = mympd_client_playlist_sort(mympd_worker_state->partition_state, playlist, sort, sortdesc, NULL);
        }
    }

    // enforce max entries
    if (rc == true &&
        max_entries > 0 &&
        mympd_worker_state->mpd_state->feat.playlist_rm_range == true)
    {
        mympd_client_playlist_crop(mympd_worker_state->partition_state, playlist, max_entries);
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
 * @param mympd_worker_state pointer to the t_mympd_worker_state struct
 * @return true on success, else false
 */
static bool mympd_worker_smartpls_per_tag(struct t_mympd_worker_state *mympd_worker_state) {
    for (unsigned i = 0; i < mympd_worker_state->smartpls_generate_tag_types.len; i++) {
        enum mpd_tag_type tag = mympd_worker_state->smartpls_generate_tag_types.tags[i];

        if (mpd_search_db_tags(mympd_worker_state->partition_state->conn, tag) == false) {
            mpd_search_cancel(mympd_worker_state->partition_state->conn);
            MYMPD_LOG_ERROR(NULL, "Error creating MPD search command");
            return false;
        }
        struct t_list tag_list;
        list_init(&tag_list);
        if (mpd_search_commit(mympd_worker_state->partition_state->conn)) {
            struct mpd_pair *pair;
            while ((pair = mpd_recv_pair_tag(mympd_worker_state->partition_state->conn, tag)) != NULL) {
                if (strlen(pair->value) > 0) {
                    list_push(&tag_list, pair->value, 0, NULL, NULL);
                }
                mpd_return_pair(mympd_worker_state->partition_state->conn, pair);
            }
        }
        mpd_response_finish(mympd_worker_state->partition_state->conn);
        if (mympd_check_error_and_recover(mympd_worker_state->partition_state, NULL, "mpd_search_commit") == false) {
            list_clear(&tag_list);
            return false;
        }
        struct t_list_node *current;
        while ((current = list_shift_first(&tag_list)) != NULL) {
            const char *tagstr = mpd_tag_name(tag);
            sds filename = sdsdup(current->key);
            sanitize_filename(filename);
            sds playlist = sdscatfmt(sdsempty(), "%S%s%s-%s", mympd_worker_state->smartpls_prefix, (sdslen(mympd_worker_state->smartpls_prefix) > 0 ? "-" : ""), tagstr, filename);
            sds plpath = sdscatfmt(sdsempty(), "%S/%s/%s", mympd_worker_state->config->workdir, DIR_WORK_SMARTPLS, playlist);
            if (testfile_read(plpath) == false) {
                //file does not exist, create it
                sds expression = sdsnewlen("(", 1);
                expression = escape_mpd_search_expression(expression, tagstr, "==", current->key);
                expression = sdscatlen(expression, ")", 1);
                bool rc = smartpls_save_search(mympd_worker_state->config->workdir, playlist, expression, mympd_worker_state->smartpls_sort, false, 0);
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
 * @param mympd_worker_state pointer to the t_mympd_worker_state struct
 * @param playlist playlist to update
 * @return true on success, else false
 */
static bool mympd_worker_smartpls_delete(struct t_mympd_worker_state *mympd_worker_state, const char *playlist) {
    struct mpd_playlist *pl;
    bool exists = false;

    //first check if playlist exists
    if (mpd_send_list_playlists(mympd_worker_state->partition_state->conn)) {
        while ((pl = mpd_recv_playlist(mympd_worker_state->partition_state->conn)) != NULL) {
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
    mpd_response_finish(mympd_worker_state->partition_state->conn);
    if (mympd_check_error_and_recover(mympd_worker_state->partition_state, NULL, "mpd_send_list_playlists") == false) {
        return false;
    }

    //delete playlist if exists
    if (exists == true) {
        mpd_run_rm(mympd_worker_state->partition_state->conn, playlist);
        return mympd_check_error_and_recover(mympd_worker_state->partition_state, NULL, "mpd_run_rm");
    }
    return true;
}

/**
 * Updates a search based smart playlist
 * @param mympd_worker_state pointer to the t_mympd_worker_state struct
 * @param playlist playlist to update
 * @param expression mpd search expression
 * @param sort sort by tag
 * @param sortdesc sort descending?
 * @param max_entries max entries to add to the playlist
 * @return true on success, else false
 */
static bool mympd_worker_smartpls_update_search(struct t_mympd_worker_state *mympd_worker_state,
        const char *playlist, const char *expression, const char *sort, bool sortdesc, unsigned max_entries)
{
    sds error = sdsempty();
    const char *r_sort = strcmp(sort, "shuffle") == 0
        ? NULL
        : sort;
    if (strcmp(sort, "shuffle") == 0 ||
        max_entries == 0)
    {
        max_entries = UINT_MAX;
    }
    bool rc = mympd_client_search_add_to_plist_window(mympd_worker_state->partition_state,
        expression, playlist, UINT_MAX, r_sort, sortdesc, 0, max_entries, &error);
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
 * Updates a sticker based smart playlist
 * @param mympd_worker_state pointer to the t_mympd_worker_state struct
 * @param playlist playlist to update
 * @param sticker sticker evaluate
 * @param value sticker value
 * @param op compare operator
 * @param sort sort by field
 * @param sort_desc sort descending?
 * @param max_entries max entries to add to the playlist
 * @return true on success, else false
 */
static bool mympd_worker_smartpls_update_sticker(struct t_mympd_worker_state *mympd_worker_state,
        const char *playlist, const char *sticker, const char *value, const char *op,
        const char *sort, bool sort_desc, unsigned max_entries)
{
    enum mpd_sticker_operator oper = sticker_oper_parse(op);
    if (oper == MPD_STICKER_OP_UNKOWN) {
        MYMPD_LOG_ERROR(NULL, "Invalid sticker compare operator");
        return false;
    }

    enum mpd_sticker_sort sort_op = sticker_sort_parse(sort);

    if (sort_op == MPD_STICKER_SORT_UNKOWN ||
        max_entries == 0)
    {
        // we must get all entries
        max_entries = UINT_MAX;
    }

    if (sort_op == MPD_STICKER_SORT_UNKOWN) {
        sort_op = MPD_STICKER_SORT_URI;
    }

    struct t_list *add_list = stickerdb_find_stickers_sorted(mympd_worker_state->stickerdb, STICKER_TYPE_SONG,
        NULL, sticker, oper, value, sort_op, sort_desc, 0, max_entries);
    if (add_list == NULL) {
        MYMPD_LOG_ERROR(NULL, "Could not fetch stickers for \"%s\"", sticker);
        return false;
    }

    unsigned i = 0;
    struct t_list_node *current;
    bool rc = true;
    while (add_list->length > 0) {
        if (mpd_command_list_begin(mympd_worker_state->partition_state->conn, false)) {
            unsigned j = 0;
            while ((current = list_shift_first(add_list)) != NULL) {
                i++;
                j++;
                rc = mpd_send_playlist_add(mympd_worker_state->partition_state->conn, playlist, current->key);
                list_node_free(current);
                if (rc == false) {
                    mympd_set_mpd_failure(mympd_worker_state->partition_state, "Error adding command to command list mpd_send_playlist_add");
                    break;
                }
                if (j == MPD_COMMANDS_MAX) {
                    break;
                }
            }
            mympd_client_command_list_end_check(mympd_worker_state->partition_state);
        }
        mpd_response_finish(mympd_worker_state->partition_state->conn);
        if (mympd_check_error_and_recover(mympd_worker_state->partition_state, NULL, "mpd_send_playlist_add") == false) {
            rc = false;
            break;
        }
    }
    list_free(add_list);
    if (rc == true) {
        MYMPD_LOG_INFO(NULL, "Updated smart playlist \"%s\" with %u songs", playlist, i);
    }
    return rc;
}

/**
 * Updates a newest song smart playlist
 * @param mympd_worker_state pointer to the t_mympd_worker_state struct
 * @param playlist playlist to update
 * @param timerange timerange in seconds since last database update
 * @param sort sort by tag
 * @param sortdesc sort descending?
 * @param max_entries max entries to add to the playlist
 * @return true on success, else false
 */
static bool mympd_worker_smartpls_update_newest(struct t_mympd_worker_state *mympd_worker_state,
        const char *playlist, unsigned timerange, const char *sort, bool sortdesc, unsigned max_entries)
{
    unsigned long value_max = 0;
    struct mpd_stats *stats = mpd_run_stats(mympd_worker_state->partition_state->conn);
    if (stats != NULL) {
        value_max = mpd_stats_get_db_update_time(stats);
        mpd_stats_free(stats);
    }
    mpd_response_finish(mympd_worker_state->partition_state->conn);
    if (mympd_check_error_and_recover(mympd_worker_state->partition_state, NULL, "mpd_run_stats") == false) {
        return false;
    }

    //prevent overflow
    if (timerange > value_max) {
        return false;
    }
    value_max = value_max - timerange;

    if (strcmp(sort, "shuffle") == 0 ||
        max_entries == 0)
    {
        max_entries = UINT_MAX;
    }

    sds error = sdsempty();
    const char *r_sort = strcmp(sort, "shuffle") == 0
        ? NULL
        : sort;
    sds expression = mympd_worker_state->mpd_state->feat.db_added == true
        ? sdscatfmt(sdsempty(), "(added-since '%U')", value_max)
        : sdscatfmt(sdsempty(), "(modified-since '%U')", value_max);
    bool rc = mympd_client_search_add_to_plist_window(mympd_worker_state->partition_state, expression,
        playlist, UINT_MAX, r_sort, sortdesc, 0, max_entries, &error);
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
