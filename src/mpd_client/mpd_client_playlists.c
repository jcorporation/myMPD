/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mpd_client_playlists.h"

#include "../lib/api.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../lib/validate.h"
#include "../mpd_shared/mpd_shared_search.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "mpd_client_utility.h"

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//private definitions
static int mpd_client_enum_playlist(struct t_mympd_state *mympd_state, const char *playlist, bool empty_check);
static bool smartpls_init(struct t_config *config, const char *name, const char *value);

//public functions
bool smartpls_default(struct t_config *config) {
    bool rc = true;

    //try to get prefix from state file, fallback to default value
    sds prefix = sdsempty();
    sds prefix_file = sdscatfmt(sdsempty(), "%s/state/smartpls_prefix", config->workdir);
    FILE *fp = fopen(prefix_file, OPEN_FLAGS_READ);
    if (fp != NULL) {
        if (sdsgetline(&prefix, fp, 50) != 0) {
            prefix = sdscat(prefix, "myMPDsmart");
        }
        fclose(fp);
    }
    else {
        prefix = sdscat(prefix, "myMPDsmart");
    }
    sdsfree(prefix_file);
    
    sds smartpls_file = sdscatfmt(sdsempty(), "%s%sbestRated", prefix, (sdslen(prefix) > 0 ? "-" : ""));
    rc = smartpls_init(config, smartpls_file, 
        "{\"type\": \"sticker\", \"sticker\": \"like\", \"maxentries\": 200, \"minvalue\": 2, \"sort\": \"\"}");
    if (rc == false) {
        sdsfree(smartpls_file);
        sdsfree(prefix);
        return rc;
    }
    
    sdscrop(smartpls_file);
    smartpls_file = sdscatfmt(smartpls_file, "%s%smostPlayed", prefix, (sdslen(prefix) > 0 ? "-" : ""));
    rc = smartpls_init(config, smartpls_file, 
        "{\"type\": \"sticker\", \"sticker\": \"playCount\", \"maxentries\": 200, \"minvalue\": 0, \"sort\": \"\"}");
    if (rc == false) {
        sdsfree(smartpls_file);
        sdsfree(prefix);
        return rc;
    }
    
    sdscrop(smartpls_file);
    smartpls_file = sdscatfmt(smartpls_file, "%s%snewestSongs", prefix, (sdslen(prefix) > 0 ? "-" : ""));
    rc = smartpls_init(config, smartpls_file, 
        "{\"type\": \"newest\", \"timerange\": 604800, \"sort\": \"\"}");
    sdsfree(smartpls_file);
    sdsfree(prefix);
    
    return rc;
}

void mpd_client_smartpls_update(const char *playlist) {
    t_work_request *request = create_request(-1, 0, MYMPD_API_SMARTPLS_UPDATE, NULL);
    request->data = tojson_char(request->data, "plist", playlist, false);
    request->data = sdscat(request->data, "}}");
    tiny_queue_push(mympd_api_queue, request, 0);
}

void mpd_client_smartpls_update_all(void) {
    t_work_request *request = create_request(-1, 0, MYMPD_API_SMARTPLS_UPDATE_ALL, NULL);
    request->data = sdscat(request->data, "}}");
    tiny_queue_push(mympd_api_queue, request, 0);
}

sds mpd_client_put_playlists(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                             const unsigned int offset, const unsigned int limit, sds searchstr) 
{
    bool rc = mpd_send_list_playlists(mympd_state->mpd_state->conn);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_playlists") == false) {
        return buffer;
    }

    sdstolower(searchstr);

    struct mpd_playlist *pl;
    struct list entity_list;
    list_init(&entity_list);
    size_t search_len = strlen(searchstr);
    while ((pl = mpd_recv_playlist(mympd_state->mpd_state->conn)) != NULL) {
        const char *plpath = mpd_playlist_get_path(pl);
        sds plpath_lower = sdsnew(plpath);
        sdstolower(plpath_lower);
        if (search_len == 0  || strstr(plpath_lower, searchstr) != NULL) {
            list_push(&entity_list, plpath, mpd_playlist_get_last_modified(pl), NULL, NULL);
        }
        mpd_playlist_free(pl);
        sdsfree(plpath_lower);
    }
    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    list_sort_by_key(&entity_list, true);
    
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer,"\"data\":[");

    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    unsigned real_limit = limit == 0 ? offset + MAX_MPD_RESULTS : offset + limit;
    struct list_node *current = entity_list.head;
    while (current != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= real_limit) {
            if (entities_returned++) {
                buffer = sdscat(buffer,",");
            }
            bool smartpls = is_smartpls(mympd_state, current->key);
            buffer = sdscat(buffer, "{");
            buffer = tojson_char(buffer, "Type", (smartpls == true ? "smartpls" : "plist"), true);
            buffer = tojson_char(buffer, "uri", current->key, true);
            buffer = tojson_char(buffer, "name", current->key, true);
            buffer = tojson_long(buffer, "last_modified", current->value_i, false);
            buffer = sdscat(buffer, "}");
        }
        current = current->next;
    }
    list_free(&entity_list);
    buffer = sdscat(buffer, "],");
    buffer = tojson_char(buffer, "searchstr", searchstr, true);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, false);
    buffer = jsonrpc_result_end(buffer);
    
    return buffer;
}

