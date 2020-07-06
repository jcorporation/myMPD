/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

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
#include "config_defs.h"
#include "../utility.h"
#include "../mpd_shared/mpd_shared_typedefs.h"
#include "../mpd_shared/mpd_shared_tags.h"
#include "../mpd_shared.h"
#include "mpd_client_utility.h"
#include "mpd_client_sticker.h"
#include "mpd_client_jukebox.h"

//private definitions
static struct list *mpd_client_jukebox_get_last_played(t_config *config, t_mpd_client_state *mpd_client_state);
static bool mpd_client_jukebox_fill_jukebox_queue(t_config *config, t_mpd_client_state *mpd_client_state, int addSongs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static bool _mpd_client_jukebox_fill_jukebox_queue(t_config *config, t_mpd_client_state *mpd_client_state, int addSongs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static bool mpd_client_jukebox_unique_tag(t_mpd_client_state *mpd_client_state, const char *uri, const char *value, bool manual, struct list *queue_list);
static bool mpd_client_jukebox_unique_album(t_mpd_client_state *mpd_client_state, const char *album, bool manual, struct list *queue_list);
static bool add_album_to_queue(t_mpd_client_state *mpd_client_state, const char *album);

//public functions
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
    
    if (queue_length >= mpd_client_state->jukebox_queue_length && now < add_time) {
        LOG_DEBUG("Jukebox: Queue length >= %d and add_time not reached", mpd_client_state->jukebox_queue_length);
        return true;
    }

    //add song if add_time is reached or queue is empty
    unsigned long addSongs = mpd_client_state->jukebox_queue_length - queue_length;
    if (now > add_time && add_time > 0) {
        addSongs++;
    }

    if (addSongs < 1) {
        return true;
    }
        
    if (mpd_client_state->feat_playlists == false && strcmp(mpd_client_state->jukebox_playlist, "Database") != 0) {
        LOG_WARN("Jukebox: Playlists are disabled");
        return true;
    }
    
    if (addSongs > INT_MAX) {
        addSongs = INT_MAX;
    }

    bool rc = mpd_client_jukebox_add_to_queue(config, mpd_client_state, (int)addSongs, mpd_client_state->jukebox_mode, mpd_client_state->jukebox_playlist, false);
    
    if (rc == true) {
        bool rc2 = mpd_run_play(mpd_client_state->mpd_state->conn);
        check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc2, "mpd_run_play");
    }
    else {
        LOG_DEBUG("Jukebox mode: %d", mpd_client_state->jukebox_mode);
        LOG_ERROR("Jukebox: Error adding song(s)");
        if (mpd_client_state->jukebox_mode != JUKEBOX_OFF && attempt == 0) {
            LOG_ERROR("Jukebox: trying again");
            //retry it only one time
            mpd_client_jukebox(config, mpd_client_state, 1);
        }
    }
    return rc;
}

bool mpd_client_jukebox_add_to_queue(t_config *config, t_mpd_client_state *mpd_client_state, int addSongs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual) {
    if (manual == false) {
        LOG_DEBUG("Jukebox queue length: %d", mpd_client_state->jukebox_queue.length);
    }
    if ((manual == false && addSongs > mpd_client_state->jukebox_queue.length) ||
        (manual == true)) 
    {
        bool rc = mpd_client_jukebox_fill_jukebox_queue(config, mpd_client_state, addSongs, jukebox_mode, playlist, manual);
        if (rc == false) {
            return false;
        }
    }
    int added = 0;
    struct list_node *current;
    if (manual == false) {
        current = mpd_client_state->jukebox_queue.head;
    }
    else {
        current = mpd_client_state->jukebox_queue_tmp.head;
    }
    while (current != NULL && added < addSongs) {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
	    bool rc = mpd_run_add(mpd_client_state->mpd_state->conn, current->key);
            if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_run_add") == true) {
	        LOG_INFO("Jukebox adding song: %s", current->key);
                added++;
            }
            else {
                LOG_ERROR("Jukebox adding song %s failed", current->key);
            }
        }
        else {
            bool rc = add_album_to_queue(mpd_client_state, current->key);
            if (rc == true) {
                LOG_INFO("Jukebox adding album: %s", current->key);
                added++;
            }
            else {
                LOG_ERROR("Jukebox adding album %s failed", current->key);
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
        LOG_ERROR("Error adding song(s)");
        return false;
    }
    if (manual == false) {
        if ((jukebox_mode == JUKEBOX_ADD_SONG && mpd_client_state->jukebox_queue.length < 25) ||
            (jukebox_mode == JUKEBOX_ADD_ALBUM && mpd_client_state->jukebox_queue.length < 5))
        {
            bool rc = mpd_client_jukebox_fill_jukebox_queue(config, mpd_client_state, addSongs, jukebox_mode, playlist, manual);
            if (rc == false) {
                return false;
            }
        }
        LOG_DEBUG("Jukebox queue length: %d", mpd_client_state->jukebox_queue.length);
    }
    return true;
}


