/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "../api.h"
#include "../log.h"
#include "../tiny_queue.h"
#include "mpd_client_utility.h"
#include "mpd_client_cover.h"
#include "mpd_client_sticker.h"
#include "mpd_client_browse.h"

sds mpd_client_put_fingerprint(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                               const char *uri)
{
    if (validate_songuri(uri) == false) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Invalid URI", true);
        return buffer;
    }

    buffer = jsonrpc_start_result(buffer, method, request_id);
    
    char fp_buffer[8192];
    const char *fingerprint = mpd_run_getfingerprint_chromaprint(mpd_state->conn, uri, fp_buffer, sizeof(fp_buffer));
    if (fingerprint == NULL) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }
    buffer = sdscat(buffer, ",");
    buffer = tojson_char(buffer, "fingerprint", fingerprint, false);
    mpd_response_finish(mpd_state->conn);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

sds mpd_client_put_songdetails(t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                               const char *uri)
{
    if (validate_songuri(uri) == false) {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Invalid URI", true);
        return buffer;
    }

    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer,",");

    if (!mpd_send_list_all_meta(mpd_state->conn, uri)) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }

    struct mpd_song *song;
    if ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
        buffer = put_song_tags(buffer, mpd_state, &mpd_state->mympd_tag_types, song);
        mpd_song_free(song);
    }
    else {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }
    mpd_response_finish(mpd_state->conn);

    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    
    if (mpd_state->feat_sticker) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        assert(sticker);
        mpd_client_get_sticker(mpd_state, uri, sticker);
        buffer = sdscat(buffer, ",");
        buffer = tojson_long(buffer, "playCount", sticker->playCount, true);
        buffer = tojson_long(buffer, "skipCount", sticker->skipCount, true);
        buffer = tojson_long(buffer, "like", sticker->like, true);
        buffer = tojson_long(buffer, "lastPlayed", sticker->lastPlayed, true);
        buffer = tojson_long(buffer, "lastSkipped", sticker->lastSkipped, false);
        FREE_PTR(sticker);
    }
    
    buffer = sdscat(buffer, ",");
    buffer = put_extra_files(mpd_state, buffer, uri);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

sds mpd_client_put_filesystem(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                              const char *path, const unsigned int offset, const char *filter, const t_tags *tagcols)
{
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    
    if (!mpd_send_list_meta(mpd_state->conn, path)) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }

    struct mpd_entity *entity;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            switch (mpd_entity_get_type(entity)) {
                case MPD_ENTITY_TYPE_UNKNOWN: {
                    entity_count--;
                    break;
                }
                case MPD_ENTITY_TYPE_SONG: {
                    const struct mpd_song *song = mpd_entity_get_song(entity);
                    const char *entityName = mpd_client_get_tag(song, MPD_TAG_TITLE);
                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, entityName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*entityName) == 0 )) 
                    {
                        if (entities_returned++) {
                            buffer = sdscat(buffer, ",");
                        }
                        buffer = sdscat(buffer, "{\"Type\":\"song\",");
                        buffer = put_song_tags(buffer, mpd_state, tagcols, song);
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
                        bool smartpls = is_smartpls(config, mpd_state, plName);
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

    mpd_response_finish(mpd_state->conn);
    
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
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

sds mpd_client_put_db_tag(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                          const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter)
{
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");
    
    if (mpd_search_db_tags(mpd_state->conn, mpd_tag_name_parse(mpdtagtype)) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        mpd_search_cancel(mpd_state->conn);
        return buffer;
    }
    if (mpd_tag_name_parse(mpdsearchtagtype) != MPD_TAG_UNKNOWN) {
        if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdsearchtagtype), searchstr) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            mpd_search_cancel(mpd_state->conn);
            return buffer;
        }
    }
    if (mpd_search_commit(mpd_state->conn) == false || check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    struct mpd_pair *pair;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    while ((pair = mpd_recv_pair_tag(mpd_state->conn, mpd_tag_name_parse(mpdtagtype))) != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            if (strcmp(pair->value, "") == 0) {
                entity_count--;
            }
            else if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, pair->value, 1) == 0 ||
                    (strncmp(filter, "0", 1) == 0 && isalpha(*pair->value) == 0 )) 
            {
                if (entities_returned++) {
                    buffer = sdscat(buffer, ",");
                }
                buffer = sdscat(buffer, "{");
                buffer = tojson_char(buffer, "type", mpdtagtype, true);
                buffer = tojson_char(buffer, "value", pair->value, false);
                buffer = sdscat(buffer, "}");
            }
            else {
                entity_count--;
            }
        }
        mpd_return_pair(mpd_state->conn, pair);
    }

    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_char(buffer, "filter", filter, true);
    buffer = tojson_char(buffer, "searchstr", searchstr, true);
    buffer = tojson_char(buffer, "searchtagtype", mpdsearchtagtype, true);
    buffer = tojson_char(buffer, "tagtype", mpdtagtype, false);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