sds mpd_client_put_playlist_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                                 sds uri, const unsigned int offset, const unsigned int limit, sds searchstr, const struct t_tags *tagcols)
{
    bool rc = mpd_send_list_playlist_meta(mympd_state->mpd_state->conn, uri);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_playlist_meta") == false) {
        return buffer;
    }

    sdstolower(searchstr);
   
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer,"\"data\":[");

    struct mpd_song *song;
    unsigned entities_returned = 0;
    unsigned entity_count = 0;
    unsigned total_time = 0;
    unsigned real_limit = limit == 0 ? offset + MAX_MPD_RESULTS : offset + limit;
    sds entityName = sdsempty();
    size_t search_len = strlen(searchstr);
    while ((song = mpd_recv_song(mympd_state->mpd_state->conn)) != NULL) {
        entity_count++;
        total_time += mpd_song_get_duration(song);
        if (entity_count > offset && entity_count <= real_limit) {
            entityName = mpd_shared_get_tags(song, MPD_TAG_TITLE, entityName);
            sdstolower(entityName);
            if (search_len == 0  || strstr(entityName, searchstr) != NULL) {
                if (entities_returned++) {
                    buffer= sdscat(buffer, ",");
                }
                buffer = sdscat(buffer, "{");
                buffer = tojson_char(buffer, "Type", "song", true);
                buffer = tojson_long(buffer, "Pos", entity_count, true);
                buffer = put_song_tags(buffer, mympd_state->mpd_state, tagcols, song);
                buffer = sdscat(buffer, "}");
            }
            else {
                entity_count--;
            }
        }
        mpd_song_free(song);
    }
    sdsfree(entityName);
    
    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    
    bool smartpls = is_smartpls(mympd_state, uri);

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "totalTime", total_time, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_char(buffer, "searchstr", searchstr, true);
    buffer = tojson_char(buffer, "uri", uri, true);
    buffer = tojson_bool(buffer, "smartpls", smartpls, false);
    buffer = jsonrpc_result_end(buffer);
    
    return buffer;
}

sds mpd_client_playlist_rename(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *old_playlist, const char *new_playlist)
{
    sds old_pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->workdir, old_playlist);
    sds new_pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->workdir, new_playlist);
    if (access(old_pl_file, F_OK ) != -1) { /* Flawfinder: ignore */
        //smart playlist
        if (access(new_pl_file, F_OK ) == -1) { /* Flawfinder: ignore */
            //new playlist doesn't exist
            errno = 0;
            if (rename(old_pl_file, new_pl_file) == -1) {
                MYMPD_LOG_ERROR("Renaming smart playlist %s to %s failed", old_pl_file, new_pl_file);
                MYMPD_LOG_ERRNO(errno);
                buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Renaming playlist failed");
                sdsfree(old_pl_file);
                sdsfree(new_pl_file);
                return buffer;
            }
        } 
    }
    sdsfree(old_pl_file);
    sdsfree(new_pl_file);
    //rename mpd playlist
    bool rc = mpd_run_rename(mympd_state->mpd_state->conn, old_playlist, new_playlist);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_run_rename") == true) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "playlist", "info", "Successfully renamed playlist");
    }
    return buffer;
}

sds mpd_client_playlist_delete(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *playlist)
{
    //remove smart playlist
    sds pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->workdir, playlist);
    errno = 0;
    int rc = unlink(pl_file);
    if (rc == -1 && errno != ENOENT) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Deleting smart playlist failed");
        MYMPD_LOG_ERROR("Deleting smart playlist \"%s\" failed", playlist);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(pl_file);
        return buffer;
    }
    //ignore error
    MYMPD_LOG_DEBUG("File \"%s\" does not exist", pl_file);
    sdsfree(pl_file);
    //remove mpd playlist
    bool rc2 = mpd_run_rm(mympd_state->mpd_state->conn, playlist);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc2, "mpd_run_rm") == true) {
        buffer = jsonrpc_respond_ok(buffer, method, request_id, "playlist");
    }
    return buffer;
}

