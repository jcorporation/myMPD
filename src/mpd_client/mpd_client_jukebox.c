/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <inttypes.h>
#include <limits.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "../random.h"
#include "mympd_config_defs.h"
#include "../utility.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared/mpd_shared_sticker.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"
#include "mpd_client_sticker.h"
#include "mpd_client_jukebox.h"

//private definitions
static struct list *mpd_client_jukebox_get_last_played(t_config *config, t_mpd_client_state *mpd_client_state, enum jukebox_modes jukebox_mode);
static bool mpd_client_jukebox_fill_jukebox_queue(t_config *config, t_mpd_client_state *mpd_client_state, 
    unsigned add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static bool _mpd_client_jukebox_fill_jukebox_queue(t_config *config, t_mpd_client_state *mpd_client_state,
    unsigned add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static bool mpd_client_jukebox_unique_tag(t_mpd_client_state *mpd_client_state, const char *uri, const char *value, bool manual, struct list *queue_list);
static bool mpd_client_jukebox_unique_album(t_mpd_client_state *mpd_client_state, const char *album, const char *albumartist, bool manual, struct list *queue_list);
static bool add_album_to_queue(t_mpd_client_state *mpd_client_state, const char *album, const char *albumartist);

//public functions
bool mpd_client_rm_jukebox_entry(t_mpd_client_state *mpd_client_state, unsigned pos) {
    return list_shift(&mpd_client_state->jukebox_queue, pos);
}

sds mpd_client_put_jukebox_list(t_mpd_client_state *mpd_client_state, sds buffer, sds method, long request_id, 
                                const unsigned int offset, const unsigned int limit, const t_tags *tagcols)
{
    unsigned entity_count = 0;
    unsigned entities_returned = 0;

    buffer = jsonrpc_result_start(buffer, method, request_id);
    buffer = sdscat(buffer, "\"data\":[");

    if (mpd_client_state->jukebox_queue.length > 0) {
        struct list_node *current = mpd_client_state->jukebox_queue.head;
        while (current != NULL) {
            entity_count++;
            if (entity_count > offset && (entity_count <= offset + limit || limit == 0)) {
                if (entities_returned++) {
                    buffer = sdscat(buffer, ",");
                }
                buffer = sdscat(buffer, "{");
                buffer = tojson_long(buffer, "Pos", entity_count, true);
                if (mpd_client_state->jukebox_mode == JUKEBOX_ADD_SONG) {
                    bool rc = mpd_send_list_meta(mpd_client_state->mpd_state->conn, current->key);
                    if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_meta") == true) {
                        struct mpd_entity *entity;
                        if ((entity = mpd_recv_entity(mpd_client_state->mpd_state->conn)) != NULL) {
                            const struct mpd_song *song = mpd_entity_get_song(entity);
                            buffer = put_song_tags(buffer, mpd_client_state->mpd_state, tagcols, song);
                            if (mpd_client_state->mpd_state->feat_stickers == true && mpd_client_state->sticker_cache != NULL) {
                                buffer = sdscat(buffer, ",");
                                buffer = mpd_shared_sticker_list(buffer, mpd_client_state->sticker_cache, mpd_song_get_uri(song));
                            }
                            mpd_entity_free(entity);
                            mpd_response_finish(mpd_client_state->mpd_state->conn);
                        }
                        else {
                            buffer = put_empty_song_tags(buffer, mpd_client_state->mpd_state, tagcols, current->key);
                        }
                    }
                    else {
                        buffer = put_empty_song_tags(buffer, mpd_client_state->mpd_state, tagcols, current->key);
                    }
                    mpd_response_finish(mpd_client_state->mpd_state->conn);
                    check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false);
                }
                else if (mpd_client_state->jukebox_mode == JUKEBOX_ADD_ALBUM) {
                    buffer = tojson_char(buffer, "uri", "Album", true);
                    buffer = tojson_char(buffer, "Title", "", true);
                    buffer = tojson_char(buffer, "Album", current->key, true);
                    buffer = tojson_char(buffer, "AlbumArtist", current->value_p, true);
                    buffer = tojson_char(buffer, "Artist", current->value_p, false);
                }
                buffer = sdscat(buffer, "}");
            }
            current = current->next;
        }
    }
    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_result_end(buffer);

    return buffer;
}