sds mpd_client_put_songs_in_album(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                                  const char *album, const char *search, const char *tag, const t_tags *tagcols)
{
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");

    if (mpd_search_db_songs(mpd_state->conn, true) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        mpd_search_cancel(mpd_state->conn);
        return buffer;
    }    
    if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(tag), search) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        mpd_search_cancel(mpd_state->conn);
        return buffer;
    }
    if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        mpd_search_cancel(mpd_state->conn);
        return buffer;
    }
    if (mpd_search_commit(mpd_state->conn) == false || check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        return buffer;
    }

    struct mpd_song *song;
    struct mpd_song *first_song = NULL;
    int entity_count = 0;
    int entities_returned = 0;
    int totalTime = 0;

    while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
        entity_count++;
        if (entities_returned++) {
            buffer = sdscat(buffer, ",");
        }
        else {
            first_song = mpd_song_dup(song);
        }
        buffer = sdscat(buffer, "{\"Type\": \"song\",");
        buffer = put_song_tags(buffer, mpd_state, tagcols, song);
        buffer = sdscat(buffer, "}");

        totalTime += mpd_song_get_duration(song);
        mpd_song_free(song);
    }
    mpd_response_finish(mpd_state->conn);

    char *albumartist = NULL;
    if (first_song != NULL) {
        albumartist = mpd_client_get_tag(first_song, MPD_TAG_ALBUM_ARTIST);
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
    
    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }
    
    return buffer;    
}

sds mpd_client_put_firstsong_in_albums(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                                       const char *searchstr, const char *tag, const char *sort, bool sortdesc, const unsigned int offset)
{
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, ",\"data\":[");

    if (mpd_search_db_songs(mpd_state->conn, false) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        mpd_search_cancel(mpd_state->conn);
        return buffer;
    }
    sds expression = sdscatprintf(sdsempty(), "((Track == '%d')", config->covergridminsongs);
    int searchstr_len = strlen(searchstr);
    if (config->regex == true && searchstr_len > 0 && searchstr_len <= 2 && strlen(tag) > 0) {
        expression = sdscatfmt(expression, " AND (%s =~ '^%s')", tag, searchstr);
    }
    else if (strlen(searchstr) > 0 && strlen(tag) > 0) {
        expression = sdscatfmt(expression, " AND (%s contains '%s')", tag, searchstr);
    }
    expression = sdscat(expression, ")");
    if (mpd_search_add_expression(mpd_state->conn, expression) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        mpd_search_cancel(mpd_state->conn);
        return buffer;
    }
    sdsfree(expression);
    if (strlen(sort) > 0) {
        if (mpd_search_add_sort_name(mpd_state->conn, sort, sortdesc) == false) {
            buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
            mpd_search_cancel(mpd_state->conn);
            return buffer;
        }
    }
    if (mpd_search_add_window(mpd_state->conn, offset, offset + mpd_state->max_elements_per_page) == false) {
        buffer = check_error_and_recover(mpd_state, buffer, method, request_id);
        mpd_search_cancel(mpd_state->conn);
        return buffer;
    }
    if (mpd_search_commit(mpd_state->conn) == false || check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    struct mpd_song *song;
    int entity_count = 0;
    int entities_returned = 0;

    while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
        entity_count++;
        if (entities_returned++) {
            buffer = sdscat(buffer, ",");
        }
        buffer = sdscat(buffer, "{\"Type\": \"album\",");
        buffer = tojson_char(buffer, "Album", mpd_client_get_tag(song, MPD_TAG_ALBUM), true);
        buffer = tojson_char(buffer, "AlbumArtist", mpd_client_get_tag(song, MPD_TAG_ALBUM_ARTIST), true);
        buffer = tojson_char(buffer, "FirstSongUri", mpd_song_get_uri(song), false);
        buffer = sdscat(buffer, "}");

        mpd_song_free(song);
    }
    mpd_response_finish(mpd_state->conn);

    if (check_error_and_recover2(mpd_state, &buffer, method, request_id, false) == false) {
        return buffer;
    }

    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", -1, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end_result(buffer);
        
    return buffer;    
}
