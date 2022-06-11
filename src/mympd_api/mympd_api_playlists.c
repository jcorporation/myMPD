/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_playlists.h"

#include "../../dist/utf8/utf8.h"
#include "../lib/api.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_shared/mpd_shared_search.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "mympd_api_utility.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//private definitions
static int mympd_api_enum_playlist(struct t_mympd_state *mympd_state, const char *playlist, bool empty_check);
static bool smartpls_init(struct t_config *config, const char *name, const char *value);

//public functions
bool mympd_api_smartpls_default(struct t_config *config) {
    bool rc = true;

    //try to get prefix from state file, fallback to default value
    sds prefix = sdsempty();
    sds prefix_file = sdscatfmt(sdsempty(), "%s/state/smartpls_prefix", config->workdir);
    FILE *fp = fopen(prefix_file, OPEN_FLAGS_READ);
    if (fp != NULL) {
        if (sds_getline(&prefix, fp, 50) != 0) {
            prefix = sdscat(prefix, "myMPDsmart");
        }
        (void) fclose(fp);
    }
    else {
        prefix = sdscat(prefix, "myMPDsmart");
    }
    FREE_SDS(prefix_file);

    sds smartpls_file = sdscatfmt(sdsempty(), "%s%sbestRated", prefix, (sdslen(prefix) > 0 ? "-" : ""));
    rc = smartpls_init(config, smartpls_file,
        "{\"type\": \"sticker\", \"sticker\": \"like\", \"maxentries\": 200, \"minvalue\": 2, \"sort\": \"\"}");
    if (rc == false) {
        FREE_SDS(smartpls_file);
        FREE_SDS(prefix);
        return rc;
    }

    sdsclear(smartpls_file);
    smartpls_file = sdscatfmt(smartpls_file, "%s%smostPlayed", prefix, (sdslen(prefix) > 0 ? "-" : ""));
    rc = smartpls_init(config, smartpls_file,
        "{\"type\": \"sticker\", \"sticker\": \"playCount\", \"maxentries\": 200, \"minvalue\": 0, \"sort\": \"\"}");
    if (rc == false) {
        FREE_SDS(smartpls_file);
        FREE_SDS(prefix);
        return rc;
    }

    sdsclear(smartpls_file);
    smartpls_file = sdscatfmt(smartpls_file, "%s%snewestSongs", prefix, (sdslen(prefix) > 0 ? "-" : ""));
    rc = smartpls_init(config, smartpls_file,
        "{\"type\": \"newest\", \"timerange\": 604800, \"sort\": \"\"}");
    FREE_SDS(smartpls_file);
    FREE_SDS(prefix);

    return rc;
}

void mympd_api_smartpls_update(const char *playlist) {
    struct t_work_request *request = create_request(-1, 0, MYMPD_API_SMARTPLS_UPDATE, NULL);
    request->data = tojson_char(request->data, "plist", playlist, false);
    request->data = sdscatlen(request->data, "}}", 2);
    mympd_queue_push(mympd_api_queue, request, 0);
}

void mympd_api_smartpls_update_all(void) {
    struct t_work_request *request = create_request(-1, 0, MYMPD_API_SMARTPLS_UPDATE_ALL, NULL);
    request->data = sdscatlen(request->data, "}}", 2);
    mympd_queue_push(mympd_api_queue, request, 0);
}