bool mpd_client_jukebox(t_config *config, t_mpd_client_state *mpd_client_state, unsigned attempt) {
    struct mpd_status *status = mpd_run_status(mpd_client_state->mpd_state->conn);
    if (status == NULL) {
        check_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0);
        return false;
    }
    size_t queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);
    
    time_t now = time(NULL);
    time_t add_time = mpd_client_state->crossfade < mpd_client_state->song_end_time ? mpd_client_state->song_end_time - mpd_client_state->crossfade : 0;
    
    MYMPD_LOG_DEBUG("Queue length: %u", queue_length);
    MYMPD_LOG_DEBUG("Min queue length: %u", mpd_client_state->jukebox_queue_length);
    
    if (queue_length >= mpd_client_state->jukebox_queue_length && now < add_time) {
        MYMPD_LOG_DEBUG("Jukebox: Queue length >= %d and add_time not reached", mpd_client_state->jukebox_queue_length);
        return true;
    }

    //add song if add_time is reached or queue is empty
    unsigned add_songs = substractUnsigned(mpd_client_state->jukebox_queue_length, queue_length);
    
    if (now > add_time && add_time > 0 && queue_length <= mpd_client_state->jukebox_queue_length) {
        MYMPD_LOG_DEBUG("Time now %d greater than add_time %d, adding song", now, add_time);
        add_songs++;
    }

    if (add_songs < 1) {
        MYMPD_LOG_DEBUG("Jukebox: nothing to do");
        return true;
    }
    
    if (add_songs > 99) {
        MYMPD_LOG_WARN("Jukebox: max songs to add set to %ul, adding max. 99 songs", add_songs);
        add_songs = 99;
    }
        
    if (mpd_client_state->mpd_state->feat_playlists == false && strcmp(mpd_client_state->jukebox_playlist, "Database") != 0) {
        MYMPD_LOG_WARN("Jukebox: Playlists are disabled");
        return true;
    }

    bool rc = mpd_client_jukebox_add_to_queue(config, mpd_client_state, add_songs, mpd_client_state->jukebox_mode, mpd_client_state->jukebox_playlist, false);
    
    if (rc == true) {
        bool rc2 = mpd_run_play(mpd_client_state->mpd_state->conn);
        check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc2, "mpd_run_play");
    }
    else {
        MYMPD_LOG_DEBUG("Jukebox mode: %d", mpd_client_state->jukebox_mode);
        MYMPD_LOG_ERROR("Jukebox: Error adding song(s)");
        if (mpd_client_state->jukebox_mode != JUKEBOX_OFF && attempt == 0) {
            MYMPD_LOG_ERROR("Jukebox: trying again");
            //retry it only one time
            mpd_client_jukebox(config, mpd_client_state, 1);
        }
    }
    //notify clients
    send_jsonrpc_event("update_jukebox");
    return rc;
}

bool mpd_client_jukebox_add_to_queue(t_config *config, t_mpd_client_state *mpd_client_state, unsigned add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual) {
    if (manual == false) {
        MYMPD_LOG_DEBUG("Jukebox queue length: %d", mpd_client_state->jukebox_queue.length);
    }
    if ((manual == false && add_songs > mpd_client_state->jukebox_queue.length) ||
        (manual == true)) 
    {
        bool rc = mpd_client_jukebox_fill_jukebox_queue(config, mpd_client_state, add_songs, jukebox_mode, playlist, manual);
        if (rc == false) {
            return false;
        }
    }
    unsigned added = 0;
    struct list_node *current;
    if (manual == false) {
        current = mpd_client_state->jukebox_queue.head;
    }
    else {
        current = mpd_client_state->jukebox_queue_tmp.head;
    }
    while (current != NULL && added < add_songs) {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
	    bool rc = mpd_run_add(mpd_client_state->mpd_state->conn, current->key);
            if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_add") == true) {
	        MYMPD_LOG_NOTICE("Jukebox adding song: %s", current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR("Jukebox adding song %s failed", current->key);
            }
        }
        else {
            bool rc = add_album_to_queue(mpd_client_state, current->key, current->value_p);
            if (rc == true) {
                MYMPD_LOG_NOTICE("Jukebox adding album: %s - %s", current->value_p, current->key);
                added++;
            }
            else {
                MYMPD_LOG_ERROR("Jukebox adding album %s - %s failed", current->value_p, current->key);
            }
        }
        if (manual == false) {
            list_shift(&mpd_client_state->jukebox_queue, 0);
            current = mpd_client_state->jukebox_queue.head;
        }
        else {
            list_shift(&mpd_client_state->jukebox_queue_tmp, 0);
            current = mpd_client_state->jukebox_queue_tmp.head;
        }
    }
    if (added > 0) {
        bool rc = mpd_run_play(mpd_client_state->mpd_state->conn);
        check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_play");
    }
    else {
        MYMPD_LOG_ERROR("Error adding song(s)");
        return false;
    }
    if (manual == false) {
        if ((jukebox_mode == JUKEBOX_ADD_SONG && mpd_client_state->jukebox_queue.length < 25) ||
            (jukebox_mode == JUKEBOX_ADD_ALBUM && mpd_client_state->jukebox_queue.length < 5))
        {
            bool rc = mpd_client_jukebox_fill_jukebox_queue(config, mpd_client_state, add_songs, jukebox_mode, playlist, manual);
            if (rc == false) {
                return false;
            }
        }
        MYMPD_LOG_DEBUG("Jukebox queue length: %d", mpd_client_state->jukebox_queue.length);
    }
    return true;
}

