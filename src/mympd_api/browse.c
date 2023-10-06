/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2023 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/browse.h"

#include "dist/utf8/utf8.h"
#include "src/lib/album_cache.h"
#include "src/lib/jsonrpc.h"
#include "src/lib/log.h"
#include "src/lib/rax_extras.h"
#include "src/lib/sds_extras.h"
#include "src/lib/sticker.h"
#include "src/mpd_client/errorhandler.h"
#include "src/mpd_client/search.h"
#include "src/mpd_client/search_local.h"
#include "src/mpd_client/stickerdb.h"
#include "src/mpd_client/tags.h"
#include "src/mympd_api/extra_media.h"
#include "src/mympd_api/sticker.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>

/**
 * Lists album details
 * @param partition_state pointer to partition specific states
 * @param buffer sds string to append response
 * @param request_id jsonrpc request id
 * @param albumid id of the album (key in the album_cache)
 * @param tagcols t_tags struct of song tags to print
 * @return pointer to buffer
 */
sds mympd_api_browse_album_detail(struct t_partition_state *partition_state, sds buffer, long request_id,
        sds albumid, const struct t_tags *tagcols)
{
    enum mympd_cmd_ids cmd_id = MYMPD_API_DATABASE_ALBUM_DETAIL;

    struct mpd_song *mpd_album = album_cache_get_album(&partition_state->mpd_state->album_cache, albumid);
    if (mpd_album == NULL) {
        return jsonrpc_respond_message(buffer, cmd_id, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_ERROR, "Could not find album");
    }

    sds expression = get_search_expression_album(partition_state->mpd_state->tag_albumartist, mpd_album);

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
    FREE_SDS(expression);
    int entities_returned = 0;
    time_t last_played_max = 0;
    sds first_song_uri = sdsempty();
    sds last_played_song_uri = sdsempty();
    unsigned duration = 0;
    if (mpd_search_commit(partition_state->conn)) {
        buffer = jsonrpc_respond_start(buffer, cmd_id, request_id);
        buffer = sdscat(buffer, "\"data\":[");

        struct mpd_song *song;
        if (partition_state->mpd_state->feat_stickers == true &&
            tagcols->stickers_len > 0)
        {
            stickerdb_exit_idle(partition_state->mympd_state->stickerdb);
        }
        while ((song = mpd_recv_song(partition_state->conn)) != NULL) {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            else {
                first_song_uri = sdscat(first_song_uri, mpd_song_get_uri(song));
            }
            buffer = sdscat(buffer, "{\"Type\": \"song\",");
            buffer = print_song_tags(buffer, partition_state->mpd_state->feat_tags, tagcols, song,
                &partition_state->mympd_state->config->albums);
            if (partition_state->mpd_state->feat_stickers == true &&
                tagcols->stickers_len > 0)
            {
                struct t_sticker sticker;
                stickerdb_get_all_batch(partition_state->mympd_state->stickerdb, mpd_song_get_uri(song), &sticker, false);
                buffer = mympd_api_sticker_print(buffer, &sticker, tagcols);

                if (sticker.mympd[STICKER_LAST_PLAYED] > last_played_max) {
                    last_played_max = (time_t)sticker.mympd[STICKER_LAST_PLAYED];
                    last_played_song_uri = sds_replace(last_played_song_uri, mpd_song_get_uri(song));
                }
                sticker_struct_clear(&sticker);
            }
            buffer = sdscatlen(buffer, "}", 1);
            duration += mpd_song_get_duration(song);
            mpd_song_free(song);
        }
    }
    mpd_response_finish(partition_state->conn);
    if (partition_state->mpd_state->feat_stickers == true &&
        tagcols->stickers_len > 0)
    {
        stickerdb_enter_idle(partition_state->mympd_state->stickerdb);
    }
    if (mympd_check_error_and_recover_respond(partition_state, &buffer, cmd_id, request_id, "mpd_search_commit") == false) {
        FREE_SDS(first_song_uri);
        FREE_SDS(last_played_song_uri);
        return buffer;
    }

    // Set album duration for simple album mode
    if (partition_state->mympd_state->config->albums.mode == ALBUM_MODE_SIMPLE) {
        album_cache_set_total_time(mpd_album, duration);
    }

    buffer = sdscatlen(buffer, "],", 2);
    buffer = mympd_api_get_extra_media(partition_state->mpd_state, buffer, first_song_uri, false);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_int(buffer, "returnedEntities", entities_returned, true);
    buffer = print_album_tags(buffer, &partition_state->mpd_state->tags_album, mpd_album, &partition_state->mympd_state->config->albums);
    buffer = sdscat(buffer, ",\"lastPlayedSong\":{");
    buffer = tojson_time(buffer, "time", last_played_max, true);
    buffer = tojson_sds(buffer, "uri", last_played_song_uri, false);
    buffer = sdscatlen(buffer, "}", 1);
    buffer = jsonrpc_end(buffer);

    FREE_SDS(first_song_uri);
    FREE_SDS(last_played_song_uri);
    return buffer;
}

