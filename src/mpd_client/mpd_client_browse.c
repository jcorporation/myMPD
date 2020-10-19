/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define _GNU_SOURCE 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../api.h"
#include "../log.h"
#include "../tiny_queue.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "mpd_client_utility.h"
#include "mpd_client_cover.h"
#include "mpd_client_sticker.h"
#include "mpd_client_browse.h"

sds mpd_client_put_fingerprint(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id,
                               const char *uri)
{
    if (validate_songuri(uri) == false) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Invalid URI", true);
        return buffer;
    }
    
    char fp_buffer[8192];
    const char *fingerprint = mpd_run_getfingerprint_chromaprint(mpd_client_state->mpd_state->conn, uri, fp_buffer, sizeof(fp_buffer));
    if (fingerprint == NULL) {
        check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false);
        return buffer;
    }
    
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",");
    buffer = tojson_char(buffer, "fingerprint", fingerprint, false);
    buffer = jsonrpc_end_result(buffer);
    
    mpd_response_finish(mpd_client_state->mpd_state->conn);
    check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false);
    
    return buffer;
}

sds mpd_client_put_songdetails(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, 
                               const char *uri)
{
    if (validate_songuri(uri) == false) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Invalid URI", true);
        return buffer;
    }

    bool rc = mpd_send_list_meta(mpd_client_state->mpd_state->conn, uri);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_meta") == false) {
        return buffer;
    }

    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer,",");

    struct mpd_song *song;
    if ((song = mpd_recv_song(mpd_client_state->mpd_state->conn)) != NULL) {
        buffer = put_song_tags(buffer, mpd_client_state->mpd_state, &mpd_client_state->mpd_state->mympd_tag_types, song);
        mpd_song_free(song);
    }

    mpd_response_finish(mpd_client_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    
    if (mpd_client_state->feat_sticker) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        assert(sticker);
        mpd_client_get_sticker(mpd_client_state, uri, sticker);
        buffer = sdscat(buffer, ",");
        buffer = tojson_long(buffer, "playCount", sticker->playCount, true);
        buffer = tojson_long(buffer, "skipCount", sticker->skipCount, true);
        buffer = tojson_long(buffer, "like", sticker->like, true);
        buffer = tojson_long(buffer, "lastPlayed", sticker->lastPlayed, true);
        buffer = tojson_long(buffer, "lastSkipped", sticker->lastSkipped, false);
        FREE_PTR(sticker);
    }
    
    buffer = sdscat(buffer, ",");
    buffer = put_extra_files(mpd_client_state, buffer, uri);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