//private functions
static bool add_album_to_queue(t_mpd_client_state *mpd_client_state, const char *album, const char *albumartist) {
    bool rc = mpd_search_add_db_songs(mpd_client_state->mpd_state->conn, true);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_add_db_songs") == false) {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        return false;
    }
    rc = mpd_search_add_tag_constraint(mpd_client_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_add_tag_constraint") == false) {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        return false;
    }
    //falback to artist tag if albumartist is not enabled
    if (mpd_shared_tag_exists(mpd_client_state->mpd_state->mympd_tag_types.tags, mpd_client_state->mpd_state->mympd_tag_types.len, MPD_TAG_ALBUM_ARTIST) == true) {
        rc = mpd_search_add_tag_constraint(mpd_client_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM_ARTIST, albumartist);
    }
    else {
        rc = mpd_search_add_tag_constraint(mpd_client_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ARTIST, albumartist);
    }
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_add_tag_constraint") == false) {
        mpd_search_cancel(mpd_client_state->mpd_state->conn);
        return false;
    }
    rc = mpd_search_commit(mpd_client_state->mpd_state->conn);
    mpd_response_finish(mpd_client_state->mpd_state->conn);
    return check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_commit");
}

static struct list *mpd_client_jukebox_get_last_played(t_config *config, t_mpd_client_state *mpd_client_state, enum jukebox_modes jukebox_mode) {
    struct mpd_song *song;
    struct list *queue_list = (struct list *) malloc(sizeof(struct list));
    assert(queue_list);
    list_init(queue_list);
        
