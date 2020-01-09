/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    if ((jukebox_mode == JUKEBOX_ADD_SONG && mpd_state->jukebox_queue.length < 50) ||
        (jukebox_mode == JUKEBOX_ADD_ALBUM && mpd_state->jukebox_queue.length < 5) ||
        addSongs > mpd_state->jukebox_queue.length)
    {
        if (!mpd_client_jukebox_fill_jukebox_queue(mpd_state, addSongs, jukebox_mode, playlist, manual)) {
            return false;
        }
    }
    int added = 0;
    struct node *current = mpd_state->jukebox_queue.list;
    while (current != NULL) {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
	    LOG_INFO("Jukebox adding song: %s", current->data);
	    if (!mpd_run_add(mpd_state->conn, current->data)) {
                check_error_and_recover(mpd_state, NULL, NULL, 0);
            }
            else {
                added++;
            }
        }
        else {
            LOG_INFO("Jukebox adding album: %s", current->data);
            if (!mpd_send_command(mpd_state->conn, "searchadd", "Album", current->data, NULL)) {
                check_error_and_recover(mpd_state, NULL, NULL, 0);
                return false;
            }
            else {
                added++;
            }
            mpd_response_finish(mpd_state->conn);
        }
        list_shift(&mpd_state->jukebox_queue, 0);
        current = mpd_state->jukebox_queue.list;
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
    return true;
}

bool mpd_client_jukebox_fill_jukebox_queue(t_mpd_state *mpd_state, int addSongs, enum jukebox_modes jukebox_mode, const char *playlist, bool manual) {
    int i;
    struct mpd_entity *entity;
    const struct mpd_song *song;
    struct mpd_pair *pair;
    int lineno = 1;
    int nkeep = 0;
    (void) manual;
    
    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        //add songs
        nkeep = 100 - mpd_state->jukebox_queue.length;
        if (strcmp(playlist, "Database") == 0) {
            if (!mpd_send_list_all(mpd_state->conn, "/")) {
                check_error_and_recover(mpd_state, NULL, NULL, 0);
                return false;
            }
        }
        else {
            if (!mpd_send_list_playlist(mpd_state->conn, playlist)) {
                check_error_and_recover(mpd_state, NULL, NULL, 0);
                return false;
            }
        }
        while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
            if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
                if (randrange(lineno) < addSongs) {
		    if (nkeep < addSongs) {
		        song = mpd_entity_get_song(entity);
		        list_push(&mpd_state->jukebox_queue, mpd_song_get_uri(song), lineno, NULL);
		        nkeep++;
                    }
                    else {
                        i = addSongs > 1 ? randrange(addSongs) : 0;
                        song = mpd_entity_get_song(entity);
                        list_replace(&mpd_state->jukebox_queue, i, mpd_song_get_uri(song), lineno, NULL);
                    }
                }
                lineno++;
            }
            mpd_entity_free(entity);
        }
    }
    else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
        //add album
        nkeep = 10 - mpd_state->jukebox_queue.length;
        if (!mpd_search_db_tags(mpd_state->conn, MPD_TAG_ALBUM)) {
            check_error_and_recover(mpd_state, NULL, NULL, 0);
            return false;
        }
        if (!mpd_search_commit(mpd_state->conn)) {
            check_error_and_recover(mpd_state, NULL, NULL, 0);
            return false;
        }
        while ((pair = mpd_recv_pair_tag(mpd_state->conn, MPD_TAG_ALBUM )) != NULL)  {
            if (randrange(lineno) < addSongs) {
		if (nkeep < addSongs) {
                    list_push(&mpd_state->jukebox_queue, pair->value, lineno, NULL);
                    nkeep++;
                }
		else {
                    i = addSongs > 1 ? randrange(addSongs) : 0;
                    list_replace(&mpd_state->jukebox_queue, i, pair->value, lineno, NULL);
                }
            }
            lineno++;
            mpd_return_pair(mpd_state->conn, pair);
        }
    }

    if (nkeep < addSongs) {
        LOG_WARN("Input didn't contain %d entries", addSongs);
    }

    //finally shuffle the list
    list_shuffle(&mpd_state->jukebox_queue);
    return true;
}
