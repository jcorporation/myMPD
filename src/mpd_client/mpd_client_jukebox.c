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
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mpd_client_utility.h"
#include "mpd_client_jukebox.h"

//private definitions
static bool mpd_client_jukebox_fill_jukebox_queue(t_mpd_state *mpd_state, int addSongs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual);
static bool mpd_client_jukebox_unique_tag(t_mpd_state *mpd_state, const char *uri, const char *value, bool manual, struct list *queue_list);
static bool mpd_client_jukebox_unique_album(t_mpd_state *mpd_state, const char *album, bool manual, struct list *queue_list);
static void mpd_client_jukebox_enforce_last_played(t_mpd_state *mpd_state);

//public functions

bool mpd_client_jukebox(t_mpd_state *mpd_state) {
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (status == NULL) {
        check_error_and_recover(mpd_state, NULL, NULL, 0);
        return false;
    }
    size_t queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);
    
    time_t now = time(NULL);
    time_t add_time = mpd_state->crossfade < mpd_state->song_end_time ? mpd_state->song_end_time - mpd_state->crossfade : 0;
    
    if (queue_length >= mpd_state->jukebox_queue_length && now < add_time) {
        LOG_DEBUG("Jukebox: Queue length >= %d and add_time not reached", mpd_state->jukebox_queue_length);
        return true;
    }

    //add song if add_time is reached or queue is empty
    int addSongs = mpd_state->jukebox_queue_length - queue_length;
    if (now > add_time && add_time > 0) {
        addSongs++;
    }

    if (addSongs < 1) {
        return true;
    }
        
    if (mpd_state->feat_playlists == false && strcmp(mpd_state->jukebox_playlist, "Database") != 0) {
        LOG_WARN("Jukebox: Playlists are disabled");
        return true;
    }

    bool rc = mpd_client_jukebox_add_to_queue(mpd_state, addSongs, mpd_state->jukebox_mode, mpd_state->jukebox_playlist, false);
    
    if (rc == true) {
        if (!mpd_run_play(mpd_state->conn)) {
            check_error_and_recover(mpd_state, NULL, NULL, 0);
        }
    }
    else {
        LOG_ERROR("Error adding song(s), trying again");
        mpd_client_jukebox(mpd_state);
    }
    return rc;
}

bool mpd_client_jukebox_add_to_queue(t_mpd_state *mpd_state, int addSongs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual) {
    if (manual == false) {
        LOG_DEBUG("Jukebox queue length: %d", mpd_state->jukebox_queue.length);
    }
    if ((manual == false && addSongs > mpd_state->jukebox_queue.length) ||
        (manual == true)) 
    {
        LOG_DEBUG("Jukebox queue to small, adding entities");
        if (mpd_state->feat_tags == true) {
            if (mpd_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
                enable_mpd_tags(mpd_state, mpd_state->jukebox_unique_tag);
            }
            else {
                mpd_run_clear_tag_types(mpd_state->conn);
            }
        }
        bool rc =mpd_client_jukebox_fill_jukebox_queue(mpd_state, addSongs, jukebox_mode, playlist, manual);
        if (mpd_state->feat_tags == true) {
            enable_mpd_tags(mpd_state, mpd_state->mympd_tag_types);
        }
        if (rc == false) {
            return false;
        }
    }
    int added = 0;
    struct node *current;
    if (manual == false) {
        current = mpd_state->jukebox_queue.head;
    }
    else {
        current = mpd_state->jukebox_queue_tmp.head;
    }
    while (current != NULL && added < addSongs) {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
	    LOG_INFO("Jukebox adding song: %s", current->key);
	    if (!mpd_run_add(mpd_state->conn, current->key)) {
                check_error_and_recover(mpd_state, NULL, NULL, 0);
            }
            else {
                added++;
            }
        }
        else {
            LOG_INFO("Jukebox adding album: %s", current->key);
            if (!mpd_send_command(mpd_state->conn, "searchadd", "Album", current->key, NULL)) {
                check_error_and_recover(mpd_state, NULL, NULL, 0);
                return false;
            }
            else {
                added++;
            }
            mpd_response_finish(mpd_state->conn);
        }
        if (manual == false) {
            list_shift(&mpd_state->jukebox_queue, 0);
            current = mpd_state->jukebox_queue.head;
        }
        else {
            list_shift(&mpd_state->jukebox_queue_tmp, 0);
            current = mpd_state->jukebox_queue_tmp.head;
        }
    }
    if (added > 0) {
        if (!mpd_run_play(mpd_state->conn)) {
            check_error_and_recover(mpd_state, NULL, NULL, 0);
        }
    }
    else {
        LOG_ERROR("Error adding song(s)");
        return false;
    }
    if (manual == false) {
        if ((jukebox_mode == JUKEBOX_ADD_SONG && mpd_state->jukebox_queue.length < 25) ||
            (jukebox_mode == JUKEBOX_ADD_ALBUM && mpd_state->jukebox_queue.length < 5))
        {
            LOG_DEBUG("Jukebox queue to small, adding entities");
            if (mpd_state->feat_tags == true) {
                if (mpd_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
                    enable_mpd_tags(mpd_state, mpd_state->jukebox_unique_tag);
                }
                else {
                    mpd_run_clear_tag_types(mpd_state->conn);
                }
            }
            bool rc = mpd_client_jukebox_fill_jukebox_queue(mpd_state, addSongs, jukebox_mode, playlist, manual);
            if (mpd_state->feat_tags == true) {
                enable_mpd_tags(mpd_state, mpd_state->mympd_tag_types);
            }
            if (rc == false) {
                return false;
            }
        }
        LOG_DEBUG("Jukebox queue length: %d", mpd_state->jukebox_queue.length);
    }
    return true;
}


