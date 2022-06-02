/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "mympd_api_browse.h"

#include "../../dist/utf8/utf8.h"
#include "../lib/jsonrpc.h"
#include "../lib/log.h"
#include "../lib/mem.h"
#include "../lib/mimetype.h"
#include "../lib/mympd_configuration.h"
#include "../lib/sds_extras.h"
#include "../lib/validate.h"
#include "../mpd_shared/mpd_shared_search.h"
#include "../mpd_shared/mpd_shared_sticker.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "mympd_api_utility.h"
#include "mympd_api_albumart.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <libgen.h>
#include <string.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

//private definitions
static bool _search_song(struct mpd_song *song, struct t_list *expr_list, struct t_tags *browse_tag_types);
static pcre2_code *_compile_regex(char *regex_str);
static bool _cmp_regex(pcre2_code *re_compiled, const char *value);
static bool search_dir_entry(rax *rt, sds key, sds entity_name, struct mpd_entity *entity, sds searchstr);

struct t_dir_entry {
    sds name;
    struct mpd_entity *entity;
};

//public functions
sds mympd_api_browse_fingerprint(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                                 const char *uri)
{
    char fp_buffer[8192];
    const char *fingerprint = mpd_run_getfingerprint_chromaprint(mympd_state->mpd_state->conn, uri, fp_buffer, sizeof(fp_buffer));
    if (fingerprint == NULL) {
        check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false);
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = tojson_char(buffer, "fingerprint", fingerprint, false);
    buffer = jsonrpc_result_end(buffer);

    mpd_response_finish(mympd_state->mpd_state->conn);
    check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false);

    return buffer;
}

sds mympd_api_browse_songdetails(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                               const char *uri)
{
    bool rc = mpd_send_list_meta(mympd_state->mpd_state->conn, uri);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_meta") == false) {
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);

    struct mpd_song *song;
    if ((song = mpd_recv_song(mympd_state->mpd_state->conn)) != NULL) {
        const struct mpd_audio_format *audioformat = mpd_song_get_audio_format(song);
        buffer = printAudioFormat(buffer, audioformat);
        buffer = sdscatlen(buffer, ",", 1);
        buffer = get_song_tags(buffer, mympd_state->mpd_state, &mympd_state->mpd_state->tag_types_mympd, song);
        mpd_song_free(song);
    }

    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    if (mympd_state->mpd_state->feat_mpd_stickers) {
        buffer = sdscatlen(buffer, ",", 1);
        buffer = mpd_shared_sticker_list(buffer, mympd_state->sticker_cache, uri);
    }

    buffer = sdscatlen(buffer, ",", 1);
    buffer = get_extra_files(mympd_state, buffer, uri, false);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

