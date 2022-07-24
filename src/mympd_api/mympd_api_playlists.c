/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_playlists.h"

#include "../../dist/utf8/utf8.h"
#include "../lib/api.h"
#include "../lib/filehandler.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/rax_extras.h"
#include "../lib/sds_extras.h"
#include "../lib/smartpls.h"
#include "../lib/state_files.h"
#include "../lib/sticker_cache.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_client/mpd_client_errorhandler.h"
#include "../mpd_client/mpd_client_playlists.h"
#include "../mpd_client/mpd_client_search.h"
#include "../mpd_client/mpd_client_search_local.h"
#include "../mpd_client/mpd_client_tags.h"
#include "mympd_api_sticker.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Simple struct to save playlist data
 */
struct t_pl_data {
    time_t last_modified;
    enum playlist_types type;
    sds name;
};

/**
 * Frees the t_pl_data struct used as callback for rax_free_data
 * @param data void pointer to a t_sticker_value struct
 */
static void free_t_pl_data(void *data) {
    struct t_pl_data *pl_data = (struct t_pl_data *)data;
    FREE_SDS(pl_data->name);
    FREE_PTR(pl_data);
}

sds mympd_api_playlist_list(struct t_partition_state *partition_state, sds buffer, long request_id,
        const long offset, const long limit, sds searchstr, enum playlist_types type)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYLIST_LIST;
    bool rc = mpd_send_list_playlists(partition_state->conn);
    if (mympd_check_rc_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, rc, "mpd_send_list_playlists") == false) {
        return buffer;
    }
    size_t search_len = sdslen(searchstr);

    struct mpd_playlist *pl;
    rax *entity_list = raxNew();

    long real_limit = offset + limit;
    sds key = sdsempty();
    while ((pl = mpd_recv_playlist(partition_state->conn)) != NULL) {
        const char *plpath = mpd_playlist_get_path(pl);
        bool smartpls = is_smartpls(partition_state->mpd_shared_state->config->workdir, plpath);
        if ((search_len == 0 || utf8casestr(plpath, searchstr) != NULL) &&
            (type == PLTYPE_ALL || (type == PLTYPE_STATIC && smartpls == false) || (type == PLTYPE_SMART && smartpls == true)))
        {
            struct t_pl_data *data = malloc_assert(sizeof(struct t_pl_data));
            data->last_modified = mpd_playlist_get_last_modified(pl);
            data->type = smartpls == true ? PLTYPE_SMART : PLTYPE_STATIC;
            data->name = sdsnew(plpath);
            sdsclear(key);
            key = sdscatsds(key, data->name);
            sds_utf8_tolower(key);
            while (raxTryInsert(entity_list, (unsigned char *)key, sdslen(key), data, NULL) == 0) {
                //duplicate - add chars until it is uniq
                key = sdscatlen(key, ":", 1);
            }
        }
        mpd_playlist_free(pl);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
        //free result
        rax_free_data(entity_list, free_t_pl_data);
        //return error message
        return buffer;
    }

    //add empty smart playlists
    if (type != PLTYPE_STATIC) {
        sds smartpls_path = sdscatfmt(sdsempty(), "%S/smartpls", partition_state->mpd_shared_state->config->workdir);
        errno = 0;
        DIR *smartpls_dir = opendir(smartpls_path);
        if (smartpls_dir != NULL) {
            struct dirent *next_file;
            while ((next_file = readdir(smartpls_dir)) != NULL ) {
                if (next_file->d_type == DT_REG &&
                    (search_len == 0 || utf8casestr(next_file->d_name, searchstr) != NULL)
                ) {
                    struct t_pl_data *data = malloc_assert(sizeof(struct t_pl_data));
                    data->last_modified = smartpls_get_mtime(partition_state->mpd_shared_state->config->workdir, next_file->d_name);
                    data->type = PLTYPE_SMARTPLS_ONLY;
                    data->name = sdsnew(next_file->d_name);
                    sdsclear(key);
                    key = sdscat(key, next_file->d_name);
                    sds_utf8_tolower(key);
                    if (raxTryInsert(entity_list, (unsigned char *)key, sdslen(key), data, NULL) == 0) {
                        //smart playlist already added
                        FREE_SDS(data->name);
                        FREE_PTR(data);
                    }
                }
            }
            closedir(smartpls_dir);
        }
        else {
            MYMPD_LOG_ERROR("Can not open smartpls dir \"%s\"", smartpls_path);
            MYMPD_LOG_ERRNO(errno);
        }
        FREE_SDS(smartpls_path);
    }
    FREE_SDS(key);
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer,"\"data\":[");

    long entity_count = 0;
    long entities_returned = 0;
    raxIterator iter;
    raxStart(&iter, entity_list);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        struct t_pl_data *data = (struct t_pl_data *)iter.data;
        if (entity_count >= offset &&
            entity_count < real_limit)
        {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscat(buffer, "{");
            buffer = tojson_char(buffer, "Type", (data->type == PLTYPE_STATIC ? "plist" : "smartpls"), true);
            buffer = tojson_sds(buffer, "uri", data->name, true);
            buffer = tojson_sds(buffer, "name", data->name, true);
            buffer = tojson_llong(buffer, "lastModified", data->last_modified, true);
            buffer = tojson_bool(buffer, "smartplsOnly", data->type == PLTYPE_SMARTPLS_ONLY ? true : false, false);
            buffer = sdscat(buffer, "}");
        }
        entity_count++;
        FREE_SDS(data->name);
        FREE_PTR(data);
    }
    raxStop(&iter);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_sds(buffer, "searchstr", searchstr, true);
    buffer = tojson_llong(buffer, "totalEntities", (long long)entity_list->numele, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, false);
    buffer = jsonrpc_respond_end(buffer);
    raxFree(entity_list);
    return buffer;
}