sds mympd_api_playlist_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                             const long offset, const long limit, sds searchstr, enum playlist_types type)
{
    bool rc = mpd_send_list_playlists(mympd_state->mpd_state->conn);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_playlists") == false) {
        return buffer;
    }
    size_t search_len = sdslen(searchstr);

    struct mpd_playlist *pl;
    rax *entity_list = raxNew();

    struct t_pl_data {
        time_t last_modified;
        enum playlist_types type;
        sds name;
    };

    long real_limit = offset + limit;
    sds key = sdsempty();
    while ((pl = mpd_recv_playlist(mympd_state->mpd_state->conn)) != NULL) {
        const char *plpath = mpd_playlist_get_path(pl);
        bool smartpls = is_smartpls(mympd_state->config->workdir, plpath);
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
    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        //free result
        raxIterator iter;
        raxStart(&iter, entity_list);
        raxSeek(&iter, "^", NULL, 0);
        while (raxNext(&iter)) {
            struct t_pl_data *data = (struct t_pl_data *)iter.data;
            FREE_SDS(data->name);
            FREE_PTR(iter.data);
        }
        raxStop(&iter);
        raxFree(entity_list);
        //return error message
        return buffer;
    }

    //add empty smart playlists
    if (type != PLTYPE_STATIC) {
        sds smartpls_path = sdscatfmt(sdsempty(), "%s/smartpls", mympd_state->config->workdir);
        errno = 0;
        DIR *smartpls_dir = opendir(smartpls_path);
        if (smartpls_dir != NULL) {
            struct dirent *next_file;
            while ((next_file = readdir(smartpls_dir)) != NULL ) {
                if (next_file->d_type == DT_REG &&
                    (search_len == 0 || utf8casestr(next_file->d_name, searchstr) != NULL)
                ) {
                    struct t_pl_data *data = malloc_assert(sizeof(struct t_pl_data));
                    data->last_modified = mpd_shared_get_smartpls_mtime(mympd_state->config, next_file->d_name);
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
    buffer = jsonrpc_result_start(buffer, method, request_id);
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
    buffer = tojson_char(buffer, "searchstr", searchstr, true);
    buffer = tojson_llong(buffer, "totalEntities", (long long)entity_list->numele, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, false);
    buffer = jsonrpc_result_end(buffer);
    raxFree(entity_list);
    return buffer;
}

sds mympd_api_playlist_content_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                                 sds plist, const long offset, const long limit, sds searchstr, const struct t_tags *tagcols)
{
    bool rc = mpd_send_list_playlist_meta(mympd_state->mpd_state->conn, plist);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_playlist_meta") == false) {
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer,"\"data\":[");

    struct mpd_song *song;
    long entities_returned = 0;
    long entity_count = 0;
    unsigned total_time = 0;
    long real_limit = offset + limit;
    sds entityName = sdsempty();
    size_t search_len = sdslen(searchstr);
    while ((song = mpd_recv_song(mympd_state->mpd_state->conn)) != NULL) {
        total_time += mpd_song_get_duration(song);
        if (entity_count >= offset && entity_count < real_limit) {
            entityName = mpd_shared_get_tag_values(song, MPD_TAG_TITLE, entityName);
            if (search_len == 0 ||
                utf8casestr(entityName, searchstr) != NULL)
            {
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
                buffer = get_song_tags(buffer, mympd_state->mpd_state, tagcols, song);
                buffer = sdscatlen(buffer, "}", 1);
            }
            else {
                entity_count--;
            }
        }
        mpd_song_free(song);
        entity_count++;
    }
    FREE_SDS(entityName);

    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    bool smartpls = is_smartpls(mympd_state->config->workdir, plist);

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_uint(buffer, "totalTime", total_time, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_char(buffer, "searchstr", searchstr, true);
    buffer = tojson_char(buffer, "plist", plist, true);
    buffer = tojson_bool(buffer, "smartpls", smartpls, false);
    buffer = jsonrpc_result_end(buffer);

    return buffer;
}

sds mympd_api_playlist_rename(struct t_mympd_state *mympd_state, sds buffer, sds method,
                              long request_id, const char *old_playlist, const char *new_playlist)
{
    //first handle smart playlists
    sds old_pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->workdir, old_playlist);
    sds new_pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->workdir, new_playlist);
    //link old name to new name
    errno = 0;
    if (link(old_pl_file, new_pl_file) == -1) {
        if (errno == EEXIST) {
            //handle new smart playlist name exists already
            MYMPD_LOG_ERROR("A playlist with name \"%s\" already exists", new_pl_file);
            buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "A smart playlist with this name already exists");
            FREE_SDS(old_pl_file);
            FREE_SDS(new_pl_file);
            return buffer;
        }
        if (errno != ENOENT) {
            //ENOENT is OK, handle other errors
            MYMPD_LOG_ERROR("Renaming smart playlist from \"%s\" to \"%s\" failed", old_pl_file, new_pl_file);
            MYMPD_LOG_ERRNO(errno);
            buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Renaming playlist failed");
            FREE_SDS(old_pl_file);
            FREE_SDS(new_pl_file);
            return buffer;
        }
        //no smart playlist exists, this is OK
    }
    errno = 0;
    //remove old smart playlist
    if (unlink(old_pl_file) == -1) {
        MYMPD_LOG_ERROR("Deleting old smart playlist \"%s\" failed", old_pl_file);
        MYMPD_LOG_ERRNO(errno);
        //try to remove new smart playlist to prevent duplicates
        errno = 0;
        if (unlink(new_pl_file) == -1) {
            MYMPD_LOG_ERROR("Deleting new smart playlist \"%s\" failed", new_pl_file);
            MYMPD_LOG_ERRNO(errno);
        }
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Renaming playlist failed");
        FREE_SDS(old_pl_file);
        FREE_SDS(new_pl_file);
        return buffer;
    }
    FREE_SDS(old_pl_file);
    FREE_SDS(new_pl_file);
    //rename mpd playlist
    bool rc = mpd_run_rename(mympd_state->mpd_state->conn, old_playlist, new_playlist);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_run_rename") == true) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "playlist", "info", "Successfully renamed playlist");
    }
    return buffer;
}