sds mympd_api_browse_read_comments(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                             const char *uri)
{
    bool rc = mpd_send_read_comments(mympd_state->mpd_state->conn, uri);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_meta") == false) {
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":{");
    struct mpd_pair *pair;
    int entities_returned = 0;
    while ((pair = mpd_recv_pair(mympd_state->mpd_state->conn)) != NULL) {
        if (entities_returned++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        buffer = tojson_char(buffer, pair->name, pair->value,false);
        mpd_return_pair(mympd_state->mpd_state->conn, pair);
    }
    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    buffer = sdscatlen(buffer, "},", 2);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "totalEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);
    return buffer;
}

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
    sds_utf8_tolower(searchstr);
    while ((entity = mpd_recv_entity(mympd_state->mpd_state->conn)) != NULL) {
        switch (mpd_entity_get_type(entity)) {
            case MPD_ENTITY_TYPE_SONG: {
                const struct mpd_song *song = mpd_entity_get_song(entity);
                sds entity_name =  mpd_shared_get_tag_values(song, MPD_TAG_TITLE, sdsempty());
                key = sdscatfmt(key, "2%s", mpd_song_get_uri(song));
                search_dir_entry(entity_list, key, entity_name, entity, searchstr);
                break;
            }
            case MPD_ENTITY_TYPE_DIRECTORY: {
                const struct mpd_directory *dir = mpd_entity_get_directory(entity);
                sds entity_name = sdsnew(mpd_directory_get_path(dir));
                sds_basename_uri(entity_name);
                key = sdscatfmt(key, "0%S", entity_name);
                search_dir_entry(entity_list, key, entity_name, entity, searchstr);
                break;
            }
            case MPD_ENTITY_TYPE_PLAYLIST: {
                const struct mpd_playlist *pl = mpd_entity_get_playlist(entity);
                const char *pl_path = mpd_playlist_get_path(pl);
                if (path[0] == '/') {
                    //do not show mpd playlists in root directory
                    char *ext = strrchr(pl_path, '.');
                    if (ext == NULL ||
                        (strcmp(ext, ".m3u") != 0 && strcmp(ext, ".pls") != 0))
                    {
                        mpd_entity_free(entity);
                        break;
                    }
                }
                sds entity_name = sdsnew(pl_path);
                sds_basename_uri(entity_name);
                key = sdscatfmt(key, "1%S", entity_name);
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
            sdsfree(entry_data->name);
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
        free(path_cpy);
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
                    sds_basename_uri(filename);
                    buffer = tojson_char(buffer, "Filename", filename, false);
                    FREE_SDS(filename);
                    if (mympd_state->mpd_state->feat_mpd_stickers) {
                        buffer = sdscatlen(buffer, ",", 1);
                        buffer = mpd_shared_sticker_list(buffer, mympd_state->sticker_cache, mpd_song_get_uri(song));
                    }
                    buffer = sdscatlen(buffer, "}", 1);
                    break;
                }
                case MPD_ENTITY_TYPE_DIRECTORY: {
                    const struct mpd_directory *dir = mpd_entity_get_directory(entry_data->entity);
                    buffer = sdscat(buffer, "{\"Type\":\"dir\",");
                    buffer = tojson_char(buffer, "uri", mpd_directory_get_path(dir), true);
                    buffer = tojson_char(buffer, "name", entry_data->name, true);
                    buffer = tojson_char(buffer, "Filename", entry_data->name, false);
                    buffer = sdscatlen(buffer, "}", 1);
                    break;
                }
                case MPD_ENTITY_TYPE_PLAYLIST: {
                    const struct mpd_playlist *pl = mpd_entity_get_playlist(entry_data->entity);
                    bool smartpls = is_smartpls(mympd_state->config->workdir, entry_data->name);
                    buffer = sdscatfmt(buffer, "{\"Type\": \"%s\",", (smartpls == true ? "smartpls" : "plist"));
                    buffer = tojson_char(buffer, "uri", mpd_playlist_get_path(pl), true);
                    buffer = tojson_char(buffer, "name", entry_data->name, true);
                    buffer = tojson_char(buffer, "Filename", entry_data->name, false);
                    buffer = sdscatlen(buffer, "}", 1);
                    break;
                }
                default:
                    break;
            }
        }
        mpd_entity_free(entry_data->entity);
        sdsfree(entry_data->name);
        FREE_PTR(iter.data);
        entity_count++;
    }
    raxStop(&iter);
    buffer = sdscatlen(buffer, "],", 2);
    buffer = get_extra_files(mympd_state, buffer, path, true);
    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_llong(buffer, "totalEntities", (long long)entity_list->numele, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_char(buffer, "search", searchstr, false);
    buffer = jsonrpc_result_end(buffer);
    raxFree(entity_list);
    return buffer;
}