sds mympd_api_playlist_content_list(struct t_partition_state *partition_state, sds buffer, long request_id,
        sds plist, const long offset, const long limit, sds searchstr, const struct t_tags *tagcols)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYLIST_CONTENT_LIST;
    bool rc = mpd_send_list_playlist_meta(partition_state->conn, plist);
    if (mympd_check_rc_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, rc, "mpd_send_list_playlist_meta") == false) {
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer,"\"data\":[");

    struct mpd_song *song;
    long entities_returned = 0;
    long entity_count = 0;
    unsigned total_time = 0;
    long real_limit = offset + limit;
    time_t last_played_max = 0;
    sds last_played_song_uri = sdsempty();
    while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
        total_time += mpd_song_get_duration(song);
        if (entity_count >= offset &&
            entity_count < real_limit)
        {
            if (search_mpd_song(song, searchstr, tagcols) == true) {
                if (entities_returned++) {
                    buffer= sdscatlen(buffer, ",", 1);
                }
                buffer = sdscatlen(buffer, "{", 1);
                if (is_streamuri(mpd_song_get_uri(song)) == true) {
                    buffer = tojson_char(buffer, "Type", "stream", true);
                }
                else {
                    buffer = tojson_char(buffer, "Type", "song", true);
                }
                buffer = tojson_long(buffer, "Pos", entity_count, true);
                buffer = get_song_tags(buffer, partition_state, tagcols, song);
                if (partition_state->mpd_shared_state->feat_mpd_stickers) {
                    buffer = sdscatlen(buffer, ",", 1);
                    struct t_sticker *sticker = get_sticker_from_cache(&partition_state->mpd_shared_state->sticker_cache, mpd_song_get_uri(song));
                    buffer = mympd_api_print_sticker(buffer, sticker);
                    if (sticker != NULL &&
                        sticker->lastPlayed > last_played_max)
                    {
                        last_played_max = sticker->lastPlayed;
                        last_played_song_uri = sds_replace(last_played_song_uri, mpd_song_get_uri(song));
                    }
                }
                buffer = sdscatlen(buffer, "}", 1);
            }
            else {
                entity_count--;
            }
        }
        mpd_song_free(song);
        entity_count++;
    }

    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
        return buffer;
    }

    bool smartpls = is_smartpls(partition_state->mpd_shared_state->config->workdir, plist);

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_uint(buffer, "totalTime", total_time, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_sds(buffer, "searchstr", searchstr, true);
    buffer = tojson_sds(buffer, "plist", plist, true);
    buffer = tojson_bool(buffer, "smartpls", smartpls, true);
    buffer = sdscat(buffer, "\"lastPlayedSong\":{");
    buffer = tojson_llong(buffer, "time", (long long)last_played_max, true);
    buffer = tojson_sds(buffer, "uri", last_played_song_uri, false);
    buffer = sdscatlen(buffer, "}", 1);
    buffer = jsonrpc_respond_end(buffer);

    FREE_SDS(last_played_song_uri);
    return buffer;
}