//private functions
static bool add_album_to_queue(t_mpd_client_state *mpd_client_state, const char *album) {
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
    rc = mpd_search_commit(mpd_client_state->mpd_state->conn);
    return check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc, "mpd_search_commit");
}

static struct list *mpd_client_jukebox_get_last_played(t_config *config, t_mpd_client_state *mpd_client_state) {
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
        const char *tag_value = NULL;
        if (mpd_client_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
            tag_value = mpd_song_get_tag(song, mpd_client_state->jukebox_unique_tag.tags[0], 0);
        }
        list_push(queue_list, mpd_song_get_uri(song), 0, tag_value, NULL);
        mpd_song_free(song);
    }
    mpd_response_finish(mpd_client_state->mpd_state->conn);
    check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false);
    
    //put last_played to queue list
    struct list_node *current = mpd_client_state->last_played.head;
    while (current != NULL) {
        bool rc2 = mpd_send_list_meta(mpd_client_state->mpd_state->conn, current->key);
        if (check_rc_error_and_recover(mpd_client_state->mpd_state, NULL, NULL, 0, false, rc2, "mpd_send_list_meta") == true) {
            song = mpd_recv_song(mpd_client_state->mpd_state->conn);
            if (song != NULL) {
                list_push(queue_list, current->key, 0, mpd_song_get_tag(song, mpd_client_state->jukebox_unique_tag.tags[0], 0), NULL);
                mpd_song_free(song);
            }
        }
        mpd_response_finish(mpd_client_state->mpd_state->conn);
        check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false);
        current = current->next;
    }
    //get last_played from disc
    if (queue_list->length < 20 && config->readonly == false) {
        char *line = NULL;
        char *data = NULL;
        char *crap = NULL;
        size_t n = 0;
        sds lp_file = sdscatfmt(sdsempty(), "%s/state/last_played", config->varlibdir);
        FILE *fp = fopen(lp_file, "r");
        sdsfree(lp_file);
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
                            list_push(queue_list, data, 0, mpd_song_get_tag(song, mpd_client_state->jukebox_unique_tag.tags[0], 0), NULL);
                            mpd_song_free(song);
                        }
                    }
                    mpd_response_finish(mpd_client_state->mpd_state->conn);
                    check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false);
                }
                else {
                    LOG_ERROR("Reading last_played line failed");
                }
            }
            fclose(fp);
            FREE_PTR(line);
        }
    }
    LOG_DEBUG("Jukebox last_played list length: %d", queue_list->length);
    return queue_list;
}

static bool mpd_client_jukebox_fill_jukebox_queue(t_config *config, t_mpd_client_state *mpd_client_state, int addSongs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual) {
    LOG_DEBUG("Jukebox queue to small, adding entities");
    if (mpd_client_state->mpd_state->feat_tags == true) {
        if (mpd_client_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
            enable_mpd_tags(mpd_client_state->mpd_state, mpd_client_state->jukebox_unique_tag);
        }
        else {
            disable_all_mpd_tags(mpd_client_state->mpd_state);
        }
    }
    bool rc = _mpd_client_jukebox_fill_jukebox_queue(config, mpd_client_state, addSongs, jukebox_mode, playlist, manual);
    if (mpd_client_state->mpd_state->feat_tags == true) {
        enable_mpd_tags(mpd_client_state->mpd_state, mpd_client_state->mpd_state->mympd_tag_types);
    }
    
    if (rc == false) {
        LOG_ERROR("Filling jukebox queue failed, disabling jukebox");
        send_jsonrpc_notify_error("Filling jukebox queue failed, disabling jukebox");
        mpd_client_state->jukebox_mode = JUKEBOX_OFF;
        return false;
    }
    return true;
}

