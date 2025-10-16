/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD browse database API
 */

#include "compile_time.h"
#include "src/mympd_api/albums.h"

#include "src/lib/album.h"
#include "src/lib/cache/cache_rax_album.h"
#include "src/lib/fields.h"
#include "src/lib/json/json_print.h"
#include "src/lib/json/json_rpc.h"
#include "src/lib/log.h"
#include "src/lib/rax_extras.h"
#include "src/lib/sds_extras.h"
#include "src/lib/search.h"
#include "src/lib/sticker.h"
#include "src/mympd_api/extra_media.h"
#include "src/mympd_api/sticker.h"
#include "src/mympd_client/errorhandler.h"
#include "src/mympd_client/search.h"
#include "src/mympd_client/stickerdb.h"
#include "src/mympd_client/tags.h"

#include <string.h>

// private definitions
static sds get_sort_key_album(sds key, enum sort_by_type sort_by,
        enum mpd_tag_type sort_tag, const struct t_album *album);
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
sds mympd_api_album_detail(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
        sds buffer, unsigned request_id, sds albumid, const struct t_fields *tagcols)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_DATABASE_ALBUM_DETAIL;

    if (mympd_state->album_cache.cache == NULL) {
        buffer = jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_WARN, "Albumcache not ready");
        return buffer;
    }

    struct t_album *mpd_album = album_cache_get_album(&mympd_state->album_cache, albumid);
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
        album_set_total_time(mpd_album, 0);
        album_set_disc_count(mpd_album, 0);
        album_set_song_count(mpd_album, 0);
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
                album_inc_total_time(mpd_album, mpd_song_get_duration(song));
                album_set_discs(mpd_album, mpd_song_get_tag(song, MPD_TAG_DISC, 0));
                album_inc_song_count(mpd_album);
            }
            mpd_song_free(song);
        }
    }
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_search_commit") == false) {
        FREE_SDS(first_song_uri);
        FREE_SDS(last_played_song_uri);
        return buffer;
    }
    if (print_stickers == true) {
        stickerdb_enter_idle(mympd_state->stickerdb);
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = mympd_api_get_extra_media(buffer, partition_state->mpd_state, mympd_state->booklet_name, mympd_state->info_txt_name, first_song_uri, false);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_uint(buffer, "totalEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_uint(buffer, "offset", 0, true);
    buffer = tojson_uint(buffer, "limit", MPD_RESULTS_MAX, true);
    buffer = tojson_sds(buffer, "AlbumId", albumid, true);
    buffer = print_album_tags(buffer, &partition_state->mpd_state->config->albums, &partition_state->mpd_state->tags_album, mpd_album);
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
sds mympd_api_album_list(struct t_mympd_state *mympd_state, struct t_partition_state *partition_state,
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
    if (expr_list == NULL) {
        buffer = jsonrpc_respond_message(buffer, MYMPD_API_DATABASE_ALBUM_LIST, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_WARN, "Invalid search expression");
        return buffer;
    }
    
    //search and sort albumlist
    unsigned real_limit = offset + limit;
    rax *albums = raxNew();
    raxIterator iter;
    raxStart(&iter, mympd_state->album_cache.cache);
    raxSeek(&iter, "^", NULL, 0);
    sds key = sdsempty();
    while (raxNext(&iter)) {
        struct t_album *album = (struct t_album *)iter.data;
        if (expr_list->length == 0 ||
            search_expression_album(album, expr_list, &partition_state->mpd_state->tags_browse) == true)
        {
            key = get_sort_key_album(key, sort_by, sort_tag, album);
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
            struct t_album *album = (struct t_album *)iter.data;
            buffer = sdscat(buffer, "{\"Type\": \"album\",");
            buffer = print_album_tags(buffer, &partition_state->mpd_state->config->albums, &tagcols->mpd_tags, album);
            buffer = sdscatlen(buffer, ",", 1);
            buffer = tojson_char(buffer, "FirstSongUri", album_get_uri(album), false);
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

// private functions

/**
 * Gets an alphanumeric string for sorting
 * @param key already allocated sds string to append
 * @param sort_by enum sort_by
 * @param sort_tag mpd tag to sort by
 * @param album pointer to t_album
 * @return pointer to key
 */
static sds get_sort_key_album(sds key, enum sort_by_type sort_by, enum mpd_tag_type sort_tag, const struct t_album *album) {
    if (sort_by == SORT_BY_LAST_MODIFIED) {
        key = sds_pad_int((int64_t)album_get_last_modified(album), key);
    }
    else if (sort_by == SORT_BY_ADDED) {
        key = sds_pad_int((int64_t)album_get_added(album), key);
    }
    else if (is_numeric_tag(sort_tag) == true) {
        key = album_get_tag_value_padded(album, sort_tag, '0', PADDING_LENGTH, key);
    }
    else if (sort_tag > MPD_TAG_UNKNOWN) {
        key = album_get_tag_value_string(album, sort_tag, key);
        if (sdslen(key) == 0) {
            key = sdscatlen(key, "zzzzzzzzzz", 10);
        }
    }
    enum mpd_tag_type secondary_sort_tag = sort_tag == MPD_TAG_ALBUM
        ? MPD_TAG_ALBUM_ARTIST
        : MPD_TAG_ALBUM;
    key = sdscatfmt(key, "::%s::%s", album_get_tag(album, secondary_sort_tag, 0), album_get_uri(album));
    sds_utf8_tolower(key);
    return key;
}


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
