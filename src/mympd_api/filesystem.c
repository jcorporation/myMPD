/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2026 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD filesystem API
 */

#include "src/mympd_api/filesystem.h"

#include "src/lib/array/array.h"
#include "src/lib/array/array_sort.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/mem.h"
#include "src/lib/sds/sds_extras.h"
#include "src/lib/sds/sds_file.h"
#include "src/lib/sds/sds_utf8.h"
#include "src/lib/smartpls.h"
#include "src/lib/utf8_wrapper.h"
#include "src/lib/utility.h"
#include "src/mympd_api/extra_media.h"
#include "src/mympd_api/sticker.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/stickerdb.h"
#include "src/mympd_client/tags.h"

#include <string.h>

/**
 * Private definitions
 */

static bool search_dir_entry(struct t_array *array, sds key, sds entity_name, struct mpd_entity *entity,
    const char *searchstr, size_t searchstr_len, bool *array_error);
static void free_mpd_entity(struct t_array_node *node);

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
    sds entity_name = sdsempty();
    struct t_array *entity_list = array_new();
    bool array_error = false;
    unsigned real_limit = offset + limit;
    size_t searchstr_len;
    char *searchstr_utf8 = utf8_wrap_normalize(searchstr, sdslen(searchstr), &searchstr_len);

    if (mpd_send_list_meta(partition_state->conn, path)) {
        struct mpd_entity *entity;
        while ((entity = mpd_recv_entity(partition_state->conn)) != NULL) {
            switch (mpd_entity_get_type(entity)) {
                case MPD_ENTITY_TYPE_SONG: {
                    const struct mpd_song *song = mpd_entity_get_song(entity);
                    entity_name = mympd_client_get_tag_value_string(song, MPD_TAG_TITLE, sdsempty());
                    key = sdscatfmt(key, "2%s", mpd_song_get_uri(song));
                    search_dir_entry(entity_list, key, entity_name, entity, searchstr_utf8, searchstr_len, &array_error);
                    break;
                }
                case MPD_ENTITY_TYPE_DIRECTORY: {
                    const struct mpd_directory *dir = mpd_entity_get_directory(entity);
                    entity_name = sdscat(entity_name, mpd_directory_get_path(dir));
                    sds_basename_uri(entity_name);
                    key = sdscatfmt(key, "0%s", mpd_directory_get_path(dir));
                    search_dir_entry(entity_list, key, entity_name, entity, searchstr_utf8, searchstr_len, &array_error);
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
                    entity_name = sdscat(entity_name, pl_path);
                    sds_basename_uri(entity_name);
                    key = sdscatfmt(key, "1%s", pl_path);
                    search_dir_entry(entity_list, key, entity_name, entity, searchstr_utf8, searchstr_len, &array_error);
                    break;
                }
                default: {
                    mpd_entity_free(entity);
                }
            }
            sdsclear(key);
            sdsclear(entity_name);
            if (array_error == true) {
                break;
            }
        }
    }
    FREE_SDS(key);
    FREE_SDS(entity_name);
    FREE_PTR(searchstr_utf8);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_send_list_meta") == false) {
        array_free(entity_list, free_mpd_entity);
        //return error message
        return buffer;
    }
    if (array_error == true ||
        array_sort_by_key(entity_list, ARRAY_SORT_ASC) == false)
    {
        array_free(entity_list, free_mpd_entity);
        return jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE,
            JSONRPC_SEVERITY_ERROR, "Could not sort filesystem results");
    }

    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");

    unsigned entity_count = 0;
    unsigned entities_returned = 0;

    bool print_stickers = check_get_sticker(partition_state->mpd_state->feat.stickers, &tagcols->stickers);
    if (print_stickers == true) {
        stickerdb_exit_idle(mympd_state->stickerdb);
    }
    while (entity_count < entity_list->length) {
        if (entity_count >= offset &&
            entity_count < real_limit)
        {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            struct mpd_entity *entity = (struct mpd_entity *)entity_list->nodes[entity_count].user_data;
            switch (mpd_entity_get_type(entity)) {
                case MPD_ENTITY_TYPE_SONG: {
                    const struct mpd_song *song = mpd_entity_get_song(entity);
                    buffer = sdscat(buffer, "{\"Type\":\"song\",");
                    buffer = print_song_tags(buffer, partition_state->mpd_state, &tagcols->mpd_tags, song);
                    buffer = sdscatlen(buffer, ",", 1);
                    sds filename = sdsnew(mpd_song_get_uri(song));
                    sds_basename_uri(filename);
                    buffer = tojson_sds(buffer, "Filename", filename, false);
                    FREE_SDS(filename);
                    if (print_stickers == true) {
                        buffer = mympd_api_sticker_get_print_batch(buffer, mympd_state->stickerdb, STICKER_TYPE_SONG, mpd_song_get_uri(song), &tagcols->stickers);
                    }
                    buffer = sdscatlen(buffer, "}", 1);
                    break;
                }
                case MPD_ENTITY_TYPE_DIRECTORY: {
                    const struct mpd_directory *dir = mpd_entity_get_directory(entity);
                    buffer = sdscat(buffer, "{\"Type\":\"dir\",");
                    buffer = tojson_char(buffer, "uri", mpd_directory_get_path(dir), true);
                    buffer = tojson_sds(buffer, "name", entity_list->nodes[entity_count].value_p, true);
                    buffer = tojson_sds(buffer, "Filename", entity_list->nodes[entity_count].value_p, false);
                    buffer = sdscatlen(buffer, "}", 1);
                    break;
                }
                case MPD_ENTITY_TYPE_PLAYLIST: {
                    const struct mpd_playlist *pl = mpd_entity_get_playlist(entity);
                    bool smartpls = is_smartpls(partition_state->config->workdir, entity_list->nodes[entity_count].value_p);
                    buffer = sdscatfmt(buffer, "{\"Type\": \"%s\",", (smartpls == true ? "smartpls" : "plist"));
                    buffer = tojson_char(buffer, "uri", mpd_playlist_get_path(pl), true);
                    buffer = tojson_sds(buffer, "name", entity_list->nodes[entity_count].value_p, true);
                    buffer = tojson_sds(buffer, "Filename", entity_list->nodes[entity_count].value_p, false);
                    buffer = sdscatlen(buffer, "}", 1);
                    break;
                }
                default:
                    break;
            }
        }
        entity_count++;
    }
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
    array_free(entity_list, free_mpd_entity);
    return buffer;
}