    bool rc = mpd_send_list_queue_meta(mpd_client_state->mpd_state->conn);
    if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_send_list_queue_meta") == false) {
        list_free(queue_list);
        FREE_PTR(queue_list);
        return NULL;
    }
    while ((song = mpd_recv_song(mpd_client_state->mpd_state->conn)) != NULL) {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
            const char *tag_value = NULL;
            if (mpd_client_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
                tag_value = mpd_song_get_tag(song, mpd_client_state->jukebox_unique_tag.tags[0], 0);
            }
            list_push(queue_list, mpd_song_get_uri(song), 0, tag_value, NULL);
        }
        else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
            sds album = mpd_shared_get_tags(song, MPD_TAG_ALBUM, sdsempty());
            sds albumartist = mpd_shared_get_tags(song, MPD_TAG_ALBUM_ARTIST, sdsempty());
            list_push(queue_list, album, 0, albumartist, NULL);
            sdsfree(album);
            sdsfree(albumartist);
        }
        mpd_song_free(song);
    }
    mpd_response_finish(mpd_client_state->mpd_state->conn);
    check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false);
    
    //append last_played to queue list
    struct list_node *current = mpd_client_state->last_played.head;
    while (current != NULL) {
        bool rc2 = mpd_send_list_meta(mpd_client_state->mpd_state->conn, current->key);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc2, "mpd_send_list_meta") == true) {
            song = mpd_recv_song(mpd_client_state->mpd_state->conn);
            if (song != NULL) {
                if (jukebox_mode == JUKEBOX_ADD_SONG) {
                    list_push(queue_list, current->key, 0, mpd_song_get_tag(song, mpd_client_state->jukebox_unique_tag.tags[0], 0), NULL);
                }
                else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
                    sds album = mpd_shared_get_tags(song, MPD_TAG_ALBUM, sdsempty());
                    sds albumartist = mpd_shared_get_tags(song, MPD_TAG_ALBUM_ARTIST, sdsempty());
                    list_push(queue_list, album, 0, albumartist, NULL);
                    sdsfree(album);
                    sdsfree(albumartist);
                }
                mpd_song_free(song);
            }
        }
        mpd_response_finish(mpd_client_state->mpd_state->conn);
        check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false);
        current = current->next;
    }
    //get last_played from disc
    if (queue_list->length < 20) {
        char *line = NULL;
        char *data = NULL;
        char *crap = NULL;
        size_t n = 0;
        sds lp_file = sdscatfmt(sdsempty(), "%s/state/last_played", config->varlibdir);
        FILE *fp = fopen(lp_file, "r");
        if (fp != NULL) {
            while (getline(&line, &n, fp) > 0 && queue_list->length < 20) {
                int value = strtoimax(line, &data, 10);
                if (value > 0 && strlen(data) > 2) {
                    data = data + 2;
                    strtok_r(data, "\n", &crap);
                    bool rc2 = mpd_send_list_meta(mpd_client_state->mpd_state->conn, data);
                    if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc2, "mpd_send_list_meta") == true) {
                        song = mpd_recv_song(mpd_client_state->mpd_state->conn);
                        if (song != NULL) {
                            if (jukebox_mode == JUKEBOX_ADD_SONG) {
                                list_push(queue_list, data, 0, mpd_song_get_tag(song, mpd_client_state->jukebox_unique_tag.tags[0], 0), NULL);
                            }
                            else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
                                sds album = mpd_shared_get_tags(song, MPD_TAG_ALBUM, sdsempty());
                                sds albumartist = mpd_shared_get_tags(song, MPD_TAG_ALBUM_ARTIST, sdsempty());
                                list_push(queue_list, album, 0, albumartist, NULL);
                                sdsfree(album);
                                sdsfree(albumartist);
                            }
                            mpd_song_free(song);
                        }
                    }
                    mpd_response_finish(mpd_client_state->mpd_state->conn);
                    check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false);
                }
                else {
                    MYMPD_LOG_ERROR("Reading last_played line failed");
                    MYMPD_LOG_DEBUG("Erroneous line: %s", line);
                }
            }
            fclose(fp);
            FREE_PTR(line);
        }
        else {
            //ignore missing last_played file
            MYMPD_LOG_DEBUG("Can not open \"%s\": %s", lp_file, strerror(errno));
        }
        sdsfree(lp_file);
    }
    MYMPD_LOG_DEBUG("Jukebox last_played list length: %d", queue_list->length);
    return queue_list;
}

static bool mpd_client_jukebox_fill_jukebox_queue(t_config *config, t_mpd_client_state *mpd_client_state, 
    unsigned add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual)
{
    MYMPD_LOG_DEBUG("Jukebox queue to small, adding entities");
    if (mpd_client_state->mpd_state->feat_tags == true) {
        if (mpd_client_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
            enable_mpd_tags(mpd_client_state->mpd_state, mpd_client_state->jukebox_unique_tag);
        }
        else {
            disable_all_mpd_tags(mpd_client_state->mpd_state);
        }
    }
    bool rc = _mpd_client_jukebox_fill_jukebox_queue(config, mpd_client_state, add_songs, jukebox_mode, playlist, manual);
    if (mpd_client_state->mpd_state->feat_tags == true) {
        enable_mpd_tags(mpd_client_state->mpd_state, mpd_client_state->mpd_state->mympd_tag_types);
    }
    
    if (rc == false) {
        MYMPD_LOG_ERROR("Filling jukebox queue failed, disabling jukebox");
        send_jsonrpc_notify("jukebox", "error", "Filling jukebox queue failed, disabling jukebox");
        mpd_client_state->jukebox_mode = JUKEBOX_OFF;
        return false;
    }
    return true;
}