static bool _mpd_client_jukebox_fill_jukebox_queue(t_config *config, t_mpd_client_state *mpd_client_state, int addSongs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual) {
    struct mpd_pair *pair;
    int lineno = 1;
    int skipno = 0;
    int nkeep = 0;
    
    if (manual == true) {
        list_free(&mpd_client_state->jukebox_queue_tmp);
    }
    
    if (jukebox_mode == JUKEBOX_ADD_SONG && strcmp(playlist, "Database") == 0 && mpd_client_state->mpd_state->feat_mpd_searchwindow == false) {
        LOG_ERROR("Jukebox mode song and playlist database depends on mpd version >= 0.20.0");
        return false;
    }
    
    //get last_played and current queue
    struct list *queue_list = mpd_client_jukebox_get_last_played(config, mpd_client_state);
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
            LOG_WARN("Sticker cache is null, jukebox doesn't respect last played constraint");
        }
        
        int start_length;
        if (manual == false) {
            start_length = mpd_client_state->jukebox_queue.length;
            addSongs = 50 - start_length;
        }
        else {
            start_length = 0;
        }
        do {
            LOG_DEBUG("Jukebox: iterating through source, start: %u", start);

            if (strcmp(playlist, "Database") == 0) {
                if (mpd_search_db_songs(mpd_client_state->mpd_state->conn, false) == false) { 
                    LOG_ERROR("Error in response to command: mpd_search_db_songs");
                }
                else if (mpd_search_add_uri_constraint(mpd_client_state->mpd_state->conn, MPD_OPERATOR_DEFAULT, "") == false) { 
                    LOG_ERROR("Error in response to command: mpd_search_add_uri");
                    mpd_search_cancel(mpd_client_state->mpd_state->conn);
                }
                else if (mpd_search_add_window(mpd_client_state->mpd_state->conn, start, end) == false) { 
                    LOG_ERROR("Error in response to command: mpd_search_add_window");
                    mpd_search_cancel(mpd_client_state->mpd_state->conn);
                }
                else if (mpd_search_commit(mpd_client_state->mpd_state->conn) == false) {
                    LOG_ERROR("Error in response to command: mpd_search_commit");
                    mpd_search_cancel(mpd_client_state->mpd_state->conn);
                }
            }
            else {
                if (mpd_send_list_playlist_meta(mpd_client_state->mpd_state->conn, playlist) == false) {
                    LOG_ERROR("Error in response to command: mpd_send_list_playlist_meta");
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
                    t_sticker *sticker = get_sticker_from_cache(mpd_client_state, uri);
                    if (sticker != NULL) {
                        last_played = sticker->lastPlayed;
                    }
                }
                    
                if (mpd_client_state->jukebox_enforce_unique == false || (
                    (last_played == 0 || last_played < now) && 
                    mpd_client_jukebox_unique_tag(mpd_client_state, uri, tag_value, manual, queue_list) == true)) 
                {
                    if (randrange(0, lineno) < addSongs) {
                        if (nkeep < addSongs) {
                            if (manual == false) {
		                if (list_push(&mpd_client_state->jukebox_queue, uri, lineno, tag_value, NULL) == false) {
		                    LOG_ERROR("Can't push jukebox_queue element");
		                }
                            }
                            else {
                                if (list_push(&mpd_client_state->jukebox_queue_tmp, uri, lineno, tag_value, NULL) == false) {
                                    LOG_ERROR("Can't push jukebox_queue_tmp element");
                                }
                            }
                            nkeep++;
                        }
                        else {
                            int i = addSongs > 1 ? start_length + randrange(0, addSongs) - 1 : 0;
                            if (manual == false) {
                                if (list_replace(&mpd_client_state->jukebox_queue, i, uri, lineno, tag_value, NULL) == false) {
                                    LOG_ERROR("Can't replace jukebox_queue element pos %d", i);
                                }
                            }
                            else {
                                if (list_replace(&mpd_client_state->jukebox_queue_tmp, i, uri, lineno, tag_value, NULL) == false) {
                                    LOG_ERROR("Can't replace jukebox_queue_tmp element pos %d", i);
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
        LOG_DEBUG("Jukebox iterated through %u songs, skipped %u", lineno, skipno);
    }
    else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
        //add album
        if (manual == false) {
            addSongs = 10 - mpd_client_state->jukebox_queue.length;
        }
        
        if (mpd_search_db_tags(mpd_client_state->mpd_state->conn, MPD_TAG_ALBUM) == false) {
            LOG_ERROR("Error in response to command: mpd_search_db_tags");
            mpd_search_cancel(mpd_client_state->mpd_state->conn);
        }
        else if (mpd_search_commit(mpd_client_state->mpd_state->conn) == false) { 
            LOG_ERROR("Error in response to command: mpd_search_commit");
        }
        
        if (check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false) == false) {
            list_free(queue_list);
            FREE_PTR(queue_list);
            return false;
        }
        while ((pair = mpd_recv_pair_tag(mpd_client_state->mpd_state->conn, MPD_TAG_ALBUM )) != NULL)  {
            if (mpd_client_state->jukebox_enforce_unique == false || mpd_client_jukebox_unique_album(mpd_client_state, pair->value, manual, queue_list) == true) {
                if (randrange(0, lineno) < addSongs) {
                    if (nkeep < addSongs) {
                        if (manual == false) {
                            if (list_push(&mpd_client_state->jukebox_queue, pair->value, lineno, NULL, NULL) == false) {
                                LOG_ERROR("Can't push jukebox_queue_tmp element");
                            }
                        }
                        else {
                            if (list_push(&mpd_client_state->jukebox_queue_tmp, pair->value, lineno, NULL, NULL) == false) {
                                LOG_ERROR("Can't push jukebox_queue_tmp element");
                            }
                        }
                        nkeep++;
                    }
                    else {
                        int i = addSongs > 1 ? randrange(0, addSongs) : 0;
                        if (manual == false) {
                            if (list_replace(&mpd_client_state->jukebox_queue, i, pair->value, lineno, NULL, NULL) == false) {
                                LOG_ERROR("Can't replace jukebox_queue element pos %d", i);
                            }
                        }
                        else {
                            if (list_replace(&mpd_client_state->jukebox_queue_tmp, i, pair->value, lineno, NULL, NULL) == false) {
                                LOG_ERROR("Can't replace jukebox_queue_tmp element pos %d", i);
                            }
                        }
                    }
                }
                lineno++;
            }
            mpd_return_pair(mpd_client_state->mpd_state->conn, pair);
        }
        mpd_response_finish(mpd_client_state->mpd_state->conn);
        if (check_error_and_recover2(mpd_client_state->mpd_state, NULL, NULL, 0, false) == false) {
            list_free(queue_list);
            FREE_PTR(queue_list);
            return false;
        }
        LOG_DEBUG("Jukebox iterated through %u albums, skipped %u", lineno, skipno);
    }

    if (nkeep < addSongs) {
        LOG_WARN("Jukebox queue didn't contain %d entries", addSongs);
        if (mpd_client_state->jukebox_enforce_unique == true) {
            LOG_WARN("Disabling jukebox unique constraints temporarily");
            mpd_client_state->jukebox_enforce_unique = false;
            send_jsonrpc_notify_warn("Playlist to small, disabling jukebox unique constraints temporarily");
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

static bool mpd_client_jukebox_unique_album(t_mpd_client_state *mpd_client_state, const char *album, bool manual, struct list *queue_list) {
    struct list_node *current = queue_list->head;
    while (current != NULL) {
        if (strcmp(current->value_p, album) == 0) {
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
        if (strcmp(current->key, album) == 0) {
            return false;
        }
        current = current->next;
    }
    return true;
}
