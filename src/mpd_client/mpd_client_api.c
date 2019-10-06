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
#include <signal.h>
#include <assert.h>
#include <mpd/client.h>

#include "../../dist/src/sds/sds.h"
#include "../dist/src/frozen/frozen.h"
#include "../utility.h"
#include "../api.h"
#include "../log.h"
#include "../list.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "config_defs.h"
#include "mpd_client_utility.h"
#include "mpd_client_browse.h"
#include "mpd_client_cover.h" 
#include "mpd_client_features.h"
#include "mpd_client_jukebox.h"
#include "mpd_client_playlists.h"
#include "mpd_client_queue.h"
#include "mpd_client_search.h"
#include "mpd_client_state.h"
#include "mpd_client_stats.h"
#include "mpd_client_settings.h"
#include "mpd_client_api.h"

void mpd_client_api(t_config *config, t_mpd_state *mpd_state, void *arg_request) {
    t_work_request *request = (t_work_request*) arg_request;
    unsigned int uint_buf1, uint_buf2;
    int je, int_buf1; 
    bool bool_buf, rc;
    char *p_charbuf1 = NULL;
    char *p_charbuf2 = NULL;
    char *p_charbuf3 = NULL;
    char *p_charbuf4 = NULL;

    LOG_VERBOSE("MPD CLIENT API request (%d): %s", request->conn_id, request->data);
    //create response struct
    t_work_result *response = (t_work_result*)malloc(sizeof(t_work_result));
    assert(response);
    response->conn_id = request->conn_id;
    sds data = sdsempty();
    
    switch(request->cmd_id) {
        case MPD_API_LOVE:
            if (mpd_run_send_message(mpd_state->conn, mpd_state->love_channel, mpd_state->love_message)) {
                data = jsonrpc_respond_message(data, request->method, request->id, "Scrobbled love", false);
            }
            else {
                data = jsonrpc_respond_message(data, request->method, request->id, "Failed to send love message to channel", true);
            }
        break;
        case MPD_API_LIKE:
            if (mpd_state->feat_sticker) {
                je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q, like: %d}}", &p_charbuf1, &uint_buf1);
                if (je == 2) {        
                    data = mpd_client_like_song_uri(mpd_state, data, request->method, request->id, p_charbuf1, uint_buf1);
                    FREE_PTR(p_charbuf1);
                }
            } 
            else {
                data = jsonrpc_respond_message(data, request->method, request->id, "MPD stickers are disabled", true);
                LOG_ERROR("MPD stickers are disabled");
            }
            break;
        case MPD_API_PLAYER_STATE:
            data = mpd_client_put_state(mpd_state, data, request->method, request->id);
            break;
        case MYMPD_API_SETTINGS_SET: {
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            rc = true;
            bool mpd_host_changed = false;
            while ((h = json_next_key(request->data, sdslen(request->data), h, ".data", &key, &val)) != NULL) {
                rc = mpd_api_settings_set(config, mpd_state, &key, &val, &mpd_host_changed);
                if (rc == false) {
                    break;
                }
            }
            if (rc == true) {
                if (mpd_host_changed == true) {
                    //reconnect with new settings
                    mpd_state->conn_state = MPD_DISCONNECT;
                }
                if (mpd_state->conn_state == MPD_CONNECTED) {
                    //feature detection
                    mpd_client_mpd_features(mpd_state);
                    
                    if (mpd_state->jukebox_mode != JUKEBOX_OFF) {
                        //enable jukebox
                        mpd_client_jukebox(mpd_state);
                    }
                }
                data = jsonrpc_respond_ok(data, request->method, request->id);
            }
            else {
                data = jsonrpc_start_phrase(data, request->method, request->id, "Can't save setting %{setting}", true);
                data = tojson_char_len(data, "setting", val.ptr, val.len, false);
                data = jsonrpc_end_phrase(data);
            }
            break;
        }
        case MPD_API_DATABASE_UPDATE:
            mpd_run_update(mpd_state->conn, NULL);
            data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_DATABASE_RESCAN:
            mpd_run_rescan(mpd_state->conn, NULL);
            data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_SMARTPLS_UPDATE_ALL:
            rc = mpd_client_smartpls_update_all(config, mpd_state);
            if (rc == true) {
                data = jsonrpc_respond_message(data, request->method, request->id, "Smart playlists updated", false);
            }
            else {
                data = jsonrpc_respond_message(data, request->method, request->id, "Smart playlists update failed", true);
            }
            break;
        case MPD_API_SMARTPLS_UPDATE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {playlist: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_client_smartpls_update(config, mpd_state, p_charbuf1);
                if (rc == true) {
                    data = jsonrpc_start_phrase(data, request->method, request->id, "Smart playlist %{playlist} updated", false);
                    data = tojson_char(data, "playlist", p_charbuf1, false);
                    data = jsonrpc_end_phrase(data);
                }
                else {
                    data = jsonrpc_start_phrase(data, request->method, request->id, "Updating of smart playlist %{playlist} failed", true);
                    data = tojson_char(data, "playlist", p_charbuf1, false);
                    data = jsonrpc_end_phrase(data);
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_SMARTPLS_SAVE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {type: %Q}}", &p_charbuf1);
            rc = false;
            if (je == 1) {
                if (strcmp(p_charbuf1, "sticker") == 0) {
                    je = json_scanf(request->data, sdslen(request->data), "{params: {playlist: %Q, sticker: %Q, maxentries: %d}}", &p_charbuf2, &p_charbuf3, &int_buf1);
                    if (je == 3) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, p_charbuf3, NULL, int_buf1, 0);
                        FREE_PTR(p_charbuf2);
                        FREE_PTR(p_charbuf3);
                    }
                }
                else if (strcmp(p_charbuf1, "newest") == 0) {
                    je = json_scanf(request->data, sdslen(request->data), "{params: {playlist: %Q, timerange: %d}}", &p_charbuf2, &int_buf1);
                    if (je == 2) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, NULL, NULL, 0, int_buf1);
                        FREE_PTR(p_charbuf2);
                    }
                }            
                else if (strcmp(p_charbuf1, "search") == 0) {
                    je = json_scanf(request->data, sdslen(request->data), "{params: {playlist: %Q, tag: %Q, searchstr: %Q}}", &p_charbuf2, &p_charbuf3, &p_charbuf4);
                    if (je == 3) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, p_charbuf3, p_charbuf4, 0, 0);
                        FREE_PTR(p_charbuf2);
                        FREE_PTR(p_charbuf3);                    
                        FREE_PTR(p_charbuf4);
                    }
                }
                FREE_PTR(p_charbuf1);
            }
            if (rc == true) {
                data = jsonrpc_respond_ok(data, request->method, request->id);
            }
            else {
                data = jsonrpc_respond_message(data, request->method, request->id, "Failed to save playlist", true);
            }
            break;
        case MPD_API_SMARTPLS_GET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {playlist: %Q}}", &p_charbuf1);
            if (je == 1) {
                data = mpd_client_smartpls_put(config, data, request->method, request->id, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PAUSE:
            mpd_run_toggle_pause(mpd_state->conn);
            data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_PLAYER_PREV:
            mpd_run_previous(mpd_state->conn);
            data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_PLAYER_NEXT:
            mpd_run_next(mpd_state->conn);
            data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_PLAYER_PLAY:
            mpd_run_play(mpd_state->conn);
            data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_PLAYER_STOP:
            mpd_run_stop(mpd_state->conn);
            data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_QUEUE_CLEAR:
            mpd_run_clear(mpd_state->conn);
            data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_QUEUE_CROP:
            data = mpd_client_crop_queue(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_QUEUE_RM_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {track:%u}}", &uint_buf1);
            if (je == 1) {
                mpd_run_delete_id(mpd_state->conn, uint_buf1);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            }
            break;
        case MPD_API_QUEUE_RM_RANGE:
            je = json_scanf(request->data, sdslen(request->data), "{params: {start: %u, end: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                mpd_run_delete_range(mpd_state->conn, uint_buf1, uint_buf2);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            }
            break;
        case MPD_API_QUEUE_MOVE_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {from: %u, to: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2) {
                    uint_buf2--;
                }
                mpd_run_move(mpd_state->conn, uint_buf1, uint_buf2);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            }
            break;
        case MPD_API_PLAYLIST_MOVE_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {plist: %Q, from: %u, to: %u }}", &p_charbuf1, &uint_buf1, &uint_buf2);
            if (je == 3) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2) {
                    uint_buf2--;
                }
                mpd_send_playlist_move(mpd_state->conn, p_charbuf1, uint_buf1, uint_buf2);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PLAY_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: { track:%u}}", &uint_buf1);
            if (je == 1) {
                mpd_run_play_id(mpd_state->conn, uint_buf1);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            }
            break;
        case MPD_API_PLAYER_OUTPUT_LIST:
            data = mpd_client_put_outputs(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_PLAYER_TOGGLE_OUTPUT:
            je = json_scanf(request->data, sdslen(request->data), "{params: {output: %u, state: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (uint_buf2 == 1) {
                    mpd_run_enable_output(mpd_state->conn, uint_buf1);
                }
                else {
                    mpd_run_disable_output(mpd_state->conn, uint_buf1);
                }
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            }
            break;
        case MPD_API_PLAYER_VOLUME_SET:
            je = json_scanf(request->data, sdslen(request->data), "{params: {volume:%u}}", &uint_buf1);
            if (je == 1) {
                mpd_run_set_volume(mpd_state->conn, uint_buf1);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            }
            break;
        case MPD_API_PLAYER_VOLUME_GET:
            data = mpd_client_put_volume(mpd_state, data, request->method, request->id);
            break;            
        case MPD_API_PLAYER_SEEK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {songid: %u, seek: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                mpd_run_seek_id(mpd_state->conn, uint_buf1, uint_buf2);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            }
            break;
        case MPD_API_QUEUE_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, cols: %M}}", &uint_buf1, json_to_tags, tagcols);
            if (je == 2) {
                data = mpd_client_put_queue(mpd_state, data, request->method, request->id, uint_buf1, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_LAST_PLAYED: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, cols: %M}}", &uint_buf1, json_to_tags, tagcols);
            if (je == 2) {
                data = mpd_client_put_last_played_songs(config, mpd_state, data, request->method, request->id, uint_buf1, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYER_CURRENT_SONG: {
            data = mpd_client_put_current_song(config, mpd_state, data, request->method, request->id);
            break;
        }
        case MPD_API_DATABASE_SONGDETAILS:
            je = json_scanf(request->data, sdslen(request->data), "{params: { uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                data = mpd_client_put_songdetails(config, mpd_state, data, request->method, request->id, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_FINGERPRINT:
            if (mpd_state->feat_fingerprint == true) {
                je = json_scanf(request->data, sdslen(request->data), "{params: { uri: %Q}}", &p_charbuf1);
                if (je == 1) {
                    data = mpd_client_put_fingerprint(mpd_state, data, request->method, request->id, p_charbuf1);
                    FREE_PTR(p_charbuf1);
                }
            }
            else {
                data = jsonrpc_respond_message(data, request->method, request->id, "fingerprint command not supported", true);
            }
            break;
        case MPD_API_DATABASE_TAG_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, filter: %Q, tag: %Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                data = mpd_client_put_db_tag(mpd_state, data, request->method, request->id, uint_buf1, p_charbuf2, "", "", p_charbuf1);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, filter: %Q, search: %Q, tag: %Q}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 4) {
                data = mpd_client_put_db_tag(mpd_state, data, request->method, request->id, uint_buf1, "Album", p_charbuf3, p_charbuf2, p_charbuf1);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {album: %Q, search: %Q, tag: %Q, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &p_charbuf3, json_to_tags, tagcols);
            if (je == 4) {
                data = mpd_client_put_songs_in_album(config, mpd_state, data, request->method, request->id, p_charbuf1, p_charbuf2, p_charbuf3, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYLIST_RENAME:
            je = json_scanf(request->data, sdslen(request->data), "{params: {from: %Q, to: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                data = mpd_client_playlist_rename(config, mpd_state, data, request->method, request->id, p_charbuf1, p_charbuf2);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            break;            
        case MPD_API_PLAYLIST_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset: %u, filter: %Q}}", &uint_buf1, &p_charbuf1);
            if (je == 2) {
                data = mpd_client_put_playlists(config, mpd_state, data, request->method, request->id, uint_buf1, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_CONTENT_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri: %Q, offset:%u, filter:%Q, cols: %M}}", 
                &p_charbuf1, &uint_buf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                data = mpd_client_put_playlist_list(config, mpd_state, data, request->method, request->id, p_charbuf1, uint_buf1, p_charbuf2, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYLIST_ADD_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {plist:%Q, uri:%Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                if (mpd_run_playlist_add(mpd_state->conn, p_charbuf1, p_charbuf2)) {
                    data = jsonrpc_start_phrase(data, request->method, request->id, "Added %{uri} to playlist %{playlist}", false);
                    data = tojson_char(data, "uri", p_charbuf2, true);
                    data = tojson_char(data, "playlist", p_charbuf1, false);
                    data = jsonrpc_end_phrase(data);
                }
                else {
                    data = check_error_and_recover(mpd_state, data, request->method, request->id);
                }
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);                
            }
            break;
        case MPD_API_PLAYLIST_CLEAR:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                mpd_run_playlist_clear(mpd_state->conn, p_charbuf1);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_RM_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q, track:%u}}", &p_charbuf1, &uint_buf1);
            if (je == 2) {
                mpd_run_playlist_delete(mpd_state->conn, p_charbuf1, uint_buf1);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_FILESYSTEM_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset:%u, filter:%Q, path:%Q, cols: %M}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                data = mpd_client_put_filesystem(config, mpd_state, data, request->method, request->id, p_charbuf2, uint_buf1, p_charbuf1, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_ADD_TRACK_AFTER:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q, to:%d}}", &p_charbuf1, &int_buf1);
            if (je == 2) {
                mpd_run_add_id_to(mpd_state->conn, p_charbuf1, int_buf1);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_REPLACE_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q }}", &p_charbuf1);
            if (je == 1) {
                if (mpd_command_list_begin(mpd_state->conn, false)) {
                    mpd_send_clear(mpd_state->conn);
                    mpd_send_add(mpd_state->conn, p_charbuf1);
                    mpd_send_play(mpd_state->conn);
                    if (mpd_command_list_end(mpd_state->conn)) {
                        mpd_response_finish(mpd_state->conn);
                    }
                }
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                mpd_run_add(mpd_state->conn, p_charbuf1);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_PLAY_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                int_buf1 = mpd_run_add_id(mpd_state->conn, p_charbuf1);
                if (int_buf1 != -1) {
                    mpd_run_play_id(mpd_state->conn, int_buf1);
                }
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_REPLACE_PLAYLIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_command_list_begin(mpd_state->conn, false)) {
                    mpd_send_clear(mpd_state->conn);
                    mpd_send_load(mpd_state->conn, p_charbuf1);
                    mpd_send_play(mpd_state->conn);
                    if (mpd_command_list_end(mpd_state->conn)) {
                        mpd_response_finish(mpd_state->conn);
                    }
                }
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_RANDOM:
            je = json_scanf(request->data, sdslen(request->data), "{params: {mode:%u, playlist:%Q, quantity:%d}}", &uint_buf1, &p_charbuf1, &int_buf1);
            if (je == 3) {
                rc = mpd_client_jukebox_add(mpd_state, int_buf1, uint_buf1, p_charbuf1);
                FREE_PTR(p_charbuf1);
                if (rc == true) {
                    data = jsonrpc_respond_message(data, request->method, request->id, "Sucessfully added random songs to queue", false);
                }
                else {
                    data = jsonrpc_respond_message(data, request->method, request->id, "Adding random songs to queue failed", true);
                }
            }
            break;
        case MPD_API_QUEUE_ADD_PLAYLIST:
            je = json_scanf(request->data, sdslen(request->data), "{params: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                mpd_run_load(mpd_state->conn, p_charbuf1);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_SAVE:
            je = json_scanf(request->data, sdslen(request->data), "{ params: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                mpd_run_save(mpd_state->conn, p_charbuf1);
                data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_SEARCH: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {offset:%u, filter:%Q, searchstr:%Q, cols: %M}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                data = mpd_client_search_queue(mpd_state, data, request->method, request->id, p_charbuf1, uint_buf1, p_charbuf2, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_DATABASE_SEARCH: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {searchstr:%Q, filter:%Q, plist:%Q, offset:%u, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &p_charbuf3, &uint_buf1, json_to_tags, tagcols);
            if (je == 5) {
                data = mpd_client_search(mpd_state, data, request->method, request->id, p_charbuf1, p_charbuf2, p_charbuf3, uint_buf1, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            free(tagcols);
            break;
        }
        case MPD_API_DATABASE_SEARCH_ADV: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{params: {expression:%Q, sort:%Q, sortdesc:%B, plist:%Q, offset:%u, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &bool_buf, &p_charbuf3, &uint_buf1, json_to_tags, tagcols);
            if (je == 6) {
                data = mpd_client_search_adv(mpd_state, data, request->method, request->id, p_charbuf1, p_charbuf2, bool_buf, NULL, p_charbuf3, uint_buf1, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_SHUFFLE:
            mpd_run_shuffle(mpd_state->conn);
            data = respond_with_mpd_error_or_ok(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_PLAYLIST_RM:
            je = json_scanf(request->data, sdslen(request->data), "{params: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                data = mpd_client_playlist_delete(config, mpd_state, data, request->method, request->id, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_SETTINGS_GET:
            data = mpd_client_put_settings(mpd_state, data, request->method, request->id);
            break;
        case MPD_API_DATABASE_STATS:
            data = mpd_client_put_stats(mpd_state, data, request->method, request->id);
            break;
        default:
            data = jsonrpc_respond_message(data, request->method, request->id, "Unknown request", true);
            LOG_ERROR("Unknown API request: %.*s", sdslen(request->data), request->data);
    }

    if (sdslen(data) == 0) {
        data = jsonrpc_start_phrase(data, request->method, request->id, "No response for method %{method}", true);
        data = tojson_char(data, "method", request->method, false);
        data = jsonrpc_end_phrase(data);
        LOG_ERROR("No response for cmd_id %u", request->cmd_id);
    }
    if (response->conn_id > -1) {
        LOG_DEBUG("Push response to queue for connection %lu: %s", request->conn_id, data);
        response->data = data;
        tiny_queue_push(web_server_queue, response);
    }
    else {
        sds_free(data);
        FREE_PTR(response);
    }
    sds_free(request->data);
    sds_free(request->method);
    FREE_PTR(request);
}