static bool _mpd_client_jukebox_fill_jukebox_queue(t_config *config, t_mpd_client_state *mpd_client_state,
    unsigned add_songs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual)
{
    int lineno = 1;
    int skipno = 0;
    unsigned nkeep = 0;
    
    if (manual == true) {
        list_free(&mpd_client_state->jukebox_queue_tmp);
    }
    
    //get last_played and current queue
    struct list *queue_list = mpd_client_jukebox_get_last_played(config, mpd_client_state, jukebox_mode);
    if (queue_list == NULL) {
        return false;
    }
    
    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        //add songs
        int start = 0;
        int end = start + 1000;
        time_t now = time(NULL);
        now = now - mpd_client_state->jukebox_last_played * 60 * 60;
        
        if (mpd_client_state->sticker_cache == NULL) {
            MYMPD_LOG_WARN("Sticker cache is null, jukebox doesn't respect last played constraint");
        }
        
        unsigned start_length;
        if (manual == false) {
            start_length = mpd_client_state->jukebox_queue.length;
            add_songs = substractUnsigned(50, start_length);
        }
        else {
            start_length = 0;
        }
        do {
            MYMPD_LOG_DEBUG("Jukebox: iterating through source, start: %u", start);

            if (strcmp(playlist, "Database") == 0) {
                if (mpd_search_db_songs(mpd_client_state->mpd_state->conn, false) == false) { 
                    MYMPD_LOG_ERROR("Error in response to command: mpd_search_db_songs");
                }
                else if (mpd_search_add_uri_constraint(mpd_client_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, "") == false) { 
                    MYMPD_LOG_ERROR("Error in response to command: mpd_search_add_uri");
                    mpd_search_cancel(mpd_client_state->mpd_state->conn);
                }
                else if (mpd_search_add_window(mpd_client_state->mpd_state->conn, start, end) == false) { 
                    MYMPD_LOG_ERROR("Error in response to command: mpd_search_add_window");
                    mpd_search_cancel(mpd_client_state->mpd_state->conn);
                }
                else if (mpd_search_commit(mpd_client_state->mpd_state->conn) == false) {
                    MYMPD_LOG_ERROR("Error in response to command: mpd_search_commit");
                    mpd_search_cancel(mpd_client_state->mpd_state->conn);
                }
            }
            else {
                if (mpd_send_list_playlist_meta(mpd_client_state->mpd_state->conn, playlist) == false) {
                    MYMPD_LOG_ERROR("Error in response to command: mpd_send_list_playlist_meta");
                }
            }
            
            if (check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false) == false) {
                list_free(queue_list);
                FREE_PTR(queue_list);
                return false;
            }
            struct mpd_song *song;
            while ((song = mpd_recv_song(mpd_client_state->mpd_state->conn)) != NULL) {
                const char *tag_value = mpd_song_get_tag(song, mpd_client_state->jukebox_unique_tag.tags[0], 0);
                const char *uri = mpd_song_get_uri(song);
                time_t last_played = 0;
                if (mpd_client_state->sticker_cache != NULL) {
                    t_sticker *sticker = get_sticker_from_cache(mpd_client_state->sticker_cache, uri);
                    if (sticker != NULL) {
                        last_played = sticker->lastPlayed;
                    }
                }
                    
                if (mpd_client_state->jukebox_enforce_unique == false || (
                    (last_played == 0 || last_played < now) && 
                    mpd_client_jukebox_unique_tag(mpd_client_state, uri, tag_value, manual, queue_list) == true)) 
                {
                    if (randrange(0, lineno) < add_songs) {
                        if (nkeep < add_songs) {
                            if (manual == false) {
                                if (list_push(&mpd_client_state->jukebox_queue, uri, lineno, tag_value, NULL) == false) {
                                    MYMPD_LOG_ERROR("Can't push jukebox_queue element");
                                }
                            }
                            else {
                                if (list_push(&mpd_client_state->jukebox_queue_tmp, uri, lineno, tag_value, NULL) == false) {
                                    MYMPD_LOG_ERROR("Can't push jukebox_queue_tmp element");
                                }
                            }
                            nkeep++;
                        }
                        else {
                            unsigned i = add_songs > 1 ? start_length + randrange(0, add_songs -1)  : 0;
                            if (manual == false) {
                                if (list_replace(&mpd_client_state->jukebox_queue, i, uri, lineno, tag_value, NULL) == false) {
                                    MYMPD_LOG_ERROR("Can't replace jukebox_queue element pos %u", i);
                                }
                            }
                            else {
                                if (list_replace(&mpd_client_state->jukebox_queue_tmp, i, uri, lineno, tag_value, NULL) == false) {
                                    MYMPD_LOG_ERROR("Can't replace jukebox_queue_tmp element pos %u", i);
                                }
                            }
                        }
                    }
                    lineno++;
                }
                else {
                    skipno++;
                }
                mpd_song_free(song);
            }
            mpd_response_finish(mpd_client_state->mpd_state->conn);
            if (check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false) == false) {
                list_free(queue_list);
                FREE_PTR(queue_list);
                return false;
            }
            start = end;
            end = end + 1000;
        } while (strcmp(playlist, "Database") == 0 && lineno + skipno > start);
        MYMPD_LOG_DEBUG("Jukebox iterated through %u songs, skipped %u", lineno, skipno);
    }
    else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
        //add album
        if (manual == false) {
            add_songs = substractUnsigned(10, mpd_client_state->jukebox_queue.length);
        }
        
        raxIterator iter;
        raxStart(&iter, mpd_client_state->album_cache);
        raxSeek(&iter, "^", NULL, 0);
        struct mpd_song *song;
        while (raxNext(&iter)) { 
            song = (struct mpd_song *)iter.data;
            sds album = mpd_shared_get_tags(song, MPD_TAG_ALBUM, sdsempty());
            sds albumartist = mpd_shared_get_tags(song, MPD_TAG_ALBUM_ARTIST, sdsempty());
            if (mpd_client_state->jukebox_enforce_unique == false || 
                mpd_client_jukebox_unique_album(mpd_client_state, album, albumartist, manual, queue_list) == true)
            {
                if (randrange(0, lineno) < add_songs) {
                    if (nkeep < add_songs) {
                        if (manual == false) {
                            if (list_push(&mpd_client_state->jukebox_queue, album, lineno, albumartist, NULL) == false) {
                                MYMPD_LOG_ERROR("Can't push jukebox_queue_tmp element");
                            }
                        }
                        else {
                            if (list_push(&mpd_client_state->jukebox_queue_tmp, album, lineno, albumartist, NULL) == false) {
                                MYMPD_LOG_ERROR("Can't push jukebox_queue_tmp element");
                            }
                        }
                        nkeep++;
                    }
                    else {
                        unsigned i = add_songs > 1 ? randrange(0, add_songs - 1) : 0;
                        if (manual == false) {
                            if (list_replace(&mpd_client_state->jukebox_queue, i, album, lineno, albumartist, NULL) == false) {
                                MYMPD_LOG_ERROR("Can't replace jukebox_queue element pos %d", i);
                            }
                        }
                        else {
                            if (list_replace(&mpd_client_state->jukebox_queue_tmp, i, album, lineno, albumartist, NULL) == false) {
                                MYMPD_LOG_ERROR("Can't replace jukebox_queue_tmp element pos %d", i);
                            }
                        }
                    }
                }
                lineno++;
            }
            sdsfree(album);
            sdsfree(albumartist);
        }
        raxStop(&iter);
        MYMPD_LOG_DEBUG("Jukebox iterated through %u albums, skipped %u", lineno, skipno);
    }

    if (nkeep < add_songs) {
        MYMPD_LOG_WARN("Jukebox queue didn't contain %u entries", add_songs);
        if (mpd_client_state->jukebox_enforce_unique == true) {
            MYMPD_LOG_WARN("Disabling jukebox unique constraints temporarily");
            mpd_client_state->jukebox_enforce_unique = false;
            send_jsonrpc_notify("jukebox", "warn", "Playlist to small, disabling jukebox unique constraints temporarily");
        }
    }

    list_free(queue_list);
    FREE_PTR(queue_list);
    return true;
}

