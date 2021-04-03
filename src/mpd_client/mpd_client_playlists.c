/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <inttypes.h>
#include <ctype.h>
#include <signal.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../dist/src/rax/rax.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../api.h"
#include "../list.h"
#include "mympd_config_defs.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "../utility.h"
#include "../log.h"
#include "../mympd_state.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_search.h"
#include "mpd_client_utility.h"
#include "mpd_client_playlists.h"

//private definitions
static int mpd_client_enum_playlist(struct t_mympd_state *mympd_state, const char *playlist, bool empty_check);
static bool smartpls_init(struct t_config *config, const char *name, const char *value);

//public functions
bool smartpls_default(struct t_config *config) {
    bool rc = true;

    //try to get prefix from state file, fallback to config value
    sds prefix = sdsempty();
    sds prefix_file = sdscatfmt(sdsempty(), "%s/state/smartpls_prefix", config->varlibdir);
    FILE *fp = fopen(prefix_file, "r");
    if (fp != NULL) {
        size_t n = 0;
        char *line = NULL;
        ssize_t read = getline(&line, &n, fp);
        if (read > 0) {
            prefix = sdscat(prefix, line);
            FREE_PTR(line);
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
    t_work_request *request = create_request(-1, 0, MPDWORKER_API_SMARTPLS_UPDATE, "MPDWORKER_API_SMARTPLS_UPDATE", "");
    request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MPDWORKER_API_SMARTPLS_UPDATE\",\"params\":{");
    request->data = tojson_char(request->data, "playlist", playlist, false);
    request->data = sdscat(request->data, "}}");
    tiny_queue_push(mpd_worker_queue, request, 0);
}

void mpd_client_smartpls_update_all(void) {
    t_work_request *request = create_request(-1, 0, MPDWORKER_API_SMARTPLS_UPDATE_ALL, "MPDWORKER_API_SMARTPLS_UPDATE_ALL", "");
    request->data = sdscat(request->data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MPDWORKER_API_SMARTPLS_UPDATE_ALL\",\"params\":{}}");
    tiny_queue_push(mpd_worker_queue, request, 0);
}

sds mpd_client_put_playlists(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                             const unsigned int offset, const unsigned int limit, const char *searchstr) 
{
    bool rc = mpd_send_list_playlists(mympd_state->mpd_state->conn);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_playlists") == false) {
        return buffer;
    }

    struct mpd_playlist *pl;
    struct list entity_list;
    list_init(&entity_list);
    size_t search_len = strlen(searchstr);
    while ((pl = mpd_recv_playlist(mympd_state->mpd_state->conn)) != NULL) {
        const char *plpath = mpd_playlist_get_path(pl);
        if (search_len == 0  || strcasestr(plpath, searchstr) != NULL) {
            list_push(&entity_list, plpath, mpd_playlist_get_last_modified(pl), NULL, NULL);
        }
        mpd_playlist_free(pl);
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
    struct list_node *current = entity_list.head;
    while (current != NULL) {
        entity_count++;
        if (entity_count > offset && (entity_count <= offset + limit || limit == 0)) {
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
                                 const char *uri, const unsigned int offset, const unsigned int limit, const char *searchstr, const struct t_tags *tagcols)
{
    bool rc = mpd_send_list_playlist_meta(mympd_state->mpd_state->conn, uri);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_playlist_meta") == false) {
        return buffer;
    }
    
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer,"\"data\":[");

    struct mpd_song *song;
    unsigned entities_returned = 0;
    unsigned entity_count = 0;
    unsigned total_time = 0;
    sds entityName = sdsempty();
    size_t search_len = strlen(searchstr);
    while ((song = mpd_recv_song(mympd_state->mpd_state->conn)) != NULL) {
        entity_count++;
        total_time += mpd_song_get_duration(song);
        if (entity_count > offset && (entity_count <= offset + limit || limit == 0)) {
            entityName = mpd_shared_get_tags(song, MPD_TAG_TITLE, entityName);
            if (search_len == 0  || strcasestr(entityName, searchstr) != NULL) {
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
    if (validate_string_not_dir(old_playlist) == false || validate_string_not_dir(new_playlist) == false) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Invalid filename");
        return buffer;
    }
    sds old_pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->varlibdir, old_playlist);
    sds new_pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->varlibdir, new_playlist);
    if (access(old_pl_file, F_OK ) != -1) { /* Flawfinder: ignore */
        //smart playlist
        if (access(new_pl_file, F_OK ) == -1) { /* Flawfinder: ignore */
            //new playlist doesn't exist
            if (rename(old_pl_file, new_pl_file) == -1) {
                MYMPD_LOG_ERROR("Renaming smart playlist %s to %s failed: %s", old_pl_file, new_pl_file, strerror(errno));
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
        buffer = jsonrpc_respond_message(buffer, method, request_id, false, "playlist", "info", "Sucessfully renamed playlist");
    }
    return buffer;
}

sds mpd_client_playlist_delete(struct t_mympd_state *mympd_state, sds buffer, sds method,
                               long request_id, const char *playlist) {
    if (validate_string_not_dir(playlist) == false) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Invalid filename");
        return buffer;
    }
    //remove smart playlist
    sds pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->varlibdir, playlist);
    int rc = unlink(pl_file);
    if (rc == -1 && errno != ENOENT) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Deleting smart playlist failed");
        MYMPD_LOG_ERROR("Deleting smart playlist \"%s\" failed: %s", playlist, strerror(errno));
        sdsfree(pl_file);
        return buffer;
    }
    //ignore error
    MYMPD_LOG_DEBUG("Error removing file \"%s\": %s", pl_file, strerror(errno));
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
    if (validate_string_not_dir(playlist) == false) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Can not read smart playlist file");
        return buffer;
    }
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    int int_buf1 = 0;
    int int_buf2 = 0;

    sds pl_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", config->varlibdir, playlist);
    char *content = json_fread(pl_file);
    sdsfree(pl_file);
    if (content == NULL) {
        MYMPD_LOG_ERROR("Can't read smart playlist: %s", playlist);
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Can not read smart playlist file");
        return buffer;
    }
    char *smartpltype = NULL;
    int je = json_scanf(content, (int)strlen(content), "{type: %Q }", &smartpltype);
    if (je == 1) {
        buffer = jsonrpc_result_start(buffer, method, request_id);
        buffer = tojson_char(buffer, "playlist", playlist, true);
        buffer = tojson_char(buffer, "type", smartpltype, true);
        bool rc = true;
        if (strcmp(smartpltype, "sticker") == 0) {
            je = json_scanf(content, (int)strlen(content), "{sticker: %Q, maxentries: %d, minvalue: %d}", &p_charbuf1, &int_buf1, &int_buf2);
            if (je == 3) {
                buffer = tojson_char(buffer, "sticker", p_charbuf1, true);
                buffer = tojson_long(buffer, "maxentries", int_buf1, true);
                buffer = tojson_long(buffer, "minvalue", int_buf2, true);
            }
            else if (je == 2) {
                //only for backward compatibility
                buffer = tojson_char(buffer, "sticker", p_charbuf1, true);
                buffer = tojson_long(buffer, "maxentries", int_buf1, true);
                buffer = tojson_long(buffer, "minvalue", 1, true);
            }
            else {
                rc = false;
            }
            FREE_PTR(p_charbuf1);
        }
        else if (strcmp(smartpltype, "newest") == 0) {
            je = json_scanf(content, (int)strlen(content), "{timerange: %d}", &int_buf1);
            if (je == 1) {
                buffer = tojson_long(buffer, "timerange", int_buf1, true);
            }
            else {
                rc = false;
            }
        }
        else if (strcmp(smartpltype, "search") == 0) {
            je = json_scanf(content, (int)strlen(content), "{tag: %Q, searchstr: %Q}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                buffer = tojson_char(buffer, "tag", p_charbuf1, true);
                buffer = tojson_char(buffer, "searchstr", p_charbuf2, true);
            }
            else {
                rc = false;    
            }
            FREE_PTR(p_charbuf1);
            FREE_PTR(p_charbuf2);
        }
        else {
            rc = false;            
        }
        if (rc == true) {
            je = json_scanf(content, (int)strlen(content), "{sort: %Q}", &p_charbuf1);
            if (je == 1) {
                buffer = tojson_char(buffer, "sort", p_charbuf1, false);
            }
            else {
                buffer = tojson_char(buffer, "sort", "", false);
            }
            FREE_PTR(p_charbuf1);            
            buffer = jsonrpc_result_end(buffer);
        }
        else {
            buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Can not parse smart playlist file");
            MYMPD_LOG_ERROR("Can't parse smart playlist file: %s", playlist);
        }
        FREE_PTR(smartpltype);        
    }
    else {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "playlist", "error", "Unknown smart playlist type");
        MYMPD_LOG_ERROR("Unknown smart playlist type: %s", playlist);
    }
    FREE_PTR(content);
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
    sds smartpls_path = sdscatfmt(sdsempty(), "%s/smartpls", mympd_state->config->varlibdir);
    DIR *smartpls_dir = opendir(smartpls_path);
    if (smartpls_dir != NULL) {
        struct dirent *next_file;
        while ((next_file = readdir(smartpls_dir)) != NULL ) {
            if (strncmp(next_file->d_name, ".", 1) != 0) {
                if (list_get_node(&playlists, next_file->d_name) == NULL) {
                    sds smartpls_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->varlibdir, next_file->d_name);
                    if (unlink(smartpls_file) == 0) {
                        MYMPD_LOG_INFO("Removed orphaned smartpls file \"%s\"", smartpls_file);
                    }
                    else {
                        MYMPD_LOG_ERROR("Error removing file \"%s\": %s", smartpls_file, strerror(errno));
                    }
                    sdsfree(smartpls_file);
                }
            }
        }
        closedir(smartpls_dir);
    }
    else {
        MYMPD_LOG_ERROR("Can not open smartpls dir \"%s\": %s", smartpls_path, strerror(errno));
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
                sds smartpls_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", mympd_state->config->varlibdir, current->key);
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
    if (!validate_string(name)) {
        return false;
    }
    
    sds tmp_file = sdscatfmt(sdsempty(), "%s/smartpls/%s.XXXXXX", config->varlibdir, name);
    int fd = mkstemp(tmp_file);
    if (fd < 0 ) {
        MYMPD_LOG_ERROR("Can not open file \"%s\" for write: %s", tmp_file, strerror(errno));
        sdsfree(tmp_file);
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    fprintf(fp, "%s", value);
    fclose(fp);
    sds cfg_file = sdscatfmt(sdsempty(), "%s/smartpls/%s", config->varlibdir, name);
    if (rename(tmp_file, cfg_file) == -1) {
        MYMPD_LOG_ERROR("Renaming file from %s to %s failed: %s", tmp_file, cfg_file, strerror(errno));
        sdsfree(tmp_file);
        sdsfree(cfg_file);
        return false;
    }
    sdsfree(tmp_file);
    sdsfree(cfg_file);
    return true;
}