sds mympd_api_playlist_rename(struct t_partition_state *partition_state, sds buffer,
        long request_id, const char *old_playlist, const char *new_playlist)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYLIST_RENAME;
    //first handle smart playlists
    sds old_pl_file = sdscatfmt(sdsempty(), "%S/smartpls/%s", partition_state->mpd_shared_state->config->workdir, old_playlist);
    sds new_pl_file = sdscatfmt(sdsempty(), "%S/smartpls/%s", partition_state->mpd_shared_state->config->workdir, new_playlist);
    //link old name to new name
    if (access(old_pl_file, F_OK) == 0) { /* Flawfinder: ignore */
        //smart playlist file exists
        errno = 0;
        if (link(old_pl_file, new_pl_file) == -1) {
            //handle new smart playlist name exists already
            if (errno == EEXIST) {
                MYMPD_LOG_ERROR("A playlist with name \"%s\" already exists", new_pl_file);
                buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
                    JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "A smart playlist with this name already exists");
                FREE_SDS(old_pl_file);
                FREE_SDS(new_pl_file);
                return buffer;
            }
            // other errors
            MYMPD_LOG_ERROR("Renaming smart playlist from \"%s\" to \"%s\" failed", old_pl_file, new_pl_file);
            MYMPD_LOG_ERRNO(errno);
            buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
                JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Renaming playlist failed");
            FREE_SDS(old_pl_file);
            FREE_SDS(new_pl_file);
            return buffer;
        }
        //remove old smart playlist
        if (rm_file(old_pl_file) == false) {
            //try to remove new smart playlist to prevent duplicates
            try_rm_file(new_pl_file);
            buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
                JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Renaming playlist failed");
            FREE_SDS(old_pl_file);
            FREE_SDS(new_pl_file);
            return buffer;
        }
    }
    FREE_SDS(old_pl_file);
    FREE_SDS(new_pl_file);
    //rename mpd playlist
    bool rc = mpd_run_rename(partition_state->conn, old_playlist, new_playlist);
    if (mympd_check_rc_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, rc, "mpd_run_rename") == true) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Successfully renamed playlist");
    }
    return buffer;
}

sds mympd_api_playlist_delete(struct t_partition_state *partition_state, sds buffer,
        long request_id, const char *playlist, bool smartpls_only)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYLIST_RM;
    //remove smart playlist
    sds pl_file = sdscatfmt(sdsempty(), "%S/smartpls/%s", partition_state->mpd_shared_state->config->workdir, playlist);
    if (try_rm_file(pl_file) == RM_FILE_ERROR) {
        //ignores none existing smart playlist
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_ERROR, "Deleting smart playlist failed");
        MYMPD_LOG_ERROR("Deleting smart playlist \"%s\" failed", playlist);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(pl_file);
        return buffer;
    }
    FREE_SDS(pl_file);
    if (smartpls_only == true) {
        send_jsonrpc_event(JSONRPC_EVENT_UPDATE_STORED_PLAYLIST);
        buffer = jsonrpc_respond_ok(buffer, cmd_id, request_id, JSONRPC_FACILITY_PLAYLIST);
        return buffer;
    }
    //remove mpd playlist
    bool rc = mpd_run_rm(partition_state->conn, playlist);
    if (mympd_check_rc_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, rc, "mpd_run_rm") == true) {
        buffer = jsonrpc_respond_ok(buffer, cmd_id, request_id, JSONRPC_FACILITY_PLAYLIST);
    }
    return buffer;
}