static bool mpd_client_jukebox_unique_tag(t_mpd_client_state *mpd_client_state, const char *uri, const char *value, bool manual, struct list *queue_list) {
    struct list_node *current = queue_list->head;
    while(current != NULL) {
        if (strcmp(current->key, uri) == 0) {
            return false;
        }
        if (value != NULL && strcmp(current->value_p, value) == 0) {
            return false;
        }
        current = current->next;
    }
    
    if (manual == false) {
        current = mpd_client_state->jukebox_queue.head;
    }
    else {
        current = mpd_client_state->jukebox_queue_tmp.head;
    }
    while (current != NULL) {
        if (strcmp(current->key, uri) == 0) {
            return false;
        }
        if (value != NULL && strcmp(current->value_p, value) == 0) {
            return false;
        }
        current = current->next;
    }
    return true;
}

static bool mpd_client_jukebox_unique_album(t_mpd_client_state *mpd_client_state, const char *album, const char *albumartist, bool manual, struct list *queue_list) {
    struct list_node *current = queue_list->head;
    while (current != NULL) {
        if (strcmp(current->key, album) == 0 && strcmp(current->value_p, albumartist) == 0) {
            return false;
        }
        current = current->next;
    }
    
    if (manual == false) {
        current = mpd_client_state->jukebox_queue.head;
    }
    else {
        current = mpd_client_state->jukebox_queue_tmp.head;
    }
    while (current != NULL) {
        if (strcmp(current->key, album) == 0 && strcmp(current->value_p, albumartist) == 0) {
            return false;
        }
        current = current->next;
    }
    return true;
}