//private functions
static bool mpd_client_jukebox_fill_jukebox_queue(t_mpd_state *mpd_state, int addSongs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual) {
    struct mpd_song *song;
    struct mpd_pair *pair;
    unsigned lineno = 1;
    int nkeep = 0;
    
    if (manual == true) {
        list_free(&mpd_state->jukebox_queue_tmp);
    }
    
    //get queue
    struct list *queue_list = (struct list *) malloc(sizeof(struct list));
    assert(queue_list);
    list_init(queue_list);
        
    if (mpd_send_list_queue_meta(mpd_state->conn) == false) {
        list_free(queue_list);
        FREE_PTR(queue_list);
        return false;
    }
    while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
        const char *tag_value = NULL;
        if (mpd_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
            tag_value = mpd_song_get_tag(song, mpd_state->jukebox_unique_tag.tags[0], 0);
        }
        list_push(queue_list, mpd_song_get_uri(song), 0, tag_value, NULL);
        mpd_song_free(song);
    }
    //put last_played to queue list
    struct node *current = mpd_state->last_played.head;
    while (current != NULL) {
        if (mpd_send_list_all_meta(mpd_state->conn, current->key) == false) {
            check_error_and_recover(mpd_state, NULL, NULL, 0);
        }
        else {
            song = mpd_recv_song(mpd_state->conn);
            list_push(queue_list, current->key, 0, mpd_song_get_tag(song, mpd_state->jukebox_unique_tag.tags[0], 0), NULL);
            mpd_song_free(song);
            mpd_response_finish(mpd_state->conn);
        }
        current = current->next;
    }
    
    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        //add songs
        unsigned start = 0;
        unsigned end = start + 1000;
        bool error = false;
        do {
            LOG_DEBUG("Jukebox: iterating through source, start: %u", start);
            if (manual == false) {
                addSongs = 50 - mpd_state->jukebox_queue.length;
            }
            if (strcmp(playlist, "Database") == 0) {
                if (mpd_search_db_songs(mpd_state->conn, false) == false) { error = true; }
                else if (mpd_search_add_uri_constraint(mpd_state->conn, MPD_OPERATOR_DEFAULT, "") == false) { error = true; }
                else if (mpd_search_add_window(mpd_state->conn, start, end) == false) { error = true; }
                else if (mpd_search_commit(mpd_state->conn) == false) { error = true; }
            }
            else {
                if (mpd_send_list_playlist(mpd_state->conn, playlist) == false) { error = true; }
            }
            
            if (error == true) {
                check_error_and_recover(mpd_state, NULL, NULL, 0);
                list_free(queue_list);
                FREE_PTR(queue_list);
                return false;
            }
            
            while ((song = mpd_recv_song(mpd_state->conn)) != NULL) {
                if (randrange(lineno) < addSongs) {
                    const char *tag_value = NULL;
                    if (mpd_state->jukebox_unique_tag.tags[0] != MPD_TAG_TITLE) {
                        tag_value = mpd_song_get_tag(song, mpd_state->jukebox_unique_tag.tags[0], 0);
                    }
                    if (mpd_client_jukebox_unique_tag(mpd_state, mpd_song_get_uri(song), tag_value, manual, queue_list) == true) {
		        if (nkeep < addSongs) {
		            if (manual == false) {
		                list_push(&mpd_state->jukebox_queue, mpd_song_get_uri(song), lineno, tag_value, NULL);
                            }
                            else {
                                list_push(&mpd_state->jukebox_queue_tmp, mpd_song_get_uri(song), lineno, tag_value, NULL);
                            }
                            nkeep++;
                        }
                        else {
                            int i = addSongs > 1 ? randrange(addSongs) : 0;
                            if (manual == false) {
                                list_replace(&mpd_state->jukebox_queue, i, mpd_song_get_uri(song), lineno, tag_value, NULL);
                            }
                            else {
                                list_replace(&mpd_state->jukebox_queue_tmp, i, mpd_song_get_uri(song), lineno, tag_value, NULL);
                            }
                        }
                    }
                }
                lineno++;
                mpd_song_free(song);
            }
            mpd_response_finish(mpd_state->conn);
            start = end;
            end = end + 1000;
        } while (strcmp(playlist, "Database") == 0 && lineno > start);
        LOG_DEBUG("Jukebox iterated through %d songs", lineno);
    }
    else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
        //add album
        bool error = false;
        if (manual == false) {
            addSongs = 10 - mpd_state->jukebox_queue.length;
        }
        
        if (mpd_search_db_tags(mpd_state->conn, MPD_TAG_ALBUM) == false) { error = true; }
        else if (mpd_search_commit(mpd_state->conn) == false) { error = true; }
        
        if (error == true) {
            check_error_and_recover(mpd_state, NULL, NULL, 0);
            list_free(queue_list);
            FREE_PTR(queue_list);
            return false;
        }
        while ((pair = mpd_recv_pair_tag(mpd_state->conn, MPD_TAG_ALBUM )) != NULL)  {
            if (randrange(lineno) < addSongs) {
                if (mpd_client_jukebox_unique_album(mpd_state, pair->value, manual, queue_list) == true) {
		    if (nkeep < addSongs) {
		        if (manual == false) {
                            list_push(&mpd_state->jukebox_queue, pair->value, lineno, NULL, NULL);
                        }
                        else {
                            list_push(&mpd_state->jukebox_queue_tmp, pair->value, lineno, NULL, NULL);
                        }
                        nkeep++;
                    }
                    else {
                        int i = addSongs > 1 ? randrange(addSongs) : 0;
                        if (manual == false) {
                            list_replace(&mpd_state->jukebox_queue, i, pair->value, lineno, NULL, NULL);
                        }
                        else {
                            list_replace(&mpd_state->jukebox_queue_tmp, i, pair->value, lineno, NULL, NULL);
                        }
                    }
                }
            }
            lineno++;
            mpd_return_pair(mpd_state->conn, pair);
        }
    }

    if (nkeep < addSongs) {
        LOG_WARN("Jukebox queue didn't contain %d entries", addSongs);
    }

    //finally shuffle the list
    if (manual == false) {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
            mpd_client_jukebox_enforce_last_played(mpd_state);
        }
    }
    list_free(queue_list);
    FREE_PTR(queue_list);
    return true;
}