sds mpd_client_put_filesystem(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, 
                              const char *path, const unsigned int offset, const char *filter, const t_tags *tagcols)
{
    bool rc = mpd_send_list_meta(mpd_client_state->mpd_state->conn, path);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_send_list_meta") == false) {
        return buffer;
    }
    
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    
    struct mpd_entity *entity;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    while ((entity = mpd_recv_entity(mpd_client_state->mpd_state->conn)) != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_client_state->max_elements_per_page) {
            switch (mpd_entity_get_type(entity)) {
                case MPD_ENTITY_TYPE_UNKNOWN: {
                    entity_count--;
                    break;
                }
                case MPD_ENTITY_TYPE_SONG: {
                    const struct mpd_song *song = mpd_entity_get_song(entity);
                    const char *entityName = mpd_shared_get_tag(song, MPD_TAG_TITLE);
                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, entityName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*entityName) == 0 )) 
                    {
                        if (entities_returned++) {
                            buffer = sdscat(buffer, ",");
                        }
                        buffer = sdscat(buffer, "{\"Type\":\"song\",");
                        buffer = put_song_tags(buffer, mpd_client_state->mpd_state, tagcols, song);
                        buffer = sdscat(buffer, "}");
                    }
                    else {
                        entity_count--;
                    }
                    break;
                }
                case MPD_ENTITY_TYPE_DIRECTORY: {
                    const struct mpd_directory *dir = mpd_entity_get_directory(entity);                
                    const char *entityName = mpd_directory_get_path(dir);
                    char *dirName = strrchr(entityName, '/');

                    if (dirName != NULL) {
                        dirName++;
                    }
                    else {
                        dirName = (char *)entityName;
                    }

                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, dirName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*dirName) == 0 )) 
                    {                
                        if (entities_returned++) {
                            buffer = sdscat(buffer, ",");
                        }
                        buffer = sdscat(buffer, "{\"Type\":\"dir\",");
                        buffer = tojson_char(buffer, "uri", entityName, true);
                        buffer = tojson_char(buffer, "name", dirName, false);
                        buffer = sdscat(buffer, "}");
                    }
                    else {
                        entity_count--;
                    }
                    dirName = NULL;
                    break;
                }
                case MPD_ENTITY_TYPE_PLAYLIST: {
                    const struct mpd_playlist *pl = mpd_entity_get_playlist(entity);
                    const char *entityName = mpd_playlist_get_path(pl);
                    if (strchr(entityName, '.') == NULL) {
                        entity_count--;
                        break;
                    }
                    char *plName = strrchr(entityName, '/');
                    if (plName != NULL) {
                        plName++;
                    } else {
                        plName = (char *)entityName;
                    }
                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, plName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*plName) == 0 )) 
                    {
                        if (entities_returned++) {
                            buffer = sdscat(buffer, ",");
                        }
                        bool smartpls = is_smartpls(config, mpd_client_state, plName);
                        buffer = sdscatfmt(buffer, "{\"Type\": \"%s\",", (smartpls == true ? "smartpls" : "plist"));
                        buffer = tojson_char(buffer, "uri", entityName, true);
                        buffer = tojson_char(buffer, "name", plName, false);
                        buffer = sdscat(buffer, "}");
                    } else {
                        entity_count--;
                    }
                    plName = NULL;
                    break;
                }
            }
        }
        mpd_entity_free(entity);
    }

    mpd_response_finish(mpd_client_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_char(buffer, "filter", filter, false);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

sds mpd_client_put_songs_in_album(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id,
                                  const char *album, const char *search, const char *tag, const t_tags *tagcols)
{
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");

    bool rc = mpd_search_db_songs(mpd_client_state->mpd_state->conn, true);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_db_songs") == false) {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        return buffer;
    }    
    rc = mpd_search_add_tag_constraint(mpd_client_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(tag), search);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_tag_constraint") == false) {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        return buffer;
    }
    rc = mpd_search_add_tag_constraint(mpd_client_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_tag_constraint") == false) {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        return buffer;
    }
    rc = mpd_search_commit(mpd_client_state->mpd_state->conn);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_commit") == false) {
        buffer = check_error_and_recover(mpd_client_state->mpd_state, buffer, method, request_id);
        return buffer;
    }

    struct mpd_song *song;
    struct mpd_song *first_song = NULL;
    int entity_count = 0;
    int entities_returned = 0;
    unsigned int totalTime = 0;

    while ((song = mpd_recv_song(mpd_client_state->mpd_state->conn)) != NULL) {
        entity_count++;
        if (entities_returned++) {
            buffer = sdscat(buffer, ",");
        }
        else {
            first_song = mpd_song_dup(song);
        }
        buffer = sdscat(buffer, "{\"Type\": \"song\",");
        buffer = put_song_tags(buffer, mpd_client_state->mpd_state, tagcols, song);
        buffer = sdscat(buffer, "}");

        totalTime += mpd_song_get_duration(song);
        mpd_song_free(song);
    }

    char *albumartist = NULL;
    if (first_song != NULL) {
        albumartist = mpd_shared_get_tag(first_song, MPD_TAG_ALBUM_ARTIST);
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_char(buffer, "Album", album, true);
    buffer = tojson_char(buffer, "search", search, true);
    buffer = tojson_char(buffer, "tag", tag, true);
    buffer = tojson_char(buffer, "AlbumArtist", (albumartist != NULL ? albumartist : "-"), true);
    buffer = tojson_long(buffer, "totalTime", totalTime, false);
    buffer = jsonrpc_end_result(buffer);
        
    if (first_song != NULL) {
        mpd_song_free(first_song);
    }

    mpd_response_finish(mpd_client_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    
    return buffer;    
}

sds mpd_client_put_firstsong_in_albums(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, 
                                       const char *searchstr, const char *filter, const char *sort, bool sortdesc, const unsigned int offset)
{
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");

    bool rc = mpd_search_db_songs(mpd_client_state->mpd_state->conn, false);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_db_songs") == false) {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        return buffer;
    }
    
    sds expression = sdscatprintf(sdsempty(), "((Track == '%d')", config->covergridminsongs);
/*
    unsigned long searchstr_len = strlen(searchstr);
    if (config->regex == true && searchstr_len > 0 && searchstr_len <= 2 && strlen(filter) > 0) {
        expression = sdscatfmt(expression, " AND (%s =~ '^%s')", filter, searchstr);
    }
    else if (searchstr_len > 0 && strlen(filter) > 0) {
        expression = sdscatfmt(expression, " AND (%s contains '%s')", filter, searchstr);
    }
*/
    if (strlen(searchstr) > 0) {
        expression = sdscat(expression, " AND ");
        expression = sdscat(expression, searchstr);
    }
    expression = sdscat(expression, ")");
    
    rc = mpd_search_add_expression(mpd_client_state->mpd_state->conn, expression);
    sdsfree(expression);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_expression") == false) {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        return buffer;
    }
    if (strlen(sort) > 0) {
        enum mpd_tag_type sort_tag = mpd_tag_name_parse(sort);
        if (sort_tag != MPD_TAG_UNKNOWN) {
            sort_tag = get_sort_tag(sort_tag);
            rc = mpd_search_add_sort_tag(mpd_client_state->mpd_state->conn, sort_tag, sortdesc);
            if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_sort_tag") == false) {
                mpd_search_cancel(mpd_client_state->mpd_state->conn);
                return buffer;
            }
        }
        else if (strcmp(sort, "Last-Modified") == 0) {
            rc = mpd_search_add_sort_name(mpd_client_state->mpd_state->conn, sort, sortdesc);
            if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_sort_name") == false) {
                mpd_search_cancel(mpd_client_state->mpd_state->conn);
                return buffer;
            }
        }
        else {
            LOG_WARN("Unknown sort tag: %s", sort);
        }
    }
    
    rc = mpd_search_add_window(mpd_client_state->mpd_state->conn, offset, offset + mpd_client_state->max_elements_per_page);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_add_window") == false) {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        return buffer;
    }
    
    rc = mpd_search_commit(mpd_client_state->mpd_state->conn);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_commit") == false) {
        return buffer;
    }

    struct mpd_song *song;
    int entity_count = 0;
    int entities_returned = 0;

    while ((song = mpd_recv_song(mpd_client_state->mpd_state->conn)) != NULL) {
        entity_count++;
        if (entities_returned++) {
            buffer = sdscat(buffer, ",");
        }
        buffer = sdscat(buffer, "{\"Type\": \"album\",");
        buffer = tojson_char(buffer, "Album", mpd_shared_get_tag(song, MPD_TAG_ALBUM), true);
        buffer = tojson_char(buffer, "AlbumArtist", mpd_shared_get_tag(song, MPD_TAG_ALBUM_ARTIST), true);
        buffer = tojson_char(buffer, "FirstSongUri", mpd_song_get_uri(song), false);
        buffer = sdscat(buffer, "}");

        mpd_song_free(song);
    }

    mpd_response_finish(mpd_client_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", -1, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_char(buffer, "filter", filter, true);
    buffer = tojson_char(buffer, "searchstr", searchstr, true);
    buffer = tojson_char(buffer, "sort", sort, true);
    buffer = tojson_bool(buffer, "sortdesc", sortdesc, true);
    buffer = tojson_char(buffer, "tag", "Album", false);
    buffer = jsonrpc_end_result(buffer);
        
    return buffer;    
}