sds mpd_client_smartpls_put(struct t_config *config, sds buffer, sds method, long request_id,
                            const char *playlist)
{
    sds smartpltype = NULL;
    sds sds_buf1 = NULL;
    int int_buf1 = 0;
    int int_buf2 = 0;

    sds pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", config->workdir, playlist);
    FILE *fp = fopen(pl_file, OPEN_FLAGS_READ);
    if (fp == NULL) {
        MYMPD_LOG_ERROR("Cant read smart playlist \"%s\"", playlist);
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Can not read smart playlist file");
        return buffer;
    }
    sds content = sdsempty();
    sdsgetfile(&content, fp, 2000);
    sdsfree(pl_file);

    if (json_get_string(content, "$.type", 1, 200, &smartpltype, vcb_isalnum, NULL) != true) {
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = tojson_char(buffer, "plist", playlist, true);
        buffer = tojson_char(buffer, "type", smartpltype, true);
        bool rc = true;
        if (strcmp(smartpltype, "sticker") == 0) {
            if (json_get_string(content, "$.params.sticker", 1, 200, &sds_buf1, vcb_isalnum, NULL) == true &&
                json_get_int(content, "$.params.maxentries", 0, MAX_MPD_PLAYLIST_LENGTH, &int_buf1, NULL) == true &&
                json_get_int(content, "$.params.minvalue", 0, 100, &int_buf2, NULL) == true)
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
            if (json_get_int(content, "$.params.timerange", 0, JSONRPC_INT_MAX, &int_buf1, NULL) == true) {
                buffer = tojson_long(buffer, "timerange", int_buf1, true);
            }
            else {
                rc = false;
            }
        }
        else if (strcmp(smartpltype, "search") == 0) {
            if (json_get_string(content, "$.params.expression", 1, 200, &sds_buf1, vcb_isname, NULL) == true) {
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
            if (json_get_string(content, "$.params.sort", 0, 100, &sds_buf1, vcb_ismpdsort, NULL) == true) {
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
        MYMPD_LOG_ERROR("Unknown smart playlist type: %s", playlist);
    }
    FREE_SDS(smartpltype);
    FREE_SDS(content);
    FREE_SDS(sds_buf1);
    return buffer;
}

sds mpd_client_playlist_delete_all(struct t_mympd_state *mympd_state, sds buffer, sds method,
                                   long request_id, const char *type)
{
    bool rc = mpd_send_list_playlists(mympd_state->mpd_state->conn);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_playlists") == false) {
        return buffer;
    }
    
    struct list playlists;
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
        list_free(&playlists);
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
                    sdsfree(smartpls_file);
                }
            }
        }
        closedir(smartpls_dir);
    }
    else {
        MYMPD_LOG_ERROR("Can not open smartpls dir \"%s\"", smartpls_path);
        MYMPD_LOG_ERRNO(errno);
    }
    sdsfree(smartpls_path);
    
    if (strcmp(type, "deleteEmptyPlaylists") == 0) {
        struct list_node *current = playlists.head;
        while (current != NULL) {
            current->value_i = mpd_client_enum_playlist(mympd_state, current->key, true);
            current = current->next;
        }
    }

    if (mpd_command_list_begin(mympd_state->mpd_state->conn, false)) {
        struct list_node *current = playlists.head;
        while (current != NULL) {
            bool smartpls = false;
            if (strcmp(type, "deleteSmartPlaylists") == 0) {
                sds smartpls_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->workdir, current->key);
                if (unlink(smartpls_file) == 0) {
                    MYMPD_LOG_INFO("Smartpls file %s removed", smartpls_file);
                    smartpls = true;
                }
                sdsfree(smartpls_file);
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
            current = current->next;        
        }
        if (mpd_command_list_end(mympd_state->mpd_state->conn)) {
            mpd_response_finish(mympd_state->mpd_state->conn);
        }
        if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
            list_free(&playlists);
            return buffer;
        }
    }
    else if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        list_free(&playlists);
        return buffer;
    }
    list_free(&playlists);
    buffer = jsonrpc_respond_message(buffer, method, request_id, false, "playlist", "info", "Playlists deleted");
    return buffer;
}

//private functions
static int mpd_client_enum_playlist(struct t_mympd_state *mympd_state, const char *playlist, bool empty_check) {
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
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    fprintf(fp, "%s", value);
    fclose(fp);
    sds cfg_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", config->workdir, name);
    errno = 0;
    if (rename(tmp_file, cfg_file) == -1) {
        MYMPD_LOG_ERROR("Renaming file from %s to %s failed", tmp_file, cfg_file);
        MYMPD_LOG_ERRNO(errno);
        sdsfree(tmp_file);
        sdsfree(cfg_file);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(cfg_file);
    return true;
}