sds mympd_api_browse_album_songs(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                                 sds album, struct t_list *albumartists, const struct t_tags *tagcols)
{
    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");

    bool rc = mpd_search_db_songs(mympd_state->mpd_state->conn, true);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_db_songs") == false) {
        mpd_search_cancel(mympd_state->mpd_state->conn);
        return buffer;
    }

    sds expression = sdsnewlen("(", 1);
    struct t_list_node *current = albumartists->head;
    while (current != NULL) {
        expression = escape_mpd_search_expression(expression, mpd_tag_name(mympd_state->mpd_state->tag_albumartist), "==", current->key);
        expression = sdscat(expression, " AND ");
        current = current->next;
    }
    expression = escape_mpd_search_expression(expression, "Album", "==", album);
    expression = sdscatlen(expression, ")", 1);

    rc = mpd_search_add_expression(mympd_state->mpd_state->conn, expression);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_expression") == false) {
        mpd_search_cancel(mympd_state->mpd_state->conn);
        FREE_SDS(expression);
        return buffer;
    }
    FREE_SDS(expression);
    rc = mpd_search_add_sort_tag(mympd_state->mpd_state->conn, MPD_TAG_DISC, false);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_sort_tag") == false) {
        mpd_search_cancel(mympd_state->mpd_state->conn);
        return buffer;
    }
    rc = mpd_search_add_window(mympd_state->mpd_state->conn, 0, MPD_RESULTS_MAX);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_window") == false) {
        mpd_search_cancel(mympd_state->mpd_state->conn);
        return buffer;
    }
    rc = mpd_search_commit(mympd_state->mpd_state->conn);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_commit") == false) {
        buffer = check_error_and_recover(mympd_state->mpd_state, buffer, method, request_id);
        return buffer;
    }

    struct mpd_song *song;
    struct mpd_song *first_song = NULL;
    int entity_count = 0;
    int entities_returned = 0;
    unsigned totalTime = 0;
    int discs = 1;

    while ((song = mpd_recv_song(mympd_state->mpd_state->conn)) != NULL) {
        if (entities_returned++) {
            buffer = sdscatlen(buffer, ",", 1);
        }
        else {
            first_song = mpd_song_dup(song);
        }
        const char *disc;
        if ((disc = mpd_song_get_tag(song, MPD_TAG_DISC, 0)) != NULL) {
            int d = (int)strtoimax(disc, NULL, 10);
            if (d > discs) {
                discs = d;
            }
        }
        buffer = sdscat(buffer, "{\"Type\": \"song\",");
        buffer = get_song_tags(buffer, mympd_state->mpd_state, tagcols, song);
        if (mympd_state->mpd_state->feat_mpd_stickers) {
            buffer = sdscatlen(buffer, ",", 1);
            buffer = mpd_shared_sticker_list(buffer, mympd_state->sticker_cache, mpd_song_get_uri(song));
        }
        buffer = sdscatlen(buffer, "}", 1);

        totalTime += mpd_song_get_duration(song);
        mpd_song_free(song);
        entity_count++;
    }

    buffer = sdscatlen(buffer, "],", 2);

    sds albumartist = sdsempty();
    sds mb_albumartist_id = sdsempty();
    sds mb_album_id = sdsempty();
    if (first_song != NULL) {
        buffer = get_extra_files(mympd_state, buffer, mpd_song_get_uri(first_song), false);
        albumartist = mpd_shared_get_tag_values(first_song, MPD_TAG_ALBUM_ARTIST, albumartist);
        mb_albumartist_id = mpd_shared_get_tag_values(first_song, MPD_TAG_MUSICBRAINZ_ALBUMARTISTID, mb_albumartist_id);
        mb_album_id = mpd_shared_get_tag_values(first_song, MPD_TAG_MUSICBRAINZ_ALBUMID, mb_album_id);
    }
    else {
        //this should not occur, but we should response with a complete object
        buffer = sdscat(buffer, "\"images\":[],\"bookletPath\":\"\"");
        albumartist = sdscatlen(albumartist, "\"\"", 2);
        mb_albumartist_id = sdscatlen(mb_albumartist_id, "\"\"", 2);
        mb_album_id = sdscatlen(mb_album_id, "\"\"", 2);
    }

    buffer = sdscatlen(buffer, ",", 1);
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_char(buffer, "Album", album, true);
    buffer = tojson_raw(buffer, "AlbumArtist", albumartist, true);
    buffer = tojson_raw(buffer, "MusicBrainzAlbumArtistId", mb_albumartist_id, true);
    buffer = tojson_raw(buffer, "MusicBrainzAlbumId", mb_album_id, true);
    buffer = tojson_raw(buffer, "AlbumArtist", albumartist, true);
    buffer = tojson_long(buffer, "Discs", discs, true);
    buffer = tojson_uint(buffer, "totalTime", totalTime, false);
    buffer = jsonrpc_result_end(buffer);

    FREE_SDS(albumartist);
    FREE_SDS(mb_albumartist_id);
    FREE_SDS(mb_album_id);
    if (first_song != NULL) {
        mpd_song_free(first_song);
    }
    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    return buffer;
}