sds mympd_api_playlist_delete(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *playlist, bool smartpls_only)
{
    //remove smart playlist
    sds pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->workdir, playlist);
    errno = 0;
    int rc = unlink(pl_file);
    if (rc == -1 &&
        errno != ENOENT)
    {
        //ignores none existing smart playlist
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Deleting smart playlist failed");
        MYMPD_LOG_ERROR("Deleting smart playlist \"%s\" failed", playlist);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(pl_file);
        return buffer;
    }
    FREE_SDS(pl_file);
    if (smartpls_only == true) {
        send_jsonrpc_event("update_stored_playlist");
        buffer = jsonrpc_respond_ok(buffer, method, request_id, "playlist");
        return buffer;
    }
    //remove mpd playlist
    bool rc2 = mpd_run_rm(mympd_state->mpd_state->conn, playlist);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc2, "mpd_run_rm") == true) {
        buffer = jsonrpc_respond_ok(buffer, method, request_id, "playlist");
    }
    return buffer;
}

sds mympd_api_smartpls_put(struct t_config *config, sds buffer, sds method, long request_id,
                            const char *playlist)
{
    sds pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", config->workdir, playlist);
    FILE *fp = fopen(pl_file, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Cant read smart playlist \"%s\"", playlist);
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Can not read smart playlist file");
        FREE_SDS(pl_file);
        return buffer;
    }
    sds content = sdsempty();
    sds_getfile(&content, fp, 2000);
    FREE_SDS(pl_file);
    (void) fclose(fp);

    sds smartpltype = NULL;
    sds sds_buf1 = NULL;
    int int_buf1 = 0;
    int int_buf2 = 0;

    if (json_get_string(content, "$.type", 1, 200, &smartpltype, vcb_isalnum, NULL) == true) {
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = tojson_char(buffer, "plist", playlist, true);
        buffer = tojson_char(buffer, "type", smartpltype, true);
        bool rc = true;
        if (strcmp(smartpltype, "sticker") == 0) {
            if (json_get_string(content, "$.sticker", 1, 200, &sds_buf1, vcb_isalnum, NULL) == true &&
                json_get_int(content, "$.maxentries", 0, MPD_PLAYLIST_LENGTH_MAX, &int_buf1, NULL) == true &&
                json_get_int(content, "$.minvalue", 0, 100, &int_buf2, NULL) == true)
            {
                buffer = tojson_char(buffer, "sticker", sds_buf1, true);
                buffer = tojson_long(buffer, "maxentries", int_buf1, true);
                buffer = tojson_long(buffer, "minvalue", int_buf2, true);
            }
            else {
                rc = false;
            }
        }
        else if (strcmp(smartpltype, "newest") == 0) {
            if (json_get_int(content, "$.timerange", 0, JSONRPC_INT_MAX, &int_buf1, NULL) == true) {
                buffer = tojson_long(buffer, "timerange", int_buf1, true);
            }
            else {
                rc = false;
            }
        }
        else if (strcmp(smartpltype, "search") == 0) {
            if (json_get_string(content, "$.expression", 1, 200, &sds_buf1, vcb_isname, NULL) == true) {
                buffer = tojson_char(buffer, "expression", sds_buf1, true);
            }
            else {
                rc = false;
            }
        }
        else {
            rc = false;
        }
        if (rc == true) {
            FREE_SDS(sds_buf1);
            if (json_get_string(content, "$.sort", 0, 100, &sds_buf1, vcb_ismpdsort, NULL) == true) {
                buffer = tojson_char(buffer, "sort", sds_buf1, false);
            }
            else {
                buffer = tojson_char(buffer, "sort", "", false);
            }
            buffer = jsonrpc_result_end(buffer);
        }
        else {
            buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Can not parse smart playlist file");
            MYMPD_LOG_ERROR("Can't parse smart playlist file: %s", playlist);
        }
    }
    else {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Unknown smart playlist type");
        MYMPD_LOG_ERROR("Unknown type for smart playlist \"%s\"", playlist);
    }
    FREE_SDS(smartpltype);
    FREE_SDS(content);
    FREE_SDS(sds_buf1);
    return buffer;
}