sds mpd_client_put_db_tag2(t_config *config, t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, 
                           const char *searchstr, const char *filter, const char *sort, bool sortdesc, const unsigned int offset, const char *tag)
{
    (void) sort;
    (void) sortdesc;
    int searchstr_len = strlen(searchstr);
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
   
    bool rc = mpd_search_db_tags(mpd_client_state->mpd_state->conn, mpd_tag_name_parse(tag));
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_db_tags") == false) {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        return buffer;
    }
    
    rc = mpd_search_commit(mpd_client_state->mpd_state->conn);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, &buffer, method, request_id, false, rc, "mpd_search_commit") == false) {
        return buffer;
    }

    struct mpd_pair *pair;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    enum mpd_tag_type mpdtag = mpd_tag_name_parse(tag);
    while ((pair = mpd_recv_pair_tag(mpd_client_state->mpd_state->conn, mpdtag)) != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_client_state->max_elements_per_page) {
            if (strcmp(pair->value, "") == 0) {
                entity_count--;
            }
            else if (searchstr_len == 0
                     || (searchstr_len <= 2 && strncasecmp(searchstr, pair->value, searchstr_len) == 0)
                     || (searchstr_len > 2 && strcasestr(pair->value, searchstr) != NULL))
            {
                if (entities_returned++) {
                    buffer = sdscat(buffer, ",");
                }
                buffer = sdscat(buffer, "{");
                buffer = tojson_char(buffer, "value", pair->value, false);
                buffer = sdscat(buffer, "}");
            }
            else {
                entity_count--;
            }
        }
        mpd_return_pair(mpd_client_state->mpd_state->conn, pair);
    }
    mpd_response_finish(mpd_client_state->mpd_state->conn);
    if (check_error_and_recover2(mpd_client_state->mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    sds pic_path = sdscatfmt(sdsempty(), "%s/pics/%s", config->varlibdir, tag);
    bool pic = false;
    DIR* dir = opendir(pic_path);
    if (dir) {
        closedir(dir);
        pic = true;
    }
    sdsfree(pic_path);

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", -1, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_char(buffer, "filter", filter, true);
    buffer = tojson_char(buffer, "searchstr", searchstr, true);
    buffer = tojson_char(buffer, "sort", sort, true);
    buffer = tojson_bool(buffer, "sortdesc", sortdesc, true);
    buffer = tojson_char(buffer, "tag", tag, true);
    buffer = tojson_bool(buffer, "pics", pic, false);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}
