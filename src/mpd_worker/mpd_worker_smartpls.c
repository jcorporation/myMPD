/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_worker_smartpls.h"

#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"
#include "../lib/validate.h"
#include "../mpd_shared/mpd_shared_playlists.h"
#include "../mpd_shared/mpd_shared_search.h"
#include "mpd_worker_utility.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//private definitions
static bool mpd_worker_smartpls_per_tag(struct t_mpd_worker_state *mpd_worker_state);
static bool mpd_worker_smartpls_clear(struct t_mpd_worker_state *mpd_worker_state, const char *playlist);
static bool mpd_worker_smartpls_update_search(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const char *expression);
static bool mpd_worker_smartpls_update_sticker(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const char *sticker, const int maxentries, const int minvalue);
static bool mpd_worker_smartpls_update_newest(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const int timerange);

//public functions
bool mpd_worker_smartpls_update_all(struct t_mpd_worker_state *mpd_worker_state, bool force) {
    if (mpd_worker_state->mpd_state->feat_mpd_playlists == false) {
        MYMPD_LOG_DEBUG("Playlists are disabled");
        return true;
    }

    mpd_worker_smartpls_per_tag(mpd_worker_state);

    time_t db_mtime = mpd_shared_get_db_mtime(mpd_worker_state->mpd_state);
    MYMPD_LOG_DEBUG("Database mtime: %lld", (long long)db_mtime);

    sds dirname = sdscatfmt(sdsempty(), "%s/smartpls", mpd_worker_state->config->workdir);
    errno = 0;
    DIR *dir = opendir (dirname);
    if (dir == NULL) {
        MYMPD_LOG_ERROR("Can't open smart playlist directory \"%s\"", dirname);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(dirname);
        return false;
    }
    struct dirent *next_file;
    while ((next_file = readdir(dir)) != NULL) {
        if (next_file->d_type != DT_REG) {
            continue;
        }
        time_t playlist_mtime = mpd_shared_get_playlist_mtime(mpd_worker_state->mpd_state, next_file->d_name);
        time_t smartpls_mtime = mpd_shared_get_smartpls_mtime(mpd_worker_state->config, next_file->d_name);
        MYMPD_LOG_DEBUG("Playlist %s: playlist mtime %lld, smartpls mtime %lld", next_file->d_name, (long long)playlist_mtime, (long long)smartpls_mtime);
        if (force == true || db_mtime > playlist_mtime || smartpls_mtime > playlist_mtime) {
            mpd_worker_smartpls_update(mpd_worker_state, next_file->d_name);
        }
        else {
            MYMPD_LOG_INFO("Update of smart playlist %s skipped, already up to date", next_file->d_name);
        }
    }
    closedir (dir);
    FREE_SDS(dirname);
    return true;
}

bool mpd_worker_smartpls_update(struct t_mpd_worker_state *mpd_worker_state, const char *playlist) {
    if (mpd_worker_state->mpd_state->feat_mpd_playlists == false) {
        MYMPD_LOG_WARN("Playlists are disabled");
        return true;
    }

    sds smartpltype = NULL;
    bool rc = true;
    sds sds_buf1 = NULL;
    int int_buf1;
    int int_buf2;

    sds filename = sdscatfmt(sdsempty(), "%s/smartpls/%s", mpd_worker_state->config->workdir, playlist);
    FILE *fp = fopen(filename, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Cant open smart playlist file \"%s\"", playlist);
        FREE_SDS(filename);
        return false;
    }
    sds content = sdsempty();
    sds_getfile(&content, fp, 2000);
    fclose(fp);

    if (json_get_string(content, "$.type", 1, 200, &smartpltype, vcb_isalnum, NULL) != true) {
        MYMPD_LOG_ERROR("Cant read smart playlist type from \"%s\"", filename);
        FREE_SDS(filename);
        return false;
    }
    if (strcmp(smartpltype, "sticker") == 0) {
        if (json_get_string(content, "$.sticker", 1, 200, &sds_buf1, vcb_isalnum, NULL) == true &&
            json_get_int(content, "$.maxentries", 0, MPD_PLAYLIST_LENGTH_MAX, &int_buf1, NULL) == true &&
            json_get_int(content, "$.minvalue", 0, 100, &int_buf2, NULL) == true)
        {
            rc = mpd_worker_smartpls_update_sticker(mpd_worker_state, playlist, sds_buf1, int_buf1, int_buf2);
            if (rc == false) {
                MYMPD_LOG_ERROR("Update of smart playlist \"%s\" (sticker) failed.", playlist);
            }
        }
        else {
            MYMPD_LOG_ERROR("Can't parse smart playlist file \"%s\" (sticker)", filename);
            rc = false;
        }
    }
    else if (strcmp(smartpltype, "newest") == 0) {
        if (json_get_int(content, "$.timerange", 0, JSONRPC_INT_MAX, &int_buf1, NULL) == true) {
            rc = mpd_worker_smartpls_update_newest(mpd_worker_state, playlist, int_buf1);
            if (rc == false) {
                MYMPD_LOG_ERROR("Update of smart playlist \"%s\" failed (newest)", playlist);
            }
        }
        else {
            MYMPD_LOG_ERROR("Can't parse smart playlist file \"%s\" (newest)", filename);
            rc = false;
        }
    }
    else if (strcmp(smartpltype, "search") == 0) {
        if (json_get_string(content, "$.expression", 1, 200, &sds_buf1, vcb_isname, NULL) == true) {
            rc = mpd_worker_smartpls_update_search(mpd_worker_state, playlist, sds_buf1);
            if (rc == false) {
                MYMPD_LOG_ERROR("Update of smart playlist \"%s\" (search) failed", playlist);
            }
        }
        else {
            MYMPD_LOG_ERROR("Can't parse smart playlist file \"%s\" (search)", filename);
            rc = false;
        }
    }
    if (rc == true) {
        FREE_SDS(sds_buf1);
        if (json_get_string(content, "$.sort", 0, 100, &sds_buf1, vcb_ismpdsort, NULL) == true) {
            if (sdslen(sds_buf1) > 0) {
                mpd_shared_playlist_shuffle_sort(mpd_worker_state->mpd_state, NULL, NULL, 0, playlist, sds_buf1);
            }
        }
    }
    FREE_SDS(smartpltype);
    FREE_SDS(sds_buf1);
    FREE_SDS(content);
    FREE_SDS(filename);
    return rc;
}

