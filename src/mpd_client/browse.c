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
#include "mpd_client_utils.h"
#include "browse.h"

int mpd_client_put_fingerprint(t_mpd_state *mpd_state, char *buffer, const char *uri) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out, "{type: fingerprint, data: {");
    #if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
    if (mpd_state->feat_fingerprint == true) {
        char fp_buffer[8192];
        const char *fingerprint = mpd_run_getfingerprint_chromaprint(mpd_state->conn, uri, fp_buffer, sizeof(fp_buffer));
        if (fingerprint == NULL) {
            RETURN_ERROR_AND_RECOVER("mpd_getfingerprint");
        }
        len += json_printf(&out, "fingerprint: %Q", fingerprint);
        mpd_response_finish(mpd_state->conn);
    }
    else {
        len += json_printf(&out, "fingerprint: %Q", "not supported by mpd");
    }
    #else
        len += json_printf(&out, "fingerprint: %Q", "libmpdclient to old");
        (void)(mpd_state);
        (void)(uri);
    #endif
    len += json_printf(&out, "}}");
    
    CHECK_RETURN_LEN();
}

int mpd_client_put_songdetails(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri) {
    struct mpd_entity *entity;
    const struct mpd_song *song;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out, "{type: song_details, data: {");

    if (!mpd_send_list_all_meta(mpd_state->conn, uri)) {
        RETURN_ERROR_AND_RECOVER("mpd_send_list_all_meta");
    }
    if ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
        song = mpd_entity_get_song(entity);
        PUT_SONG_TAG_ALL();
        mpd_entity_free(entity);
    }
    mpd_response_finish(mpd_state->conn);
    
    sds cover = sdsempty();
    cover = mpd_client_get_cover(config, mpd_state, uri, cover);
    len += json_printf(&out, ", cover: %Q", cover);

    if (mpd_state->feat_sticker) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        assert(sticker);
        mpd_client_get_sticker(mpd_state, uri, sticker);
        len += json_printf(&out, ", playCount: %d, skipCount: %d, like: %d, lastPlayed: %d, lastSkipped: %d",
            sticker->playCount,
            sticker->skipCount,
            sticker->like,
            sticker->lastPlayed,
            sticker->lastSkipped
        );
        FREE_PTR(sticker);
    }
    len += json_printf(&out, "}}");
    
    sdsfree(cover);
    
    CHECK_RETURN_LEN();
}

int mpd_client_put_filesystem(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *path, const unsigned int offset, const char *filter, const t_tags *tagcols) {
    struct mpd_entity *entity;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    const char *entityName;
    char smartpls_file[400];
    bool smartpls;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (!mpd_send_list_meta(mpd_state->conn, path)) {
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");
    }

    len = json_printf(&out, "{type: browse, data: [");

    while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            switch (mpd_entity_get_type(entity)) {
                case MPD_ENTITY_TYPE_UNKNOWN: {
                    entity_count--;
                    break;
                }
                case MPD_ENTITY_TYPE_SONG: {
                    const struct mpd_song *song = mpd_entity_get_song(entity);
                    entityName = mpd_client_get_tag(song, MPD_TAG_TITLE);
                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, entityName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*entityName) == 0 )) 
                    {
                        if (entities_returned++) 
                            len += json_printf(&out, ",");
                        len += json_printf(&out, "{Type: song, ");
                        PUT_SONG_TAG_COLS(tagcols);
                        len += json_printf(&out, "}");
                    }
                    else {
                        entity_count--;
                    }
                    break;
                }
                case MPD_ENTITY_TYPE_DIRECTORY: {
                    const struct mpd_directory *dir = mpd_entity_get_directory(entity);                
                    entityName = mpd_directory_get_path(dir);
                    char *dirName = strrchr(entityName, '/');

                    if (dirName != NULL) {
                        dirName++;
                    }
                    else {
                        dirName = (char *) entityName;
                    }

                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, dirName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*dirName) == 0 )) 
                    {                
                        if (entities_returned++) 
                            len += json_printf(&out, ",");
                        len += json_printf(&out, "{Type: dir, uri: %Q, name: %Q}",
                            entityName,
                            dirName
                        );
                    }
                    else {
                        entity_count--;
                    }
                    dirName = NULL;
                    break;
                }
                case MPD_ENTITY_TYPE_PLAYLIST: {
                    const struct mpd_playlist *pl = mpd_entity_get_playlist(entity);
                    entityName = mpd_playlist_get_path(pl);
                    char *plName = strrchr(entityName, '/');
                    if (plName != NULL) {
                        plName++;
                    } else {
                        plName = (char *) entityName;
                    }
                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, plName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*plName) == 0 )) 
                    {
                        if (entities_returned++) {
                            len += json_printf(&out, ",");
                        }
                        if (validate_string(plName) == true) {
                            snprintf(smartpls_file, 400, "%s/smartpls/%s", config->varlibdir, plName);
                            if (access(smartpls_file, F_OK ) != -1) {
                                smartpls = true;
                            }
                            else {
                                smartpls = false;
                            }
                        }
                        else {
                            smartpls = false;
                        }                        
                        len += json_printf(&out, "{Type: %Q, uri: %Q, name: %Q}",
                            (smartpls == true ? "smartpls" : "plist"),
                            entityName,
                            plName
                        );
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

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, filter: %Q}",
        entity_count,
        offset,
        entities_returned,
        filter
    );

    CHECK_RETURN_LEN();
}