/**
 * Lists albums from the album_cache
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
sds mympd_api_browse_album_list(struct t_partition_state *partition_state, sds buffer, long request_id,
        sds expression, sds sort, bool sortdesc, long offset, long limit, const struct t_tags *tagcols)
{
    if (partition_state->mpd_state->album_cache.cache == NULL) {
        buffer = jsonrpc_respond_message(buffer, MYMPD_API_DATABASE_ALBUM_LIST, request_id,
            JSONRPC_FACILITY_DATABASE, JSONRPC_SEVERITY_WARN, "Albumcache not ready");
        return buffer;
    }

    buffer = jsonrpc_respond_start(buffer, MYMPD_API_DATABASE_ALBUM_LIST, request_id);
    buffer = sdscat(buffer, "\"data\":[");

    //parse sort tag
    bool sort_by_last_modified = false;
    enum mpd_tag_type sort_tag = MPD_TAG_ALBUM;

    if (sdslen(sort) > 0) {
        sort_tag = mpd_tag_name_parse(sort);
        if (sort_tag != MPD_TAG_UNKNOWN) {
            sort_tag = get_sort_tag(sort_tag, &partition_state->mpd_state->tags_mympd);
        }
        else if (strcmp(sort, "LastModified") == 0) {
            sort_by_last_modified = true;
            //swap order
            sortdesc = sortdesc == false ? true : false;
        }
        else {
            MYMPD_LOG_WARN(partition_state->name, "Unknown sort tag: %s", sort);
            sort_tag = MPD_TAG_ALBUM;
        }
    }
    //parse mpd search expression
    struct t_list *expr_list = parse_search_expression_to_list(expression);
    
    //search and sort albumlist
    long real_limit = offset + limit;
    rax *albums = raxNew();
    raxIterator iter;
    raxStart(&iter, partition_state->mpd_state->album_cache.cache);
    raxSeek(&iter, "^", NULL, 0);
    sds key = sdsempty();
    while (raxNext(&iter)) {
        struct mpd_song *album = (struct mpd_song *)iter.data;
        if (expr_list->length == 0 ||
            search_song_expression(album, expr_list, &partition_state->mpd_state->tags_browse) == true)
        {
            if (sort_by_last_modified == true) {
                key = sdscatprintf(key, "%020lld::%s", (long long)mpd_song_get_last_modified(album), mpd_song_get_uri(album));
            }
            else {
                key = mpd_client_get_tag_value_string(album, sort_tag, key);
                if (sdslen(key) > 0) {
                    key = sdscatfmt(key, "::%s", mpd_song_get_uri(album));
                }
                else {
                    //sort tag not present, append to end of the list
                    MYMPD_LOG_WARN(partition_state->name, "Sort tag \"%s\" not set for \"%.*s\"", mpd_tag_name(sort_tag), (int)iter.key_len, (char *)iter.key);
                    key = sdscatfmt(key, "zzzzzzzzzz::%s", mpd_song_get_uri(album));
                }
            }
            sds_utf8_tolower(key);
            while (raxTryInsert(albums, (unsigned char*)key, sdslen(key), iter.data, NULL) == 0) {
                //duplicate - add chars until it is uniq
                key = sdscatlen(key, ":", 1);
            }
            sdsclear(key);
        }
    }
    raxStop(&iter);
    free_search_expression_list(expr_list);
    FREE_SDS(key);

    //print album list
    long entity_count = 0;
    long entities_returned = 0;
    raxStart(&iter, albums);

    if (sortdesc == false) {
        raxSeek(&iter, "^", NULL, 0);
    }
    else {
        raxSeek(&iter, "$", NULL, 0);
    }
    while (sortdesc == false ? raxNext(&iter) : raxPrev(&iter)) {
        if (entity_count >= offset) {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            struct mpd_song *album = (struct mpd_song *)iter.data;
            buffer = sdscat(buffer, "{\"Type\": \"album\",");
            buffer = print_album_tags(buffer, tagcols, album, &partition_state->mympd_state->config->albums);
            buffer = sdscatlen(buffer, ",", 1);
            buffer = tojson_char(buffer, "FirstSongUri", mpd_song_get_uri(album), false);
            buffer = sdscatlen(buffer, "}", 1);
        }
        entity_count++;
        if (entity_count == real_limit) {
            break;
        }
    }
    raxStop(&iter);

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_llong(buffer, "totalEntities", (long long)albums->numele, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
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
sds mympd_api_browse_tag_list(struct t_partition_state *partition_state, sds buffer, long request_id,
        sds searchstr, sds tag, long offset, long limit, bool sortdesc)
{
    size_t searchstr_len = sdslen(searchstr);
    enum mympd_cmd_ids cmd_id = MYMPD_API_DATABASE_TAG_LIST;

    if (mpd_search_db_tags(partition_state->conn, mpd_tag_name_parse(tag)) == false) {
        mpd_search_cancel(partition_state->conn);
        return jsonrpc_respond_message(buffer, cmd_id, request_id, JSONRPC_FACILITY_DATABASE,
            JSONRPC_SEVERITY_ERROR, "Error creating MPD search command");
    }

    long real_limit = offset + limit;
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
                while (raxTryInsert(taglist, (unsigned char *)key, sdslen(key), data, NULL) == 0) {
                    //duplicate - add chars until it is uniq
                    key = sdscatlen(key, ":", 1);
                }
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
    long entity_count = 0;
    long entities_returned = 0;
    raxIterator iter;
    raxStart(&iter, taglist);

    if (sortdesc == false) {
        raxSeek(&iter, "^", NULL, 0);
    }
    else {
        raxSeek(&iter, "$", NULL, 0);
    }
    while (sortdesc == false ? raxNext(&iter) : raxPrev(&iter)) {
        if (entity_count >= offset &&
            entity_count < real_limit)
        {
            if (entities_returned++) {
                buffer = sdscatlen(buffer, ",", 1);
            }
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_sds(buffer, "value", (sds)iter.data, false);
            buffer = sdscatlen(buffer, "}", 1);
        }
        entity_count++;
        FREE_SDS(iter.data);
    }
    raxStop(&iter);
    //checks if this tag has a directory with pictures in /src/lib/mympd/pics
    sds pic_path = sdscatfmt(sdsempty(), "%S/%s/%s", partition_state->mympd_state->config->workdir, DIR_WORK_PICS, tag);
    bool pic = false;
    errno = 0;
    DIR* dir = opendir(pic_path);
    if (dir != NULL) {
        closedir(dir);
        pic = true;
    }
    else {
        MYMPD_LOG_DEBUG(partition_state->name, "Can not open directory \"%s\"", pic_path);
        if (errno != ENOENT) {
            MYMPD_LOG_ERRNO(partition_state->name, errno);
        }
        //ignore error
    }
    FREE_SDS(pic_path);

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_llong(buffer, "totalEntities", (long long)taglist->numele, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_sds(buffer, "searchstr", searchstr, true);
    buffer = tojson_sds(buffer, "tag", tag, true);
    buffer = tojson_bool(buffer, "pics", pic, false);
    buffer = jsonrpc_end(buffer);
    raxFree(taglist);
    return buffer;
}