static bool mpd_client_jukebox_unique_tag(t_mpd_state *mpd_state, const char *uri, const char *value, bool manual, struct list *queue_list) {
    struct node *current = queue_list->head;
    while(current != NULL) {
        if (strcmp(current->key, uri) == 0) {
            return false;
        }
        else if (value != NULL && strcmp(current->value_p, value) == 0) {
            return false;
        }
        current = current->next;
    }
    
    if (manual == false) {
        current = mpd_state->jukebox_queue.head;
    }
    else {
        current = mpd_state->jukebox_queue_tmp.head;
    }
    while (current != NULL) {
        if (strcmp(current->key, uri) == 0) {
            return false;
        }
        else if (value != NULL && strcmp(current->value_p, value) == 0) {
            return false;
        }
        current = current->next;
    }
    return true;
}

static bool mpd_client_jukebox_unique_album(t_mpd_state *mpd_state, const char *album, bool manual, struct list *queue_list) {
    struct node *current = queue_list->head;
    while (current != NULL) {
        if (strcmp(current->value_p, album) == 0) {
            return false;
        }
        current = current->next;
    }
    
    if (manual == false) {
        current = mpd_state->jukebox_queue.head;
    }
    else {
        current = mpd_state->jukebox_queue_tmp.head;
    }
    while (current != NULL) {
        if (strcmp(current->key, album) == 0) {
            return false;
        }
        current = current->next;
    }
    return true;
}

static void mpd_client_jukebox_enforce_last_played(t_mpd_state *mpd_state) {
    if (mpd_state->jukebox_last_played == 0 || mpd_state->feat_sticker == false) {
        return;
    }
    t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
    assert(sticker);

    time_t now = time(NULL);
    now = now - mpd_state->jukebox_last_played * 60 * 60;
    
    long i = 0;
    struct node *current = mpd_state->jukebox_queue.head;
    while (current != NULL) {
        sticker->lastPlayed = 0;
        mpd_client_get_sticker(mpd_state, current->key, sticker);
        current = current->next;
        if (sticker->lastPlayed != 0 && sticker->lastPlayed > now) {
            list_shift(&mpd_state->jukebox_queue, i);
        }
        else {
            i++;
        }
    }
    FREE_PTR(sticker);
}
