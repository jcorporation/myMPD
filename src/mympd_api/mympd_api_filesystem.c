/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_filesystem.h"

#include "../../dist/utf8/utf8.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/sds_extras.h"
#include "../lib/utility.h"
#include "../mpd_client/mpd_client_errorhandler.h"
#include "../mpd_client/mpd_client_playlists.h"
#include "../mpd_client/mpd_client_tags.h"
#include "mympd_api_extra_media.h"
#include "mympd_api_sticker.h"

#include <libgen.h>
#include <string.h>

//private definitions

struct t_dir_entry {
    sds name;
    struct mpd_entity *entity;
};

static bool search_dir_entry(rax *rt, sds key, sds entity_name, struct mpd_entity *entity, sds searchstr);

//public functions

sds mympd_api_browse_filesystem(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                              sds path, const long offset, const long limit, sds searchstr, const struct t_tags *tagcols)
{
    bool rc = mpd_send_list_meta(mympd_state->mpd_state->conn, path);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_meta") == false) {
        return buffer;
    }
    sds key = sdsempty();
    rax *entity_list = raxNew();
    long real_limit = offset + limit;
    struct mpd_entity *entity;
    while ((entity = mpd_recv_entity(mympd_state->mpd_state->conn)) != NULL) {
        switch (mpd_entity_get_type(entity)) {
            case MPD_ENTITY_TYPE_SONG: {
                const struct mpd_song *song = mpd_entity_get_song(entity);
                sds entity_name =  mpd_client_get_tag_value_string(song, MPD_TAG_TITLE, sdsempty());
                key = sdscatfmt(key, "2%s", mpd_song_get_uri(song));
                search_dir_entry(entity_list, key, entity_name, entity, searchstr);
                break;
            }
            case MPD_ENTITY_TYPE_DIRECTORY: {
                const struct mpd_directory *dir = mpd_entity_get_directory(entity);
                sds entity_name = sdsnew(mpd_directory_get_path(dir));
                basename_uri(entity_name);
                key = sdscatfmt(key, "0%s", mpd_directory_get_path(dir));
                search_dir_entry(entity_list, key, entity_name, entity, searchstr);
                break;
            }
            case MPD_ENTITY_TYPE_PLAYLIST: {
                const struct mpd_playlist *pl = mpd_entity_get_playlist(entity);
                const char *pl_path = mpd_playlist_get_path(pl);
                if (path[0] == '/') {
                    //do not show mpd playlists in root directory
                    const char *ext = get_extension_from_filename(pl_path);
                    if (ext == NULL ||
                        (strcasecmp(ext, "m3u") != 0 && strcasecmp(ext, "pls") != 0))
                    {
                        mpd_entity_free(entity);
                        break;
                    }
                }
                sds entity_name = sdsnew(pl_path);
                basename_uri(entity_name);
                key = sdscatfmt(key, "1%s", pl_path);
                search_dir_entry(entity_list, key, entity_name, entity, searchstr);
                break;
            }
            default: {
                mpd_entity_free(entity);
            }
        }
        sdsclear(key);
    }
    FREE_SDS(key);
    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        //free result
        raxIterator iter;
        raxStart(&iter, entity_list);
        raxSeek(&iter, "^", NULL, 0);
        while (raxNext(&iter)) {
            struct t_dir_entry *entry_data = (struct t_dir_entry *)iter.data;
            mpd_entity_free(entry_data->entity);
            FREE_SDS(entry_data->name);
            FREE_PTR(iter.data);
        }
        raxStop(&iter);
        raxFree(entity_list);
        //return error message
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");

    long entity_count = 0;
    long entities_returned = 0;
    if (sdslen(path) > 1) {
        char *path_cpy = strdup(path);
        char *parent_dir = dirname(path_cpy);
        buffer = sdscat(buffer, "{\"Type\":\"parentDir\",\"name\":\"parentDir\",");
        buffer = tojson_char(buffer, "uri", (parent_dir[0] == '.' ? "" : parent_dir), false);
        buffer = sdscatlen(buffer, "}", 1);
        entity_count++;
        entities_returned++;
        FREE_PTR(path_cpy);
    }

    raxIterator iter;
    raxStart(&iter, entity_list);
    raxSeek(&iter, "^", NULL, 0);
    while (raxNext(&iter)) {
        struct t_dir_entry *entry_data = (struct t_dir_entry *)iter.data;
        if (entity_count >= offset &&
            entity_count < real_limit)
        {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            switch (mpd_entity_get_type(entry_data->entity)) {
                case MPD_ENTITY_TYPE_SONG: {
                    const struct mpd_song *song = mpd_entity_get_song(entry_data->entity);
                    buffer = sdscat(buffer, "{\"Type\":\"song\",");
                    buffer = get_song_tags(buffer, mympd_state->mpd_state, tagcols, song);
                    buffer = sdscatlen(buffer, ",", 1);
                    sds filename = sdsnew(mpd_song_get_uri(song));
                    basename_uri(filename);
                    buffer = tojson_sds(buffer, "Filename", filename, false);
                    FREE_SDS(filename);
                    if (mympd_state->mpd_state->feat_mpd_stickers) {
                        buffer = sdscatlen(buffer, ",", 1);
                        buffer = mympd_api_sticker_list(buffer, mympd_state->sticker_cache, mpd_song_get_uri(song));
                    }
                    buffer = sdscatlen(buffer, "}", 1);
                    break;
                }
                case MPD_ENTITY_TYPE_DIRECTORY: {
                    const struct mpd_directory *dir = mpd_entity_get_directory(entry_data->entity);
                    buffer = sdscat(buffer, "{\"Type\":\"dir\",");
                    buffer = tojson_char(buffer, "uri", mpd_directory_get_path(dir), true);
                    buffer = tojson_sds(buffer, "name", entry_data->name, true);
                    buffer = tojson_sds(buffer, "Filename", entry_data->name, false);
                    buffer = sdscatlen(buffer, "}", 1);
                    break;
                }
                case MPD_ENTITY_TYPE_PLAYLIST: {
                    const struct mpd_playlist *pl = mpd_entity_get_playlist(entry_data->entity);
                    bool smartpls = is_smartpls(mympd_state->config->workdir, entry_data->name);
                    buffer = sdscatfmt(buffer, "{\"Type\": \"%s\",", (smartpls == true ? "smartpls" : "plist"));
                    buffer = tojson_char(buffer, "uri", mpd_playlist_get_path(pl), true);
                    buffer = tojson_sds(buffer, "name", entry_data->name, true);
                    buffer = tojson_sds(buffer, "Filename", entry_data->name, false);
                    buffer = sdscatlen(buffer, "}", 1);
                    break;
                }
                default:
                    break;
            }
        }
        mpd_entity_free(entry_data->entity);
        FREE_SDS(entry_data->name);
        FREE_PTR(iter.data);
        entity_count++;
    }
    raxStop(&iter);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = get_extra_media(mympd_state, buffer, path, true);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_llong(buffer, "totalEntities", (long long)entity_list->numele, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_sds(buffer, "search", searchstr, false);
    buffer = jsonrpc_result_end(buffer);
    raxFree(entity_list);
    return buffer;
}

//private functions
static bool search_dir_entry(rax *rt, sds key, sds entity_name, struct mpd_entity *entity, sds searchstr) {
    if (sdslen(searchstr) == 0 ||
        utf8casestr(entity_name, searchstr) != NULL)
    {
        struct t_dir_entry *entry_data = malloc_assert(sizeof(struct t_dir_entry));
        entry_data->name = entity_name;
        entry_data->entity = entity;
        sds_utf8_tolower(key);
        while (raxTryInsert(rt, (unsigned char *)key, sdslen(key), entry_data, NULL) == 0) {
            //duplicate - add chars until it is uniq
            key = sdscatlen(key, ":", 1);
        }
        return true;
    }
    mpd_entity_free(entity);
    FREE_SDS(entity_name);
    return false;
}
