/* myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de> This project's
   homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../utility.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "../config_defs.h"
#include "../tiny_queue.h"
#include "mpd_client_utility.h"
#include "mpd_client_browse.h"

sds mpd_client_put_fingerprint(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                               const char *uri)
{
    buffer = jsonrpc_start_result(buffer, method, request_id);
    
    #if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
    char fp_buffer[8192];
    const char *fingerprint = mpd_run_getfingerprint_chromaprint(mpd_state->conn, uri, fp_buffer, sizeof(fp_buffer));
    if (fingerprint == NULL) {
        buffer = check_error_and_recover(buffer, method, request_id);
        return buffer;
    }
    buffer = sdscat(buffer, "{");
    buffer = tojson_char(buffer, "fingerprint", fingerprint, false);
    mpd_response_finish(mpd_state->conn);
    buffer = sdscat(buffer, "}");
    #else
    (void)(mpd_state);
    (void)(uri);
    #endif
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

sds mpd_client_put_songdetails(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                               const char *uri)
{
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer,"{");    

    if (!mpd_send_list_all_meta(mpd_state->conn, uri)) {
        buffer = check_error_and_recover(buffer, method, request_id);
        return buffer;
    }
    struct mpd_entity *entity;
    if ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
        const struct mpd_song *song = mpd_entity_get_song(entity);
        buffer = put_song_tags(buffer, mpd_state, mpd_state->mympd_tag_types, song);
        mpd_entity_free(entity);
    }
    else {
        buffer = check_error_and_recover(buffer, method, request_id);
        return buffer;
    }
    mpd_response_finish(mpd_state->conn);
    
    sds cover = sdsempty();
    cover = mpd_client_get_cover(config, mpd_state, uri, cover);
    buffer = sdscat(buffer, ",");
    buffer = tojson_char(buffer, "cover", cover, false);
    sdsfree(cover);

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
    buffer = sdscat(buffer, "}"); 
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

sds mpd_client_put_filesystem(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                              const char *path, const unsigned int offset, const char *filter, const t_tags *tagcols)
{
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, "[");
    
    if (!mpd_send_list_meta(mpd_state->conn, path)) {
        buffer = check_error_and_recover(buffer, method, request_id);
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
                        buffer = tojosn_char(buffer, "name", dirName, false);
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
                        bool smartpls = false;
                        if (validate_string(plName) == true) {
                            sds smartpls_file = sdscatprintf(sdsempty(), "%s/smartpls/%s", config->varlibdir, plName);
                            if (access(smartpls_file, F_OK ) != -1) {
                                smartpls = true;
                            }
                            sds_free(smartpls_file);
                        }
                        buffer = sdscatprintf(buffer, "{\"Type\": \"%s\"", (smartpls == true ? "smartpls" : "plist"));
                        buffer = tojson_char(buffer, "uri", entityName, true);
                        buffer = tojson_char(buffer, "name", plName, false);
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

    buffer = sdscatprintf(buffer, "],\"totalEntities\":%d,\"offset\":%d,\"returnedEntities\":%d,", entity_count, offset, entities_returned);
    buffer = tojson_char(buffer, "filter", filter, false);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

sds mpd_client_put_db_tag(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                          const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter)
{
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, "[");
    
    if (mpd_search_db_tags(mpd_state->conn, mpd_tag_name_parse(mpdtagtype)) == false) {
        buffer = check_error_and_recover(buffer, method, request_id);
        return buffer;
    }
    if (mpd_tag_name_parse(mpdsearchtagtype) != MPD_TAG_UNKNOWN) {
        if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdsearchtagtype), searchstr) == false) {
            buffer = check_error_and_recover(buffer, method, request_id);
            return buffer;
        }
    }
    if (mpd_search_commit(mpd_state->conn) == false) {
        buffer = check_error_and_recover(buffer, method, request_id);
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

    buffer = sdscatprintf(buffer, "],\"totalEntities\":%d,\"offset\":%d,\"returnedEntities\":%d,", entity_count, offset, entities_returned);
    buffer = tojson_char(buffer, "filter", filter, true);
    buffer = tojson_char(buffer, "searchstr", searchstr, true);
    buffer = tojson_char(buffer, "searchtagtype", mpdsearchtagtype, true);
    buffer = tojson_char(buffer, "tagtype", mpdtagtype, false);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

sds mpd_client_put_songs_in_album(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                                  const char *album, const char *search, const char *tag, const t_tags *tagcols)
{
    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, "[");

    if (mpd_search_db_songs(mpd_state->conn, true) == false) {
        buffer = check_error_and_recover(buffer, method, request_id);
        return buffer;
    }    
    if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(tag), search) == false) {
        buffer = check_error_and_recover(buffer, method, request_id);
        return buffer;
    }
    if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album) == false) {
        buffer = check_error_and_recover(buffer, method, request_id);
        return buffer;
    }
    if (mpd_search_commit(mpd_state->conn) == false) {
        buffer = check_error_and_recover(buffer, method, request_id);
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

    sds cover = sdsempty();
    char *albumartist = NULL;
    if (first_song != NULL) {
        cover = mpd_client_get_cover(config, mpd_state, mpd_song_get_uri(first_song), cover);
        albumartist = mpd_client_get_tag(first_song, MPD_TAG_ALBUM_ARTIST);
    }
    else {
        cover = sdscat(cover, "/assets/coverimage-notavailable.svg");
    }

    buffer = sdscatprintf(buffer, "],\"totalEntities\":%d,\"returnedEntities\":%d,", entity_count, entities_returned);
    buffer = tojson_char(buffer, "Album", album, true);
    buffer = tojson_char(buffer, "search", search, true);
    buffer = tojson_char(buffer, "tag", tag, true);
    buffer = tojson_char(buffer, "cover", cover, true);
    buffer = tojson_char(buffer, "AlbumArtist", (albumartist != NULL ? albumartist : "-"), true);
    buffer = tojson_long(buffer, "totalTime", totalTime, false);
    buffer = jsonrpc_end_result(buffer);
        
    sdsfree(cover);
    if (first_song != NULL) {
        mpd_song_free(first_song);
    }
    return buffer;    
}