sds mympd_api_browse_album_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                                       sds expression, sds sort, bool sortdesc, const long offset, long limit)
{
    if (mympd_state->album_cache == NULL) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, true, "database", "error", "Albumcache not ready");
        return buffer;
    }

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");

    struct mpd_song *song;
    //parse sort tag
    bool sort_by_last_modified = false;
    enum mpd_tag_type sort_tag = MPD_TAG_ALBUM;

    if (sdslen(sort) > 0) {
        enum mpd_tag_type sort_tag_org = mpd_tag_name_parse(sort);
        if (sort_tag_org != MPD_TAG_UNKNOWN) {
            sort_tag = get_sort_tag(sort_tag_org);
            if (mpd_shared_tag_exists(mympd_state->mpd_state->tag_types_mympd.tags, mympd_state->mpd_state->tag_types_mympd.len, sort_tag) == false) {
                //sort tag is not enabled, revert
                sort_tag = sort_tag_org;
            }
        }
        else if (strcmp(sort, "LastModified") == 0) {
            sort_by_last_modified = true;
        }
        else {
            MYMPD_LOG_WARN("Unknown sort tag: %s", sort);
        }
    }
    //parse mpd search expression
    struct t_list expr_list;
    list_init(&expr_list);
    int count = 0;
    sds *tokens = sdssplitlen(expression, (ssize_t)sdslen(expression), ") AND (", 7, &count);
    for (int j = 0; j < count; j++) {
        sdstrim(tokens[j], "() ");
        sds tag = sdsempty();
        sds op = sdsempty();
        sds value = sdsempty();
        size_t i = 0;
        char *p = tokens[j];
        //tag
        for (i = 0; i < sdslen(tokens[j]); i++, p++) {
            if (tokens[j][i] == ' ') {
                break;
            }
            tag = sdscatfmt(tag, "%c", *p);
        }
        if (i + 1 >= sdslen(tokens[j])) {
            MYMPD_LOG_ERROR("Can not parse search expression");
            FREE_SDS(tag);
            FREE_SDS(op);
            FREE_SDS(value);
            break;
        }
        i++;
        p++;
        //operator
        for (; i < sdslen(tokens[j]); i++, p++) {
            if (tokens[j][i] == ' ') {
                break;
            }
            op = sdscatfmt(op, "%c", *p);
        }
        if (i + 2 >= sdslen(tokens[j])) {
            MYMPD_LOG_ERROR("Can not parse search expression");
            FREE_SDS(tag);
            FREE_SDS(op);
            FREE_SDS(value);
            break;
        }
        i = i + 2;
        p = p + 2;
        //value
        for (; i < sdslen(tokens[j]) - 1; i++, p++) {
            value = sdscatfmt(value, "%c", *p);
        }
        int tag_type = mpd_tag_name_parse(tag);
        if (tag_type == -1 && strcmp(tag, "any") == 0) {
            tag_type = -2;
        }
        if (strcmp(op, "=~") == 0 || strcmp(op, "!~") == 0) {
            //is regex, compile
            pcre2_code *re_compiled = _compile_regex(value);
            list_push(&expr_list, value, tag_type, op , re_compiled);
        }
        else {
            list_push(&expr_list, value, tag_type, op , NULL);
        }
        MYMPD_LOG_DEBUG("Parsed expression tag: \"%s\", op: \"%s\", value:\"%s\"", tag, op, value);
        FREE_SDS(tag);
        FREE_SDS(op);
        FREE_SDS(value);
    }
    sdsfreesplitres(tokens, count);

    //search and sort albumlist
    long real_limit = offset + limit;
    rax *albums = raxNew();
    raxIterator iter;
    raxStart(&iter, mympd_state->album_cache);
    raxSeek(&iter, "^", NULL, 0);
    sds key = sdsempty();
    while (raxNext(&iter)) {
        song = (struct mpd_song *)iter.data;
        if (expr_list.length == 0 ||
            _search_song(song, &expr_list, &mympd_state->tag_types_browse) == true)
        {
            if (sort_by_last_modified == true) {
                key = sdscatfmt(key, "%I::%s", (long long)mpd_song_get_last_modified(song), mpd_song_get_uri(song));
            }
            else {
                const char *sort_value = mpd_song_get_tag(song, sort_tag, 0);
                if (sort_value == NULL &&
                    sort_tag == MPD_TAG_ALBUM_ARTIST)
                {
                    //fallback to artist tag if albumartist tag is not set
                    sort_value = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
                }
                if (sort_value != NULL) {
                    key = sdscatfmt(key, "%s::%s", sort_value, mpd_song_get_uri(song));
                }
                else {
                    //sort tag not present, append to end of the list
                    key = sdscatfmt(key, "zzzzzzzzzz::%s", mpd_song_get_uri(song));
                }
            }
            sds_utf8_tolower(key);
            raxInsert(albums,(unsigned char*)key, sdslen(key), iter.data, NULL);
            sdsclear(key);
        }
    }
    raxStop(&iter);
    list_clear(&expr_list);
    sdsfree(key);
    //print album list
    long entity_count = 0;
    long entities_returned = 0;
    sds album = sdsempty();
    sds artist = sdsempty();
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
            song = (struct mpd_song *)iter.data;
            album = mpd_shared_get_tag_values(song, MPD_TAG_ALBUM, album);
            artist = mpd_shared_get_tag_values(song, mympd_state->mpd_state->tag_albumartist, artist);
            buffer = sdscatlen(buffer, "{", 1);
            buffer = tojson_char(buffer, "Type", "album", true);
            buffer = tojson_raw(buffer, "Album", album, true);
            buffer = tojson_raw(buffer, "AlbumArtist", artist, true);
            buffer = tojson_char(buffer, "FirstSongUri", mpd_song_get_uri(song), false);
            buffer = sdscatlen(buffer, "}", 1);
        }
        entity_count++;
        if (entity_count == real_limit) {
            break;
        }
    }
    FREE_SDS(album);
    FREE_SDS(artist);
    raxStop(&iter);

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_llong(buffer, "totalEntities", (long long)albums->numele, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_char(buffer, "expression", expression, true);
    buffer = tojson_char(buffer, "sort", sort, true);
    buffer = tojson_bool(buffer, "sortdesc", sortdesc, true);
    buffer = tojson_char(buffer, "tag", "Album", false);
    buffer = jsonrpc_result_end(buffer);
    raxFree(albums);
    return buffer;
}