sds mympd_api_playlist_delete_all(struct t_mympd_state *mympd_state, sds buffer, sds method,
                                   long request_id, const char *type)
{
    bool rc = mpd_send_list_playlists(mympd_state->mpd_state->conn);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_playlists") == false) {
        return buffer;
    }

    struct t_list playlists;
    list_init(&playlists);
    //get all mpd playlists
    struct mpd_playlist *pl;
    while ((pl = mpd_recv_playlist(mympd_state->mpd_state->conn)) != NULL) {
        const char *plpath = mpd_playlist_get_path(pl);
        list_push(&playlists, plpath, 1, NULL, NULL);
        mpd_playlist_free(pl);
    }
    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        list_clear(&playlists);
        return buffer;
    }
    //delete each smart playlist file that have no corresponding mpd playlist file
    sds smartpls_path = sdscatfmt(sdsempty(), "%s/smartpls", mympd_state->config->workdir);
    errno = 0;
    DIR *smartpls_dir = opendir(smartpls_path);
    if (smartpls_dir != NULL) {
        struct dirent *next_file;
        while ((next_file = readdir(smartpls_dir)) != NULL ) {
            if (next_file->d_type == DT_REG) {
                if (list_get_node(&playlists, next_file->d_name) == NULL) {
                    sds smartpls_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->workdir, next_file->d_name);
                    errno = 0;
                    if (unlink(smartpls_file) == 0) {
                        MYMPD_LOG_INFO("Removed orphaned smartpls file \"%s\"", smartpls_file);
                    }
                    else {
                        MYMPD_LOG_ERROR("Error removing file \"%s\"", smartpls_file);
                        MYMPD_LOG_ERRNO(errno);
                    }
                    FREE_SDS(smartpls_file);
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

    if (strcmp(type, "deleteEmptyPlaylists") == 0) {
        struct t_list_node *current = playlists.head;
        while (current != NULL) {
            current->value_i = mympd_api_enum_playlist(mympd_state, current->key, true);
            current = current->next;
        }
    }

    if (mpd_command_list_begin(mympd_state->mpd_state->conn, false)) {
        struct t_list_node *current;
        while ((current = list_shift_first(&playlists)) != NULL) {
            bool smartpls = false;
            if (strcmp(type, "deleteSmartPlaylists") == 0) {
                sds smartpls_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->workdir, current->key);
                if (unlink(smartpls_file) == 0) {
                    MYMPD_LOG_INFO("Smartpls file %s removed", smartpls_file);
                    smartpls = true;
                }
                FREE_SDS(smartpls_file);
            }
            if (strcmp(type, "deleteAllPlaylists") == 0 ||
                (strcmp(type, "deleteSmartPlaylists") == 0 && smartpls == true) ||
                (strcmp(type, "deleteEmptyPlaylists") == 0 && current->value_i == 0))
            {
                rc = mpd_send_rm(mympd_state->mpd_state->conn, current->key);
                if (rc == false) {
                    MYMPD_LOG_ERROR("Error adding command to command list mpd_send_rm");
                    break;
                }
                MYMPD_LOG_INFO("Deleting mpd playlist %s", current->key);
            }
            list_node_free(current);
        }
        if (mpd_command_list_end(mympd_state->mpd_state->conn)) {
            mpd_response_finish(mympd_state->mpd_state->conn);
        }
        if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
            list_clear(&playlists);
            return buffer;
        }
    }
    else if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        list_clear(&playlists);
        return buffer;
    }
    list_clear(&playlists);
    buffer = jsonrpc_respond_message(buffer, method, request_id, false, "playlist", "info", "Playlists deleted");
    return buffer;
}