sds mympd_api_playlist_delete_all(struct t_partition_state *partition_state, sds buffer,
        long request_id, const char *type)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_PLAYLIST_RM_ALL;
    bool rc = mpd_send_list_playlists(partition_state->conn);
    if (mympd_check_rc_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, rc, "mpd_send_list_playlists") == false) {
        return buffer;
    }

    struct t_list playlists;
    list_init(&playlists);
    //get all mpd playlists
    struct mpd_playlist *pl;
    while ((pl = mpd_recv_playlist(partition_state->conn)) != NULL) {
        const char *plpath = mpd_playlist_get_path(pl);
        list_push(&playlists, plpath, 1, NULL, NULL);
        mpd_playlist_free(pl);
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
        list_clear(&playlists);
        return buffer;
    }
    //delete each smart playlist file that have no corresponding mpd playlist file
    sds smartpls_path = sdscatfmt(sdsempty(), "%S/smartpls", partition_state->mpd_shared_state->config->workdir);
    errno = 0;
    DIR *smartpls_dir = opendir(smartpls_path);
    if (smartpls_dir != NULL) {
        struct dirent *next_file;
        sds smartpls_file = sdsempty();
        while ((next_file = readdir(smartpls_dir)) != NULL ) {
            if (next_file->d_type == DT_REG) {
                if (list_get_node(&playlists, next_file->d_name) == NULL) {
                    smartpls_file = sdscatfmt(smartpls_file, "%S/smartpls/%s", partition_state->mpd_shared_state->config->workdir, next_file->d_name);
                    if (rm_file(smartpls_file) == true) {
                        MYMPD_LOG_INFO("Removed orphaned smartpls file \"%s\"", smartpls_file);
                    }
                    sdsclear(smartpls_file);
                }
            }
        }
        FREE_SDS(smartpls_file);
        closedir(smartpls_dir);
    }
    else {
        MYMPD_LOG_ERROR("Can not open smartpls dir \"%s\"", smartpls_path);
        MYMPD_LOG_ERRNO(errno);
    }
    FREE_SDS(smartpls_path);

    if (strcmp(type, "deleteEmptyPlaylists") == 0) {
        struct t_list_node *current = playlists.head;
        while (current != NULL) {
            current->value_i = mpd_client_enum_playlist(partition_state, current->key, true);
            current = current->next;
        }
    }

    if (mpd_command_list_begin(partition_state->conn, false)) {
        struct t_list_node *current;
        while ((current = list_shift_first(&playlists)) != NULL) {
            bool smartpls = false;
            if (strcmp(type, "deleteSmartPlaylists") == 0) {
                sds smartpls_file = sdscatfmt(sdsempty(), "%S/smartpls/%s", partition_state->mpd_shared_state->config->workdir, current->key);
                if (try_rm_file(smartpls_file) == RM_FILE_OK) {
                    MYMPD_LOG_INFO("Smartpls file %s removed", smartpls_file);
                    smartpls = true;
                }
                FREE_SDS(smartpls_file);
            }
            if (strcmp(type, "deleteAllPlaylists") == 0 ||
                (strcmp(type, "deleteSmartPlaylists") == 0 && smartpls == true) ||
                (strcmp(type, "deleteEmptyPlaylists") == 0 && current->value_i == 0))
            {
                rc = mpd_send_rm(partition_state->conn, current->key);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Error adding command to command list mpd_send_rm");
                    break;
                }
                MYMPD_LOG_INFO("Deleting mpd playlist %s", current->key);
            }
            list_node_free(current);
        }
        if (mpd_command_list_end(partition_state->conn)) {
            mpd_response_finish(partition_state->conn);
        }
        if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
            list_clear(&playlists);
            return buffer;
        }
    }
    else if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id) == false) {
        list_clear(&playlists);
        return buffer;
    }
    list_clear(&playlists);
    buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
        JSONRPC_FACILITY_PLAYLIST, JSONRPC_SEVERITY_INFO, "Playlists deleted");
    return buffer;
}