//private functions
static bool mpd_worker_smartpls_per_tag(struct t_mpd_worker_state *mpd_worker_state) {
    for (unsigned i = 0; i < mpd_worker_state->smartpls_generate_tag_types.len; i++) {
        enum mpd_tag_type tag = mpd_worker_state->smartpls_generate_tag_types.tags[i];
        bool rc = mpd_search_db_tags(mpd_worker_state->mpd_state->conn, tag);

        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_db_tags") == false) {
            mpd_search_cancel(mpd_worker_state->mpd_state->conn);
            return false;
        }
        rc = mpd_search_commit(mpd_worker_state->mpd_state->conn);
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_commit") == false) {
            return false;
        }
        struct mpd_pair *pair;
        struct t_list tag_list;
        list_init(&tag_list);
        while ((pair = mpd_recv_pair_tag(mpd_worker_state->mpd_state->conn, tag)) != NULL) {
            if (strlen(pair->value) > 0) {
                list_push(&tag_list, pair->value, 0, NULL, NULL);
            }
            mpd_return_pair(mpd_worker_state->mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_worker_state->mpd_state->conn);
        if (check_error_and_recover2(mpd_worker_state->mpd_state, NULL, NULL, 0, false) == false) {
            list_clear(&tag_list);
            return false;
        }
        struct t_list_node *current = tag_list.head;
        while (current != NULL) {
            const char *tagstr = mpd_tag_name(tag);
            sds filename = sdsdup(current->key);
            sds_sanitize_filename(filename);
            sds playlist = sdscatfmt(sdsempty(), "%s%s%s-%s", mpd_worker_state->smartpls_prefix, (sdslen(mpd_worker_state->smartpls_prefix) > 0 ? "-" : ""), tagstr, filename);
            sds plpath = sdscatfmt(sdsempty(), "%s/smartpls/%s", mpd_worker_state->config->workdir, playlist);
            if (access(plpath, F_OK) == -1) { /* Flawfinder: ignore */
                sds expression = sdsnewlen("(", 1);
                expression = escape_mpd_search_expression(expression, tagstr, "==", current->key);
                expression = sdscatlen(expression, ")", 1);
                rc = mpd_shared_smartpls_save(mpd_worker_state->config->workdir, "search", playlist, expression, 0, 0, mpd_worker_state->smartpls_sort);
                FREE_SDS(expression);
                if (rc == true) {
                    MYMPD_LOG_INFO("Created smart playlist \"%s\"", playlist);
                }
                else {
                    MYMPD_LOG_ERROR("Creation of smart playlist \"%s\" failed", playlist);
                }
            }
            FREE_SDS(playlist);
            FREE_SDS(plpath);
            FREE_SDS(filename);
            current = current->next;
        }
        list_clear(&tag_list);
    }
    return true;
}

static bool mpd_worker_smartpls_clear(struct t_mpd_worker_state *mpd_worker_state, const char *playlist) {
    struct mpd_playlist *pl;
    bool exists = false;

    //first check if playlist exists
    bool rc = mpd_send_list_playlists(mpd_worker_state->mpd_state->conn);
    if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_playlists") == false) {
        return false;
    }
    while ((pl = mpd_recv_playlist(mpd_worker_state->mpd_state->conn)) != NULL) {
        const char *plpath = mpd_playlist_get_path(pl);
        if (strcmp(playlist, plpath) == 0) {
            exists = true;
        }
        mpd_playlist_free(pl);
        if (exists == true) {
            break;
        }
    }
    mpd_response_finish(mpd_worker_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_worker_state->mpd_state, NULL, NULL, 0, false) == false) {
        return false;
    }

    //delete playlist if exists
    if (exists == true) {
        rc = mpd_run_rm(mpd_worker_state->mpd_state->conn, playlist);
        if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_rm") == false) {
            return false;
        }
    }
    return true;
}

