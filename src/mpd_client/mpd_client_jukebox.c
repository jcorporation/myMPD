/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mpd_client_utility.h"
#include "mpd_client_jukebox.h"

bool mpd_client_jukebox(t_mpd_state *mpd_state) {
    int addSongs;
    
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
    addSongs = mpd_state->jukebox_queue_length - queue_length;
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

    bool rc = mpd_client_jukebox_add(mpd_state, addSongs, mpd_state->jukebox_mode, mpd_state->jukebox_playlist);
    
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

bool mpd_client_jukebox_add(t_mpd_state *mpd_state, const int addSongs, const enum jukebox_modes jukebox_mode, const char *jukebox_playlist) {
    int i;
    struct mpd_entity *entity;
    const struct mpd_song *song;
    struct mpd_pair *pair;
    int lineno = 1;
    int nkeep = 0;
    
    if (addSongs < 1) {
        return false;
    }
        
    struct list add_list;
    list_init(&add_list);
    
    if (jukebox_mode == JUKEBOX_ADD_SONG) {
        //add songs
        if (strcmp(jukebox_playlist, "Database") == 0) {
            if (!mpd_send_list_all(mpd_state->conn, "/")) {
                check_error_and_recover(mpd_state, NULL, NULL, 0);
                list_free(&add_list);
                return false;
            }
        }
        else {
            if (!mpd_send_list_playlist(mpd_state->conn, jukebox_playlist)) {
                check_error_and_recover(mpd_state, NULL, NULL, 0);
                list_free(&add_list);
                return false;
            }
        }
        while ((entity = mpd_recv_entity(mpd_state->conn)) != NULL) {
            if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
                if (randrange(lineno) < addSongs) {
		    if (nkeep < addSongs) {
		        song = mpd_entity_get_song(entity);
		        list_push(&add_list, mpd_song_get_uri(song), lineno, NULL);
		        nkeep++;
                    }
                    else {
                        i = addSongs > 1 ? randrange(addSongs) : 0;
                        song = mpd_entity_get_song(entity);
                        list_replace(&add_list, i, mpd_song_get_uri(song), lineno, NULL);
                    }
                }
                lineno++;
            }
            mpd_entity_free(entity);
        }
    }
    else if (jukebox_mode == JUKEBOX_ADD_ALBUM) {
        //add album
        if (!mpd_search_db_tags(mpd_state->conn, MPD_TAG_ALBUM)) {
            check_error_and_recover(mpd_state, NULL, NULL, 0);
            list_free(&add_list);
            return false;
        }
        if (!mpd_search_commit(mpd_state->conn)) {
            check_error_and_recover(mpd_state, NULL, NULL, 0);
            list_free(&add_list);
            return false;
        }
        while ((pair = mpd_recv_pair_tag(mpd_state->conn, MPD_TAG_ALBUM )) != NULL)  {
            if (randrange(lineno) < addSongs) {
		if (nkeep < addSongs) {
                    list_push(&add_list, pair->value, lineno, NULL);
                    nkeep++;
                }
		else {
                    i = addSongs > 1 ? randrange(addSongs) : 0;
                    list_replace(&add_list, i, pair->value, lineno, NULL);
                }
            }
            lineno++;
            mpd_return_pair(mpd_state->conn, pair);
        }
    }

    if (nkeep < addSongs) {
        LOG_WARN("Input didn't contain %d entries", addSongs);
    }

    list_shuffle(&add_list);

    nkeep = 0;
    struct node *current = add_list.list;
    while (current != NULL) {
        if (jukebox_mode == JUKEBOX_ADD_SONG) {
	    LOG_INFO("Jukebox adding song: %s", current->data);
	    if (!mpd_run_add(mpd_state->conn, current->data)) {
                check_error_and_recover(mpd_state, NULL, NULL, 0);
            }
            else {
                nkeep++;
            }
        }
        else {
            LOG_INFO("Jukebox adding album: %s", current->data);
            if (!mpd_send_command(mpd_state->conn, "searchadd", "Album", current->data, NULL)) {
                check_error_and_recover(mpd_state, NULL, NULL, 0);
                return false;
            }
            else {
                nkeep++;
            }
            mpd_response_finish(mpd_state->conn);
        }
        current = current->next;
    }
    list_free(&add_list);
    if (nkeep > 0) {
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