int mpd_client_put_db_tag(t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter) {
    struct mpd_pair *pair;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_db_tags(mpd_state->conn, mpd_tag_name_parse(mpdtagtype)) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_db_tags");

    if (mpd_tag_name_parse(mpdsearchtagtype) != MPD_TAG_UNKNOWN) {
        if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdsearchtagtype), searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
    }

    if (mpd_search_commit(mpd_state->conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");

    len = json_printf(&out, "{type: listDBtags, data: [");
    while ((pair = mpd_recv_pair_tag(mpd_state->conn, mpd_tag_name_parse(mpdtagtype))) != NULL && len < MAX_LIST_SIZE) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + mpd_state->max_elements_per_page) {
            if (strcmp(pair->value, "") == 0) {
                entity_count--;
            }
            else if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, pair->value, 1) == 0 ||
                    (strncmp(filter, "0", 1) == 0 && isalpha(*pair->value) == 0 )) 
            {
                if (entities_returned++) 
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{type: %Q, value: %Q}",
                    mpdtagtype,
                    pair->value    
                );
            }
            else {
                entity_count--;
            }
        }
        mpd_return_pair(mpd_state->conn, pair);
    }
        
    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, "
        "tagtype: %Q, searchtagtype: %Q, searchstr: %Q, filter: %Q}",
        entity_count,
        offset,
        entities_returned,
        mpdtagtype,
        mpdsearchtagtype,
        searchstr,
        filter
    );

    CHECK_RETURN_LEN();
}

int mpd_client_put_songs_in_album(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *album, const char *search, const char *tag, const t_tags *tagcols) {
    struct mpd_song *song;
    struct mpd_song *first_song = NULL;
    int entity_count = 0;
    int entities_returned = 0;
    size_t len = 0;
    int totalTime = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_db_songs(mpd_state->conn, true) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_db_songs");
    
    if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(tag), search) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");

    if (mpd_search_add_tag_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
        
    if (mpd_search_commit(mpd_state->conn) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    }
    else {
        len = json_printf(&out, "{type: listTitles, data: [");

        while ((song = mpd_recv_song(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
            entity_count++;
            if (entities_returned++) {
                len += json_printf(&out, ", ");
            }
            else {
                first_song = mpd_song_dup(song);
            }
            len += json_printf(&out, "{Type: song, ");
            PUT_SONG_TAG_COLS(tagcols);
            len += json_printf(&out, "}");
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
        
        len += json_printf(&out, "], totalEntities: %d, returnedEntities: %d, Album: %Q, search: %Q, tag: %Q, cover: %Q, AlbumArtist: %Q, totalTime: %d}",
            entity_count,
            entities_returned,
            album,
            search,
            tag,
            cover,
            (albumartist != NULL ? albumartist : "-"),
            totalTime
        );
        sdsfree(cover);
        if (first_song != NULL) {
            mpd_song_free(first_song);
        }
    }
    
    CHECK_RETURN_LEN();
}