//private functions
static int mympd_api_enum_playlist(struct t_mympd_state *mympd_state, const char *playlist, bool empty_check) {
    bool rc = mpd_send_list_playlist(mympd_state->mpd_state->conn, playlist);
    if (check_rc_error_and_recover(mympd_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_playlist") == false) {
        return -1;
    }

    struct mpd_song *song;
    int entity_count = 0;
    while ((song = mpd_recv_song(mympd_state->mpd_state->conn)) != NULL) {
        entity_count++;
        mpd_song_free(song);
        if (empty_check == true) {
            break;
        }
    }
    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, NULL, NULL, 0, false) == false) {
        return -1;
    }
    return entity_count;
}

static bool smartpls_init(struct t_config *config, const char *name, const char *value) {
    sds tmp_file = sdscatfmt(sdsempty(), "%s/smartpls/%s.XXXXXX", config->workdir, name);
    errno = 0;
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write", tmp_file);
        MYMPD_LOG_ERRNO(errno);
        FREE_SDS(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    bool rc = true;
    if (fputs(value, fp) == EOF) {
        MYMPD_LOG_ERROR("Could not write initial smart playlist");
        rc = false;
    }
    if (fclose(fp) != 0) {
        MYMPD_LOG_ERROR("Could not close file \"%s\"", tmp_file);
        rc = false;
    }
    sds cfg_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", config->workdir, name);
    errno = 0;
    if (rc == true) {
        if (rename(tmp_file, cfg_file) == -1) {
            MYMPD_LOG_ERROR("Renaming file from %s to %s failed", tmp_file, cfg_file);
            MYMPD_LOG_ERRNO(errno);
            rc = false;
        }
    }
    else {
        //remove incomplete tmp file
        if (unlink(tmp_file) != 0) {
            MYMPD_LOG_ERROR("Could not remove incomplete tmp file \"%s\"", tmp_file);
            MYMPD_LOG_ERRNO(errno);
            rc = false;
        }
    }
    FREE_SDS(tmp_file);
    FREE_SDS(cfg_file);
    return rc;
}