/**
 * private functions
 */

/**
 * Search the entry for searchstr and add matches to the rax tree
 * @param array Pointer to the array to add the entry
 * @param key Pointer to the key to insert
 * @param entity_name Displayname of the entity
 * @param entity Pointer to mpd entity
 * @param searchstr String to search in entity_name
 * @param searchstr_len Length of searchstr
 * @param array_error Pointer to bool to signal error in array_push
 * @return true on match, else false
 */
static bool search_dir_entry(struct t_array *array, sds key, sds entity_name, struct mpd_entity *entity,
    const char *searchstr, size_t searchstr_len, bool *array_error)
{
    size_t newlen;
    char *value_utf8 = utf8_wrap_normalize(entity_name, sdslen(entity_name), &newlen);

    if (searchstr_len == 0 ||
        strstr(value_utf8, searchstr) != NULL)
    {
        if (array_push(array, key, entity_name, 0, entity) == false) {
            *array_error = true;
            mpd_entity_free(entity);
        }
        FREE_PTR(value_utf8);
        return true;
    }
    mpd_entity_free(entity);
    FREE_PTR(value_utf8);
    return false;
}

/**
 * Free the memory allocated for an MPD entity
 * @param node Pointer to the array node containing the entity
 */
static void free_mpd_entity(struct t_array_node *node) {
    struct mpd_entity *entity = (struct mpd_entity *)node->user_data;
    if (entity != NULL) {
        mpd_entity_free(entity);
    }
}
