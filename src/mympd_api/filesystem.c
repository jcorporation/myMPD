/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD filesystem API
 */

#include "compile_time.h"
#include "src/mympd_api/filesystem.h"

#include "dist/utf8/utf8.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/mem.h"
#include "src/lib/rax_extras.h"
#include "src/lib/sds_extras.h"
#include "src/lib/smartpls.h"
#include "src/lib/utility.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/stickerdb.h"
#include "src/mympd_client/tags.h"
#include "src/mympd_api/extra_media.h"
#include "src/mympd_api/sticker.h"

#include <libgen.h>
#include <string.h>

/**
 * Private definitions
 */

/**
 * Struct representing the entity in the rax tree
 */
struct t_dir_entry {
    sds name;                   //!< entity name (e.g. filename, playlistname, directory name)
    struct mpd_entity *entity;  //!< pointer to the generic mpd entity struct
};

static void free_t_dir_entry(void *data);
static bool search_dir_entry(rax *rt, sds key, sds entity_name, struct mpd_entity *entity, sds searchstr);

/**
 * Public functions
 */

/**
 * Lists the entry of directory in the mpd music directory as jsonrpc response
 * Custom order: directories, playlists, songs
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to the partition state
 * @param buffer already allocated sds string to append result
 * @param request_id jsonrpc request id
 * @param path path to list
 * @param offset offset for listing
 * @param limit max entries to list
 * @param searchstr string to search
 * @param tagcols columns to print
 * @return pointer to buffer
 */
sds mympd_api_browse_filesystem(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, sds path, unsigned offset, unsigned limit, sds searchstr, const struct t_fields *tagcols)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_DATABASE_FILESYSTEM_LIST;
    sds key = sdsempty();
    rax *entity_list = raxNew();
    unsigned real_limit = offset + limit;

    if (mpd_send_list_meta(partition_state->conn, path)) {
        struct mpd_entity *entity;
        while ((entity = mpd_recv_entity(partition_state->conn)) != NULL) {
            switch (mpd_entity_get_type(entity)) {
                case MPD_ENTITY_TYPE_SONG: {
                    const struct mpd_song *song = mpd_entity_get_song(entity);
                    sds entity_name =  mympd_client_get_tag_value_string(song, MPD_TAG_TITLE, sdsempty());
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
                    if (partition_state->mpd_state->feat.mpd_0_24_0 == false) {
                        // Workaround for older clients
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
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_list_meta") == false) {
        //free result
        rax_free_data(entity_list, free_t_dir_entry);
        //return error message
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");

    unsigned entity_count = 0;
    unsigned entities_returned = 0;

    raxIterator iter;
    raxStart(&iter, entity_list);
    raxSeek(&iter, "^", NULL, 0);
    bool print_stickers = check_get_sticker(partition_state->mpd_state->feat.stickers, &tagcols->stickers);
    if (print_stickers == true) {
        stickerdb_exit_idle(mympd_state->stickerdb);
    }
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
                    buffer = print_song_tags(buffer, partition_state->mpd_state, &tagcols->mpd_tags, song);
                    buffer = sdscatlen(buffer, ",", 1);
                    sds filename = sdsnew(mpd_song_get_uri(song));
                    basename_uri(filename);
                    buffer = tojson_sds(buffer, "Filename", filename, false);
                    FREE_SDS(filename);
                    if (print_stickers == true) {
                        buffer = mympd_api_sticker_get_print_batch(buffer, mympd_state->stickerdb, STICKER_TYPE_SONG, mpd_song_get_uri(song), &tagcols->stickers);
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
                    bool smartpls = is_smartpls(partition_state->config->workdir, entry_data->name);
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
    if (print_stickers == true) {
        stickerdb_enter_idle(mympd_state->stickerdb);
    }
    buffer = sdscatlen(buffer, "],", 2);
    buffer = mympd_api_get_extra_media(buffer, partition_state->mpd_state, mympd_state->booklet_name, mympd_state->info_txt_name, path, true);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_uint(buffer, "totalEntities", entity_count, true);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "offset", offset, true);
    buffer = tojson_sds(buffer, "search", searchstr, false);
    buffer = jsonrpc_end(buffer);
    raxFree(entity_list);
    return buffer;
}

/**
 * private functions
 */

/**
 * Frees the t_dir_entry struct used as callback for rax_free_data
 * @param data void pointer to a t_dir_entry struct
 */
static void free_t_dir_entry(void *data) {
    struct t_dir_entry *entry_data = (struct t_dir_entry *)data;
    mpd_entity_free(entry_data->entity);
    FREE_SDS(entry_data->name);
    FREE_PTR(data);
}

/**
 * Search the entry for searchstr and add matches to the rax tree
 * @param rt rax tree to insert
 * @param key key to insert
 * @param entity_name displayname of the entity
 * @param entity pointer to mpd entity
 * @param searchstr string to search in entity_name
 * @return true on match, else false
 */
static bool search_dir_entry(rax *rt, sds key, sds entity_name, struct mpd_entity *entity, sds searchstr) {
    if (sdslen(searchstr) == 0 ||
        utf8casestr(entity_name, searchstr) != NULL)
    {
        struct t_dir_entry *entry_data = malloc_assert(sizeof(struct t_dir_entry));
        entry_data->name = entity_name;
        entry_data->entity = entity;
        sds_utf8_tolower(key);
        rax_insert_no_dup(rt, key, entry_data);
        return true;
    }
    mpd_entity_free(entity);
    FREE_SDS(entity_name);
    return false;
}