static bool mpd_worker_smartpls_update_search(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const char *expression) {
    mpd_worker_smartpls_clear(mpd_worker_state, playlist);
    sds buffer = sdsempty();
    bool result;
    buffer = mpd_shared_search_adv(mpd_worker_state->mpd_state, buffer, NULL, 0, expression, NULL, true, playlist, UINT_MAX, 0, 0, MPD_PLAYLIST_LENGTH_MAX, NULL, NULL, &result);
    FREE_SDS(buffer);
    if (result == true) {
        MYMPD_LOG_INFO("Updated smart playlist \"%s\"", playlist);
    }
    else {
        MYMPD_LOG_ERROR("Updating smart playlist \"%s\" failed", playlist);
    }
    return result;
}

static bool mpd_worker_smartpls_update_sticker(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const char *sticker,
        const int maxentries, const int minvalue)
{
    bool rc = mpd_send_sticker_find(mpd_worker_state->mpd_state->conn, "song", "", sticker);
    if (check_rc_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_sticker_find") == false) {
        return false;
    }

    struct t_list add_list;
    list_init(&add_list);

    struct mpd_pair *pair;
    char *uri = NULL;
    int value_max = 0;

    while ((pair = mpd_recv_pair(mpd_worker_state->mpd_state->conn)) != NULL) {
        if (strcmp(pair->name, "file") == 0) {
            FREE_PTR(uri);
            uri = strdup(pair->value);
        }
        else if (strcmp(pair->name, "sticker") == 0) {
            size_t j;
            const char *p_value = mpd_parse_sticker(pair->value, &j);
            if (p_value != NULL) {
                char *crap;
                int value = (int)strtoimax(p_value, &crap, 10);
                if (value >= 1) {
                    list_insert_sorted_by_value_i(&add_list, uri, value, NULL, NULL, LIST_SORT_ASC);
                }
                if (value > value_max) {
                    value_max = value;
                }
            }
        }
        mpd_return_pair(mpd_worker_state->mpd_state->conn, pair);
    }
    mpd_response_finish(mpd_worker_state->mpd_state->conn);
    FREE_PTR(uri);
    if (check_error_and_recover2(mpd_worker_state->mpd_state, NULL, NULL, 0, false) == false) {
        return false;
    }

    mpd_worker_smartpls_clear(mpd_worker_state, playlist);

    if (minvalue > 0) {
        value_max = minvalue;
    }
    else if (value_max > 2) {
        value_max = value_max / 2;
    }

    int i = 0;
    if (mpd_command_list_begin(mpd_worker_state->mpd_state->conn, false)) {
        struct t_list_node *current;
        while ((current = list_shift_first(&add_list)) != NULL) {
            if (current->value_i >= value_max) {
                rc = mpd_send_playlist_add(mpd_worker_state->mpd_state->conn, playlist, current->key);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Error adding command to command list mpd_send_playlist_add");
                    break;
                }
                i++;
                if (i >= maxentries) {
                    break;
                }
            }
            list_node_free_user_data(current, list_free_cb_ignore_user_data);
        }
        if (mpd_command_list_end(mpd_worker_state->mpd_state->conn)) {
            mpd_response_finish(mpd_worker_state->mpd_state->conn);
        }
        if (check_error_and_recover2(mpd_worker_state->mpd_state, NULL, NULL, 0, false) == false) {
            list_clear(&add_list);
            return false;
        }
    }
    MYMPD_LOG_INFO("Updated smart playlist \"%s\" with %d songs, minValue: %d", playlist, i, value_max);
    return true;
}

static bool mpd_worker_smartpls_update_newest(struct t_mpd_worker_state *mpd_worker_state, const char *playlist, const int timerange) {
    unsigned long value_max = 0;
    struct mpd_stats *stats = mpd_run_stats(mpd_worker_state->mpd_state->conn);
    if (stats != NULL) {
        value_max = mpd_stats_get_db_update_time(stats);
        mpd_stats_free(stats);
    }
    else {
        check_error_and_recover(mpd_worker_state->mpd_state, NULL, NULL, 0);
        return false;
    }

    //prevent overflow
    if (timerange < 0) {
        return false;
    }
    value_max = value_max - (unsigned long)timerange;

    mpd_worker_smartpls_clear(mpd_worker_state, playlist);

    sds buffer = sdsempty();
    sds method = sdsempty();
    bool result = false;
    sds searchstr = sdscatfmt(sdsempty(), "(modified-since '%U')", value_max);
    buffer = mpd_shared_search_adv(mpd_worker_state->mpd_state, buffer, method, 0, searchstr, "LastModified", true, playlist, UINT_MAX, 0, 0, MPD_PLAYLIST_LENGTH_MAX, NULL, NULL, &result);
    FREE_SDS(searchstr);
    if (result == true) {
        MYMPD_LOG_INFO("Updated smart playlist \"%s\"", playlist);
    }
    else {
        MYMPD_LOG_ERROR("Updating smart playlist \"%s\" failed", playlist);
    }
    FREE_SDS(buffer);
    FREE_SDS(method);
    return true;
}