sds mympd_api_browse_tag_list(struct t_mympd_state *mympd_state, sds buffer, sds method, long request_id,
                          sds searchstr, sds tag, const long offset, const long limit, bool sortdesc)
{
    size_t searchstr_len = sdslen(searchstr);

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");

    bool rc = mpd_search_db_tags(mympd_state->mpd_state->conn, mpd_tag_name_parse(tag));
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_db_tags") == false) {
        mpd_search_cancel(mympd_state->mpd_state->conn);
        return buffer;
    }

    rc = mpd_search_commit(mympd_state->mpd_state->conn);
    if (check_rc_error_and_recover(mympd_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_commit") == false) {
        return buffer;
    }

    struct mpd_pair *pair;
    enum mpd_tag_type mpdtag = mpd_tag_name_parse(tag);
    long real_limit = offset + limit;
    rax *taglist = raxNew();
    sds key = sdsempty();
    //filter and sort
    while ((pair = mpd_recv_pair_tag(mympd_state->mpd_state->conn, mpdtag)) != NULL) {
        if (pair->value[0] == '\0') {
            MYMPD_LOG_DEBUG("Value is empty, skipping");
        }
        else if (searchstr_len == 0 ||
            (searchstr_len <= 2 && utf8ncasecmp(searchstr, pair->value, searchstr_len) == 0) ||
            (searchstr_len > 2 && utf8casestr(pair->value, searchstr) != NULL))
        {
            key = sdscat(key, pair->value);
            sds_utf8_tolower(key);
            raxInsert(taglist, (unsigned char *)key, sdslen(key), sdsnew(pair->value), NULL);
            sdsclear(key);
        }
        mpd_return_pair(mympd_state->mpd_state->conn, pair);
    }
    mpd_response_finish(mympd_state->mpd_state->conn);
    if (check_error_and_recover2(mympd_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    FREE_SDS(key);
    //print list
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
            buffer = tojson_char(buffer, "value", (sds)iter.data, false);
            buffer = sdscatlen(buffer, "}", 1);
        }
        entity_count++;
        FREE_SDS(iter.data);
    }
    raxStop(&iter);
    //checks if this tag has a directory with pictures in /var/lib/mympd/pics
    sds pic_path = sdscatfmt(sdsempty(), "%S/pics/%s", mympd_state->config->workdir, tag);
    bool pic = false;
    errno = 0;
    DIR* dir = opendir(pic_path);
    if (dir != NULL) {
        closedir(dir);
        pic = true;
    }
    else {
        MYMPD_LOG_DEBUG("Can not open directory \"%s\"", pic_path);
        if (errno != ENOENT) {
            MYMPD_LOG_ERRNO(errno);
        }
        //ignore error
    }
    FREE_SDS(pic_path);

    buffer = sdscatlen(buffer, "],", 2);
    buffer = tojson_llong(buffer, "totalEntities", (long long)taglist->numele, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_char(buffer, "searchstr", searchstr, true);
    buffer = tojson_char(buffer, "tag", tag, true);
    buffer = tojson_bool(buffer, "pics", pic, false);
    buffer = jsonrpc_result_end(buffer);
    raxFree(taglist);
    return buffer;
}

//private functions
static bool search_dir_entry(rax *rt, sds key, sds entity_name, struct mpd_entity *entity, sds searchstr) {
    sds_utf8_tolower(key);
    if (sdslen(searchstr) == 0 ||
        utf8str(entity_name, searchstr) != NULL)
    {
        struct t_dir_entry *entry_data = malloc_assert(sizeof(struct t_dir_entry));
        entry_data->name = entity_name;
        entry_data->entity = entity;
        raxInsert(rt, (unsigned char *)key, sdslen(key), entry_data, NULL);
        return true;
    }
    mpd_entity_free(entity);
    sdsfree(entity_name);
    return false;
}

static bool _search_song(struct mpd_song *song, struct t_list *expr_list, struct t_tags *browse_tag_types) {
    (void) browse_tag_types;
    struct t_tags one_tag;
    one_tag.len = 1;
    struct t_list_node *current = expr_list->head;
    while (current != NULL) {
        struct t_tags *tags = NULL;
        if (current->value_i == -2) {
            //any - use all browse tags
            tags = browse_tag_types;
        }
        else {
            //use selected tag only
            tags = &one_tag;
            tags->tags[0] = (enum mpd_tag_type)current->value_i;
        }
        bool rc = false;
        for (size_t i = 0; i < tags->len; i++) {
            rc = true;
            unsigned j = 0;
            const char *value = NULL;
            while ((value = mpd_song_get_tag(song, tags->tags[i], j)) != NULL) {
                j++;
                if ((strcmp(current->value_p, "contains") == 0 && utf8casestr(value, current->key) == NULL) ||
                    (strcmp(current->value_p, "starts_with") == 0 && utf8ncasecmp(current->key, value, sdslen(current->key)) != 0) ||
                    (strcmp(current->value_p, "==") == 0 && utf8casecmp(value, current->key) != 0) ||
                    (strcmp(current->value_p, "!=") == 0 && utf8casecmp(value, current->key) == 0) ||
                    (strcmp(current->value_p, "=~") == 0 && _cmp_regex((pcre2_code *)current->user_data, value) == false) ||
                    (strcmp(current->value_p, "!~") == 0 && _cmp_regex((pcre2_code *)current->user_data, value) == true))
                {
                    rc = false;
                }
                else {
                    //tag value matched
                    rc = true;
                    break;
                }
            }
            if (j == 0) {
                rc = false;
            }
            if (rc == true) {
                break;
            }
        }
        if (rc == false) {
            return false;
        }
        current = current->next;
    }
    return true;
}

static pcre2_code *_compile_regex(char *regex_str) {
    MYMPD_LOG_DEBUG("Compiling regex: \"%s\"", regex_str);
    utf8lwr(regex_str);
    PCRE2_SIZE erroroffset;
    int rc;
    pcre2_code *re_compiled = pcre2_compile(
        (PCRE2_SPTR)regex_str, /* the pattern */
        PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
        0,                     /* default options */
        &rc,		       /* for error number */
        &erroroffset,          /* for error offset */
        NULL                   /* use default compile context */
    );
    if (re_compiled == NULL){
        //Compilation failed
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(rc, buffer, sizeof(buffer));
        MYMPD_LOG_ERROR("PCRE2 compilation failed at offset %d: \"%s\"", (int)erroroffset, buffer);
        return NULL;
    }
    return re_compiled;
}

static bool _cmp_regex(pcre2_code *re_compiled, const char *value) {
    if (re_compiled == NULL) {
        return false;
    }
    char *lower = strdup(value);
    utf8lwr(lower);
    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re_compiled, NULL);
    int rc = pcre2_match(
        re_compiled,          /* the compiled pattern */
        (PCRE2_SPTR)lower,    /* the subject string */
        strlen(value),        /* the length of the subject */
        0,                    /* start at offset 0 in the subject */
        0,                    /* default options */
        match_data,           /* block for storing the result */
        NULL                  /* use default match context */
    );
    pcre2_match_data_free(match_data);
    free(lower);
    if (rc >= 0) {
        return true;
    }
    //Matching failed: handle error cases
    switch(rc) {
        case PCRE2_ERROR_NOMATCH: 
            break;
        default: {
            PCRE2_UCHAR buffer[256];
            pcre2_get_error_message(rc, buffer, sizeof(buffer));
            MYMPD_LOG_ERROR("PCRE2 matching error %d: \"%s\"", rc, buffer);
        }
    }
    return false;
}
