/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD browse database API
 */

#include "compile_time.h"
#include "src/mympd_api/browse.h"

#include "dist/utf8/utf8.h"
#include "src/lib/cache_rax_album.h"
#include "src/lib/fields.h"
#include "src/lib/filehandler.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/rax_extras.h"
#include "src/lib/sds_extras.h"
#include "src/lib/search.h"
#include "src/lib/sticker.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/search.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/extra_media.h"
#include "src/mympd_api/sticker.h"

#include <dirent.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

// private definitions

static bool check_album_sort_tag(enum sort_by_type sort_by, enum mpd_tag_type sort_tag,
        struct t_albums_config *album_config);

// public functions

/**
 * Lists album details
 * @param mympd_state pointer to mympd state
 * @param partition_state pointer to partition specific states
 * @param buffer sds string to append response
 * @param request_id jsonrpc request id
 * @param albumid id of the album (key in the album_cache)
 * @param tagcols t_fields struct of song tags to print
 * @return pointer to buffer
 */
sds mympd_api_browse_album_detail(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, sds albumid, const struct t_fields *tagcols)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_DATABASE_ALBUM_DETAIL;

    if (mympd_state->album_cache.cache == NULL) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_WARN, "Albumcache not ready");
        return buffer;
    }

    struct mpd_song *mpd_album = album_cache_get_album(&mympd_state->album_cache, albumid);
    if (mpd_album == NULL) {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Album not found");
    }

    sds expression = get_search_expression_album(sdsempty(), partition_state->mpd_state->tag_albumartist, mpd_album,
        &partition_state->config->albums);

    if (mpd_search_db_songs(partition_state->conn, true) == false ||
        mpd_search_add_expression(partition_state->conn, expression) == false ||
        mpd_search_add_sort_tag(partition_state->conn, MPD_TAG_DISC, false) == false ||
        mpd_search_add_window(partition_state->conn, 0, MPD_RESULTS_MAX) == false)
    {
        mpd_search_cancel(partition_state->conn);
        FREE_SDS(expression);
        return jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE,
            JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
    }
    unsigned entities_returned = 0;
    time_t last_played_max = 0;
    sds first_song_uri = sdsempty();
    sds last_played_song_uri = sdsempty();
    sds last_played_song_title = sdsempty();
    unsigned last_played_song_pos = 0;
    if (partition_state->config->albums.mode == ALBUM_MODE_SIMPLE) {
        // reset album values for simple album mode
        album_cache_set_total_time(mpd_album, 0);
        album_cache_set_disc_count(mpd_album, 0);
        album_cache_set_song_count(mpd_album, 0);
    }
    bool print_stickers = check_get_sticker(partition_state->mpd_state->feat.stickers, &tagcols->stickers);
    if (print_stickers == true) {
        stickerdb_exit_idle(mympd_state->stickerdb);
    }
    if (mpd_search_commit(partition_state->conn)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");

        struct mpd_song *song;
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            else {
                first_song_uri = sdscat(first_song_uri, mpd_song_get_uri(song));
            }
            buffer = sdscat(buffer, "{\"Type\": \"song\",");
            buffer = print_song_tags(buffer, partition_state->mpd_state, &tagcols->mpd_tags, song);
            if (print_stickers == true) {
                struct t_sticker sticker;
                stickerdb_get_all_batch(mympd_state->stickerdb, STICKER_TYPE_SONG, mpd_song_get_uri(song), &sticker, false);
                buffer = mympd_api_sticker_print(buffer, &sticker, &tagcols->stickers);

                if (sticker.mympd[STICKER_LAST_PLAYED] > last_played_max) {
                    last_played_max = (time_t)sticker.mympd[STICKER_LAST_PLAYED];
                    last_played_song_uri = sds_replace(last_played_song_uri, mpd_song_get_uri(song));
                    last_played_song_title = sds_replace(last_played_song_title, mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
                    last_played_song_pos = entities_returned - 1;
                }
                sticker_struct_clear(&sticker);
            }
            buffer = sdscatlen(buffer, "}", 1);
            if (partition_state->config->albums.mode == ALBUM_MODE_SIMPLE) {
                // calculate some album values for simple album mode
                album_cache_inc_total_time(mpd_album, song);
                album_cache_set_discs(mpd_album, song);
                album_cache_inc_song_count(mpd_album);
            }
            mpd_song_free(song);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (print_stickers == true) {
        stickerdb_enter_idle(mympd_state->stickerdb);
    }
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_search_commit") == false) {
        FREE_SDS(first_song_uri);
        FREE_SDS(last_played_song_uri);
        return buffer;
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = mympd_api_get_extra_media(buffer, partition_state->mpd_state, mympd_state->booklet_name, mympd_state->info_txt_name, first_song_uri, false);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_uint(buffer, "totalEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "offset", 0, true);
    buffer = tojson_uint(buffer, "limit", MPD_RESULTS_MAX, true);
    buffer = tojson_sds(buffer, "AlbumId", albumid, true);
    buffer = print_album_tags(buffer, partition_state->mpd_state, &partition_state->mpd_state->tags_album, mpd_album);
    buffer = sdscat(buffer, ",\"lastPlayedSong\":{");
    buffer = tojson_time(buffer, "time", last_played_max, true);
    buffer = tojson_uint(buffer, "pos", last_played_song_pos, true);
    buffer = tojson_sds(buffer, "title", last_played_song_title, true);
    buffer = tojson_sds(buffer, "uri", last_played_song_uri, false);
    buffer = sdscatlen(buffer, "}", 1);
    if (partition_state->mpd_state->feat.stickers == true) {
        struct t_stickers sticker;
        stickers_reset(&sticker);
        stickers_enable_all(&sticker, STICKER_TYPE_FILTER);
        buffer = mympd_api_sticker_get_print(buffer, mympd_state->stickerdb, STICKER_TYPE_FILTER, expression, &sticker);
    }
    buffer = jsonrpc_end(buffer);

    FREE_SDS(expression);
    FREE_SDS(first_song_uri);
    FREE_SDS(last_played_song_uri);
    FREE_SDS(last_played_song_title);
    return buffer;
}

/**
 * Lists albums from the album_cache
 * @param mympd_state pointer to mympd_state
 * @param partition_state pointer to partition specific states
 * @param buffer sds string to append response
 * @param request_id jsonrpc request id
 * @param expression mpd search expression
 * @param sort tag to sort the result
 * @param sortdesc true to sort descending, false to sort ascending
 * @param offset offset of results to print
 * @param limit max number of results to print
 * @param tagcols tags to print
 * @return pointer to buffer
 */
sds mympd_api_browse_album_list(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, sds expression, sds sort, bool sortdesc, unsigned offset, unsigned limit,
        const struct t_fields *tagcols)
{
    if (mympd_state->album_cache.cache == NULL) {
        buffer = jsonrpc_respond_message(buffer, MYMPD_API_DATABASE_ALBUM_LIST, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_WARN, "Albumcache not ready");
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, MYMPD_API_DATABASE_ALBUM_LIST, request_id);
    buffer = sdscat(buffer, "\"data\":[");

    //parse sort tag
    enum mpd_tag_type sort_tag = MPD_TAG_ALBUM;
    enum sort_by_type sort_by = SORT_BY_TAG;

    if (sdslen(sort) > 0) {
        sort_tag = mpd_tag_name_parse(sort);
        if (sort_tag != MPD_TAG_UNKNOWN) {
            sort_tag = get_sort_tag(sort_tag, &partition_state->mpd_state->tags_mympd);
        }
        else if (strcmp(sort, "Added") == 0) {
            sort_by = SORT_BY_ADDED;
            //swap order
            sortdesc = sortdesc == false
                ? true
                : false;
        }
        else if (strcmp(sort, "Last-Modified") == 0) {
            sort_by = SORT_BY_LAST_MODIFIED;
            //swap order
            sortdesc = sortdesc == false
                ? true
                : false;
        }
        else {
            MYMPD_LOG_WARN(partition_state->name, "Unknown sort tag: %s", sort);
            sort_tag = MPD_TAG_ALBUM;
        }
    }

    if (check_album_sort_tag(sort_by, sort_tag, &partition_state->config->albums) == false) {
        buffer = jsonrpc_respond_message(buffer, MYMPD_API_DATABASE_ALBUM_LIST, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_WARN, "Invalid sort tag");
        return buffer;
    }

    //parse mpd search expression
    struct t_list *expr_list = parse_search_expression_to_list(expression, SEARCH_TYPE_SONG);
    
    //search and sort albumlist
    unsigned real_limit = offset + limit;
    rax *albums = raxNew();
    raxIterator iter;
    raxStart(&iter, mympd_state->album_cache.cache);
    raxSeek(&iter, "^", NULL, 0);
    sds key = sdsempty();
    while (raxNext(&iter)) {
        struct mpd_song *album = (struct mpd_song *)iter.data;
        if (expr_list->length == 0 ||
            search_expression_song(album, expr_list, &partition_state->mpd_state->tags_browse) == true)
        {
            key = get_sort_key(key, sort_by, sort_tag, album);
            rax_insert_no_dup(albums, key, iter.data);
            sdsclear(key);
        }
    }
    raxStop(&iter);
    free_search_expression_list(expr_list);
    FREE_SDS(key);

    //print album list
    bool print_stickers = check_get_sticker(partition_state->mpd_state->feat.stickers, &tagcols->stickers);
    if (print_stickers == true) {
        stickerdb_exit_idle(mympd_state->stickerdb);
    }
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    raxStart(&iter, albums);
    int (*iterator)(struct raxIterator *iter);
    if (sortdesc == false) {
        raxSeek(&iter, "^", NULL, 0);
        iterator = &raxNext;
    }
    else {
        raxSeek(&iter, "$", NULL, 0);
        iterator = &raxPrev;
    }
    sds album_exp = sdsempty();
    while (iterator(&iter)) {
        if (entity_count >= offset) {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            struct mpd_song *album = (struct mpd_song *)iter.data;
            buffer = sdscat(buffer, "{\"Type\": \"album\",");
            buffer = print_album_tags(buffer, partition_state->mpd_state, &tagcols->mpd_tags, album);
            buffer = sdscatlen(buffer, ",", 1);
            buffer = tojson_char(buffer, "FirstSongUri", mpd_song_get_uri(album), false);
            if (print_stickers == true) {
                buffer = sdscatlen(buffer, ",", 1);
                album_exp = get_search_expression_album(album_exp, mympd_state->mpd_state->tag_albumartist, album, &mympd_state->config->albums);
                buffer = mympd_api_sticker_get_print_batch(buffer, mympd_state->stickerdb, STICKER_TYPE_FILTER, album_exp, &tagcols->stickers);
                
            }
            buffer = sdscatlen(buffer, "}", 1);
        }
        entity_count++;
        if (entity_count == real_limit) {
            break;
        }
    }
    raxStop(&iter);
    FREE_SDS(album_exp);
    if (print_stickers == true) {
        stickerdb_enter_idle(mympd_state->stickerdb);
    }
    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint64(buffer, "totalEntities", albums->numele, true);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "offset", offset, true);
    buffer = tojson_sds(buffer, "expression", expression, true);
    buffer = tojson_sds(buffer, "sort", sort, true);
    buffer = tojson_bool(buffer, "sortdesc", sortdesc, true);
    buffer = tojson_char(buffer, "tag", "Album", false);
    buffer = jsonrpc_end(buffer);
    raxFree(albums);
    return buffer;
}

/**
 * Lists tags from the mpd database
 * @param partition_state pointer to partition specific states
 * @param buffer sds string to append response
 * @param request_id jsonrpc request id
 * @param searchstr string to search
 * @param tag tag type to list
 * @param offset offset of results to print
 * @param limit max number of results to print
 * @param sortdesc true to sort descending, false to sort ascending
 * @return pointer to buffer
 */
sds mympd_api_browse_tag_list(struct t_partition_state *partition_state, sds buffer, unsigned request_id,
        sds searchstr, sds tag, unsigned offset, unsigned limit, bool sortdesc)
{
    size_t searchstr_len = sdslen(searchstr);
    enum mympd_cmd_ids cmd_id = MYMPD_API_DATABASE_TAG_LIST;

    if (mpd_search_db_tags(partition_state->conn, mpd_tag_name_parse(tag)) == false) {
        mpd_search_cancel(partition_state->conn);
        return jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE,
            JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
    }

    unsigned real_limit = offset + limit;
    rax *taglist = raxNew();
    sds key = sdsempty();

    if (mpd_search_commit(partition_state->conn)) {
        struct mpd_pair *pair;
        enum mpd_tag_type mpdtag = mpd_tag_name_parse(tag);
        //filter and sort
        while ((pair = mpd_recv_pair_tag(partition_state->conn, mpdtag)) != NULL) {
            if (pair->value[0] == '\0') {
                MYMPD_LOG_DEBUG(partition_state->name, "Value is empty, skipping");
            }
            else if (searchstr_len == 0 ||
                (searchstr_len <= 2 && utf8ncasecmp(searchstr, pair->value, searchstr_len) == 0) ||
                (searchstr_len > 2 && utf8casestr(pair->value, searchstr) != NULL))
            {
                key = sdscat(key, pair->value);
                //handle tags case insensitive
                sds_utf8_tolower(key);
                sds data = sdsnew(pair->value);
                rax_insert_no_dup(taglist, key, data);
                sdsclear(key);
            }
            mpd_return_pair(partition_state->conn, pair);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_search_commit") == false) {
        rax_free_sds_data(taglist);
        return buffer;
    }
    FREE_SDS(key);

    //print list
    buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
    buffer = sdscat(buffer, "\"data\":[");
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    raxIterator iter;
    raxStart(&iter, taglist);
    int (*iterator)(struct raxIterator *iter);
    if (sortdesc == false) {
        raxSeek(&iter, "^", NULL, 0);
        iterator = &raxNext;
    }
    else {
        raxSeek(&iter, "$", NULL, 0);
        iterator = &raxPrev;
    }
    while (iterator(&iter)) {
        if (entity_count >= offset &&
            entity_count < real_limit)
        {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_sds(buffer, "Value", (sds)iter.data, false);
            buffer = sdscatlen(buffer, "}", 1);
        }
        entity_count++;
        FREE_SDS(iter.data);
    }
    raxStop(&iter);

    //checks if this tag has a directory with pictures in /src/lib/mympd/pics
    sds pic_path = sdscatfmt(sdsempty(), "%S/%s/%s", partition_state->config->workdir, DIR_WORK_PICS, tag);
    bool pic =  testdir("Tag pics folder", pic_path, false, true) == DIR_EXISTS
        ? true
        : false;
    FREE_SDS(pic_path);

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_uint64(buffer, "totalEntities", taglist->numele, true);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "offset", offset, true);
    buffer = tojson_sds(buffer, "searchstr", searchstr, true);
    buffer = tojson_sds(buffer, "tag", tag, true);
    buffer = tojson_bool(buffer, "pics", pic, false);
    buffer = jsonrpc_end(buffer);
    raxFree(taglist);
    return buffer;
}

// private functions

/**
 * Validates the album sort tag
 * @param sort_by sort by type
 * @param sort_tag sort tag
 * @param album_config pointer to album config
 * @return true on valid tag, else false
 */
static bool check_album_sort_tag(enum sort_by_type sort_by, enum mpd_tag_type sort_tag,
        struct t_albums_config *album_config)
{
    if (album_config->mode == ALBUM_MODE_SIMPLE) {
        if (sort_by != SORT_BY_TAG) {
            return false;
        }
        if (album_config->group_tag != MPD_TAG_UNKNOWN &&
            sort_tag == album_config->group_tag)
        {
            return true;
        }
        switch(sort_tag) {
            case MPD_TAG_ALBUM_ARTIST_SORT:
            case MPD_TAG_ALBUM_ARTIST:
            case MPD_TAG_ALBUM_SORT:
            case MPD_TAG_ALBUM:
                return true;
            default:
                return false;
        }
    }
    return true;
}
