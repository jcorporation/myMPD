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
#include "cover.h" 
#include "features.h"
#include "jukebox.h"
#include "playlists.h"
#include "queue.h"
#include "search.h"
#include "state.h"
#include "stats.h"
#include "mpd_client_api.h"
#include "../dist/src/frozen/frozen.h"

//private definitions
static bool mpd_api_settings_set(t_config *config, t_mpd_state *mpd_state, struct json_token *key, struct json_token *val, bool *mpd_host_changed);
static int mpd_client_put_browse(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *path, const unsigned int offset, const char *filter, const t_tags *tagcols);
static int mpd_client_put_db_tag(t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter);
static int mpd_client_put_songs_in_album(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *album, const char *search, const char *tag, const t_tags *tagcols);
static int mpd_client_put_songdetails(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri);
static int mpd_client_put_fingerprint(t_mpd_state *mpd_state, char *buffer, const char *uri);

//public functions
void mpd_client_api(t_config *config, t_mpd_state *mpd_state, void *arg_request) {
    t_work_request *request = (t_work_request*) arg_request;
    unsigned int uint_buf1, uint_buf2, uint_rc;
    int je, int_buf1, int_rc; 
    float float_buf;
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
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"Scrobbled love\"}");
            }
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"Failed to send love message to channel\"}");
            }
        break;
        case MPD_API_LIKE:
            if (mpd_state->feat_sticker) {
                je = json_scanf(request->data, sdslen(request->data), "{data: {uri: %Q, like: %d}}", &p_charbuf1, &uint_buf1);
                if (je == 2) {        
                    data = mpd_client_like_song_uri(mpd_state, buffer, p_charbuf1, uint_buf1)) {
                    FREE_PTR(p_charbuf1);
                }
            } 
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"MPD stickers are disabled\"}");
                LOG_ERROR("MPD stickers are disabled");
            }
            break;
        case MPD_API_PLAYER_STATE:
            data = mpd_client_get_state(mpd_state, data);
            break;
        case MYMPD_API_SETTINGS_SET:
            void *h = NULL;
            struct json_token key;
            struct json_token val;
            bool rc = true;
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
                    mpd_client_feature(mpd_state);
                    
                    if (mpd_state->jukebox_mode != JUKEBOX_OFF) {
                        //enable jukebox
                        mpd_client_jukebox(mpd_state);
                    }
                }
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                sds settingname = sdscatrepr(sdsempty(), val.ptr, val.len);
                data = sdscatprintf(data, "{\"type\": \"error\", \"data\": \"Can't save setting %%{setting}\", \"values\": {\"setting\": \"%s\"}}", settingname);
                sds_free(settingname);
            }
            break;

        case MPD_API_DATABASE_UPDATE:
            uint_rc = mpd_run_update(mpd_state->conn, NULL);
            if (uint_rc > 0) {
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_DATABASE_RESCAN:
            uint_rc = mpd_run_rescan(mpd_state->conn, NULL);
            if (uint_rc > 0) {
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_SMARTPLS_UPDATE_ALL:
            rc = mpd_client_smartpls_update_all(config, mpd_state);
            if (rc == true) {
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"Smart playlists updated\"}");
            }
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"Smart playlists update failed\"}");
            }
            break;
        case MPD_API_SMARTPLS_UPDATE:
            je = json_scanf(request->data, sdslen(request->data), "{data: {playlist: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_client_smartpls_update(config, mpd_state, p_charbuf1);
                if (rc == true) {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"Smart playlist %%{playlist} updated\", \"values\": {\"playlist\": \"%s\"}}", p_charbuf1);
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Smart playlist update failed\"}");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_SMARTPLS_SAVE:
            je = json_scanf(request->data, sdslen(request->data), "{data: {type: %Q}}", &p_charbuf1);
            rc = false;
            if (je == 1) {
                if (strcmp(p_charbuf1, "sticker") == 0) {
                    je = json_scanf(request->data, sdslen(request->data), "{data: {playlist: %Q, sticker: %Q, maxentries: %d}}", &p_charbuf2, &p_charbuf3, &int_buf1);
                    if (je == 3) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, p_charbuf3, NULL, int_buf1, 0);
                        FREE_PTR(p_charbuf2);
                        FREE_PTR(p_charbuf3);
                    }
                }
                else if (strcmp(p_charbuf1, "newest") == 0) {
                    je = json_scanf(request->data, sdslen(request->data), "{data: {playlist: %Q, timerange: %d}}", &p_charbuf2, &int_buf1);
                    if (je == 2) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, NULL, NULL, 0, int_buf1);
                        FREE_PTR(p_charbuf2);
                    }
                }            
                else if (strcmp(p_charbuf1, "search") == 0) {
                    je = json_scanf(request->data, sdslen(request->data), "{data: {playlist: %Q, tag: %Q, searchstr: %Q}}", &p_charbuf2, &p_charbuf3, &p_charbuf4);
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
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"Failed to save playlist\"}");
            }
            break;
        case MPD_API_SMARTPLS_GET:
            je = json_scanf(request->data, sdslen(request->data), "{data: {playlist: %Q}}", &p_charbuf1);
            if (je == 1) {
                data = mpd_client_smartpls_put(config, data, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PAUSE:
            if (mpd_run_toggle_pause(mpd_state->conn)) {
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"Toggling player pause failed.\"}");
                LOG_ERROR("Error mpd_run_toggle_pause()");
            }
            break;
        case MPD_API_PLAYER_PREV:
            if (mpd_run_previous(mpd_state->conn)) {
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"Goto previous song failed\"}");
                LOG_ERROR("Error mpd_run_previous()");
            }
            break;
        case MPD_API_PLAYER_NEXT:
            if (mpd_run_next(mpd_state->conn)) {
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"Skip to next song failed\"}");
                LOG_ERROR("Error mpd_run_next()");
            }
            break;
        case MPD_API_PLAYER_PLAY:
            if (mpd_run_play(mpd_state->conn)) {
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"Begin to play failed\"}");
                LOG_ERROR("Error mpd_run_play()");
            }
            break;
        case MPD_API_PLAYER_STOP:
            if (mpd_run_stop(mpd_state->conn)) {
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"Stop playing failed\"}");
                LOG_ERROR("Error mpd_run_stop()");
            }
            break;
        case MPD_API_QUEUE_CLEAR:
            if (mpd_run_clear(mpd_state->conn)) {
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"Clearing queue failed\"}");
                LOG_ERROR("Error mpd_run_clear()");
            }
            break;
        case MPD_API_QUEUE_CROP:
            data = mpd_client_queue_crop(mpd_state, data);
            break;
        case MPD_API_QUEUE_RM_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{data: {track:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_delete_id(mpd_state->conn, uint_buf1))
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Removing track from queue failed\"}");
                    LOG_ERROR("Error mpd_run_delete_id()");
                }
            }
            break;
        case MPD_API_QUEUE_RM_RANGE:
            je = json_scanf(request->data, sdslen(request->data), "{data: {start: %u, end: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (mpd_run_delete_range(mpd_state->conn, uint_buf1, uint_buf2))
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Removing track range from queue failed\"}");
                    LOG_ERROR("Error mpd_run_delete_range()");
                }
            }
            break;
        case MPD_API_QUEUE_MOVE_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{data: {from: %u, to: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2)
                    uint_buf2--;
                if (mpd_run_move(mpd_state->conn, uint_buf1, uint_buf2))
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Moving track in queue failed\"}");
                    LOG_ERROR("Error mpd_run_move()");
                }
            }
            break;
        case MPD_API_PLAYLIST_MOVE_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{data: {plist: %Q, from: %u, to: %u }}", &p_charbuf1, &uint_buf1, &uint_buf2);
            if (je == 3) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2)
                    uint_buf2--;
                if (mpd_send_playlist_move(mpd_state->conn, p_charbuf1, uint_buf1, uint_buf2)) {
                    mpd_response_finish(mpd_state->conn);
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Moving track in playlist failed\"}");
                    LOG_ERROR("Error mpd_send_playlist_move()");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PLAY_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{data: { track:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_play_id(mpd_state->conn, uint_buf1)) {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Set playing song failed\"}");
                    LOG_ERROR("Error mpd_run_play_id()");
                }
            }
            break;
        case MPD_API_PLAYER_OUTPUT_LIST:
            data = mpd_client_put_outputs(mpd_state, data);
            break;
        case MPD_API_PLAYER_TOGGLE_OUTPUT:
            je = json_scanf(request->data, sdslen(request->data), "{data: {output: %u, state: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (uint_buf2) {
                    if (mpd_run_enable_output(mpd_state->conn, uint_buf1))
                        data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                    else {
                        data = sdscat(data, "{\"type\": \"error\", \"data\": \"Enabling output failed\"}");
                        LOG_ERROR("Error mpd_run_enable_output()");
                    }
                }
                else {
                    if (mpd_run_disable_output(mpd_state->conn, uint_buf1))
                        data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                    else {
                        data = sdscat(data, "{\"type\": \"error\", \"data\": \"Disabling output failed\"}");
                        LOG_ERROR("Error mpd_run_disable_output()");
                    }
                }
            }
            break;
        case MPD_API_PLAYER_VOLUME_SET:
            je = json_scanf(request->data, sdslen(request->data), "{data: {volume:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_set_volume(mpd_state->conn, uint_buf1))
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Setting volume failed\"}");
                    LOG_ERROR("MPD_API_PLAYER_PLAY_TRACK: Error mpd_run_set_volume()");
                }
            }
            break;
        case MPD_API_PLAYER_VOLUME_GET:
            data = mpd_client_put_volume(mpd_state, data);
            break;            
        case MPD_API_PLAYER_SEEK:
            je = json_scanf(request->data, sdslen(request->data), "{data: {songid: %u, seek: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (mpd_run_seek_id(mpd_state->conn, uint_buf1, uint_buf2))
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Seeking song failed\"}");
                    LOG_ERROR("Error mpd_run_seek_id()");
                }
            }
            break;
        case MPD_API_QUEUE_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{data: {offset: %u, cols: %M}}", &uint_buf1, json_to_tags, tagcols);
            if (je == 2) {
                data = mpd_client_put_queue(mpd_state, data, uint_buf1, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_LAST_PLAYED: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{data: {offset: %u, cols: %M}}", &uint_buf1, json_to_tags, tagcols);
            if (je == 2) {
                data = mpd_client_put_last_played_songs(config, mpd_state, data, uint_buf1, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYER_CURRENT_SONG: {
            data = mpd_client_put_current_song(config, mpd_state, data);
            break;
        }
        case MPD_API_DATABASE_SONGDETAILS:
            je = json_scanf(request->data, sdslen(request->data), "{data: { uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                data = mpd_client_put_songdetails(config, mpd_state, data, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_FINGERPRINT:
            je = json_scanf(request->data, sdslen(request->data), "{data: { uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                data = mpd_client_put_fingerprint(mpd_state, data, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_TAG_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{data: {offset: %u, filter: %Q, tag: %Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                data = mpd_client_put_db_tag(mpd_state, data, uint_buf1, p_charbuf2, "", "", p_charbuf1);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{data: {offset: %u, filter: %Q, search: %Q, tag: %Q}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 4) {
                data = mpd_client_put_db_tag(mpd_state, data, uint_buf1, "Album", p_charbuf3, p_charbuf2, p_charbuf1);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{data: {album: %Q, search: %Q, tag: %Q, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &p_charbuf3, json_to_tags, tagcols);
            if (je == 4) {
                data = mpd_client_put_songs_in_album(config, mpd_state, data, p_charbuf1, p_charbuf2, p_charbuf3, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYLIST_RENAME:
            je = json_scanf(request->data, sdslen(request->data), "{data: {from: %Q, to: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                data = mpd_client_rename_playlist(config, mpd_state, data, p_charbuf1, p_charbuf2);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            break;            
        case MPD_API_PLAYLIST_LIST:
            je = json_scanf(request->data, sdslen(request->data), "{data: {offset: %u, filter: %Q}}", &uint_buf1, &p_charbuf1);
            if (je == 2) {
                data = mpd_client_put_playlists(config, mpd_state, data, uint_buf1, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_CONTENT_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{data: {uri: %Q, offset:%u, filter:%Q, cols: %M}}", 
                &p_charbuf1, &uint_buf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                data = mpd_client_put_playlist_list(config, mpd_state, data, p_charbuf1, uint_buf1, p_charbuf2, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYLIST_ADD_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{data: {plist:%Q, uri:%Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                if (mpd_run_playlist_add(mpd_state->conn, p_charbuf1, p_charbuf2)) {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"Added %%{uri} to playlist %%{playlist}\", "
                                        "\"values\": {");
                    data = tojson_char(data, "uri", p_charbuf2, true);
                    data = tojosn_char(data, "playlist", p_charbuf1, false);
                    data = sdscat(data, "}}");
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Adding song to playlist failed\"}");
                    LOG_ERROR("Error mpd_run_playlist_add");
                }
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);                
            }
            break;
        case MPD_API_PLAYLIST_CLEAR:
            je = json_scanf(request->data, sdslen(request->data), "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_playlist_clear(mpd_state->conn, p_charbuf1)) {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Clearing playlist failed\"}");
                    LOG_ERROR("Error mpd_run_playlist_clear");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_RM_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{data: {uri:%Q, track:%u}}", &p_charbuf1, &uint_buf1);
            if (je == 2) {
                if (mpd_run_playlist_delete(mpd_state->conn, p_charbuf1, uint_buf1)) {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Removing track from playlist failed\"}");
                    LOG_ERROR("Error mpd_run_playlist_delete");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_FILESYSTEM_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{data: {offset:%u, filter:%Q, path:%Q, cols: %M}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                data = mpd_client_put_browse(config, mpd_state, data, p_charbuf2, uint_buf1, p_charbuf1, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_ADD_TRACK_AFTER:
            je = json_scanf(request->data, sdslen(request->data), "{data: {uri:%Q, to:%d}}", &p_charbuf1, &int_buf1);
            if (je == 2) {
                int_rc = mpd_run_add_id_to(mpd_state->conn, p_charbuf1, int_buf1);
                if (int_rc > -1 ) 
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add_id_to()");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_REPLACE_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{data: {uri:%Q }}", &p_charbuf1);
            if (je == 1) {
                if (!mpd_run_clear(mpd_state->conn)) {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Clearing queue failed\"}");
                    LOG_ERROR("Error mpd_run_clear");
                }
                else if (!mpd_run_add(mpd_state->conn, p_charbuf1)) {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add");
                }
                else if (!mpd_run_play(mpd_state->conn)) {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Can not start playing\"}");
                    LOG_ERROR("Error mpd_run_play");
                }
                else
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_add(mpd_state->conn, p_charbuf1))
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_PLAY_TRACK:
            je = json_scanf(request->data, sdslen(request->data), "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                int_buf1 = mpd_run_add_id(mpd_state->conn, p_charbuf1);
                if (int_buf1 != -1) {
                    if (mpd_run_play_id(mpd_state->conn, int_buf1)) {
                        data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                    }
                    else {
                        data = sdscat(data, "{\"type\": \"error\", \"data\": \"Can not start playing\"}");
                        LOG_ERROR("Error mpd_run_play_id()");
                    }
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add_id");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_REPLACE_PLAYLIST:
            je = json_scanf(request->data, sdslen(request->data), "{data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (!mpd_run_clear(mpd_state->conn)) {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Clearing queue failed\"}");
                    LOG_ERROR("Error mpd_run_clear");                
                }
                else if (!mpd_run_load(mpd_state->conn, p_charbuf1)) {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Adding playlist to queue failed\"}");
                    LOG_ERROR("Error mpd_run_load");
                }
                else if (!mpd_run_play(mpd_state->conn)) {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Can not start playing\"}");
                    LOG_ERROR("Error mpd_run_play");
                }
                else {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_RANDOM:
            je = json_scanf(request->data, sdslen(request->data), "{data: {mode:%u, playlist:%Q, quantity:%d}}", &uint_buf1, &p_charbuf1, &int_buf1);
            if (je == 3) {
                rc = mpd_client_jukebox_add(mpd_state, int_buf1, uint_buf1, p_charbuf1);
                FREE_PTR(p_charbuf1);
                if (rc == true) {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"Sucessfully added random songs to queue\"}");
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Adding random songs to queue failed\"}");
                }
            }
            break;
        case MPD_API_QUEUE_ADD_PLAYLIST:
            je = json_scanf(request->data, sdslen(request->data), "{data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_load(mpd_state->conn, p_charbuf1)) {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Adding playlist to queue failed\"}");
                    LOG_ERROR("Error mpd_run_load");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_SAVE:
            je = json_scanf(request->data, sdslen(request->data), "{ data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_save(mpd_state->conn, p_charbuf1)) {
                    data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    data = sdscat(data, "{\"type\": \"error\", \"data\": \"Saving queue as playlist failed\"}");
                    LOG_ERROR("Error mpd_run_save");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_SEARCH: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{data: {offset:%u, filter:%Q, searchstr:%Q, cols: %M}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                data = mpd_client_search_queue(mpd_state, data, p_charbuf1, uint_buf1, p_charbuf2, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_DATABASE_SEARCH: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, sdslen(request->data), "{data: {searchstr:%Q, filter:%Q, plist:%Q, offset:%u, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &p_charbuf3, &uint_buf1, json_to_tags, tagcols);
            if (je == 5) {
                data = mpd_client_search(mpd_state, data, p_charbuf1, p_charbuf2, p_charbuf3, uint_buf1, tagcols);
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
            je = json_scanf(request->data, sdslen(request->data), "{data: {expression:%Q, sort:%Q, sortdesc:%B, plist:%Q, offset:%u, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &bool_buf, &p_charbuf3, &uint_buf1, json_to_tags, tagcols);
            if (je == 6) {
                data = mpd_client_search_adv(mpd_state, data, p_charbuf1, p_charbuf2, bool_buf, NULL, p_charbuf3, uint_buf1, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_SHUFFLE:
            if (mpd_run_shuffle(mpd_state->conn))
                data = sdscat(data, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                data = sdscat(data, "{\"type\": \"error\", \"data\": \"Shuffling queue failed\"}");
                LOG_ERROR("Error mpd_run_shuffle");
            }
            break;
        case MPD_API_PLAYLIST_RM:
            je = json_scanf(request->data, sdslen(request->data), "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                data = mpd_client_playlist_delete(config, mpd_state, data, p_charbuf1)
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_SETTINGS_GET:
            data = mpd_client_put_settings(mpd_state, data);
            break;
        case MPD_API_DATABASE_STATS:
            data = mpd_client_put_stats(mpd_state, data);
            break;
        default:
            data = sdscat(data, "{\"type\": \"error\", \"data\": \"Unknown request\"}");
            LOG_ERROR("Unknown API request: %.*s", sdslen(request->data), request->data);
    }

    if (mpd_state->conn_state == MPD_CONNECTED && mpd_connection_get_error(mpd_state->conn) != MPD_ERROR_SUCCESS) {
        LOG_ERROR("MPD error: %s", mpd_connection_get_error_message(mpd_state->conn));
        data = sdscat(sdsempty(), "{\"type\":\"error\", \"data\": \"%s\"}", mpd_connection_get_error_message(mpd_state->conn));
        /* Try to recover error */
        if (!mpd_connection_clear_error(mpd_state->conn))
            mpd_state->conn_state = MPD_FAILURE;
    }

    if (sdslen(data) == 0) {
        data = sdscat(data, "{\"type\": \"error\", \"data\": \"No response for cmd_id %%{cmdId}\", \"values\": {\"cmdId\": %d}}", request->cmd_id);
    }
    if (response->conn_id > -1) {
        LOG_DEBUG("Push response to queue for connection %lu: %s", request->conn_id, data);
        reponse->data = data;
        tiny_queue_push(web_server_queue, response);
    }
    else {
        sds_free(data);
        FREE_PTR(response);
    }
    sds_free(request->data);
    FREE_PTR(request);
}

static bool mpd_api_settings_set(t_config *config, t_mpd_state *mpd_state, struct json_token *key, struct json_token *val, bool *mpd_host_changed) {
    bool rc = true;
    char *crap;
    sds settingvalue = sdscatlen(sdsempty(), val->ptr, val->len);
    if (strncmp(key->ptr, "mpdPass", key->len) == 0) {
        if (strcmp(key, "dontsetpassword") != 0) {
            mpd_host_changed = true;
            mpd_state->mpd_pass = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        }
        else {
            sds_free(settingvalue);
            return true;
        }
    }
    else if (strncmp(key->ptr, "mpdHost", key->len) == 0) {
        if (strncmp(val->ptr, mpd_state->mpd_host, val->len) != 0) {
            mpd_host_changed = true;
            mpd_state->mpd_host = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        }
    }
    else if (strncmp(key->ptr, "mpdPort", key->len) == 0) {
        int mpd_port = strtoimax(settingvalue, &crap, 10);
        if (mpd_state->mpd_port != mpd_port) {
            mpd_host_changed = true;
            mpd_state->mpd_port = mpd_port;
        }
    }
    else if (strncmp(key->ptr, "musicDirectory", key->len) == 0) {
        mpd_state->mpd_host = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "jukeboxMode", key->len) == 0) {
        int jukebox_mode = strtoimax(settingvalue, &crap, 10);
        if (jukebox_mode < 0 || jukebox_mode > 2) {
            sds_free(settingvalue);
            return false;
        }
        mpd_state->jukebox_mode = jukebox_mode;
    }
    else if (strncmp(key->ptr, "jukeboxPlaylist", key->len) == 0) {
        mpd_state->jukebox_playlist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "jukeboxQueueLength", key->len) == 0) {
        int jukebox_queue_length = strtoimax(settingvalue, &crap, 10);
        if (jukebox_queue_length <= 0 || jukebox_queue_length > 999) {
            sds_free(settingvalue);
            return false;
        }
        mpd_state->jukebox_queue_length = jukebox_queue_length;
    }
    else if (strncmp(key->ptr, "autoPlay", key->len) == 0) {
        mpd_state->auto_play = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "coverimage", key->len) == 0) {
        mympd_state->coverimage = val->type == JSON_TYPE_TRUE ? true : false;
        settingname = sdscat(sdsempty(), "coverimage");
    }
    else if (strncmp(key->ptr, "coverimageName", key->len) == 0) {
        if (validate_string(settingvalue) && sdslen(settingvalue) > 0) {
            mpd_state->coverimage_name = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
        }
        else {
            sds_free(settingvalue);
            return false;
        }
    }
    else if (strncmp(key->ptr, "love", key->len) == 0) {
        mpd_state->love = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strcmp(key->ptr, "loveChannel", key->len) == 0) {
        mpd_state->love_channel = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "loveMessage", key->len) == 0) {
        mpd_state->love_message = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "taglist", key->len) == 0) {
        mpd_state->taglist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "searchtaglist", key->len) == 0) {
        mpd_state->searchtaglist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "browsetaglist", key->len) == 0) {
        mpd_state->browsetaglist = sdscatlen(sdsempty(), settingvalue, sdslen(settingvalue));
    }
    else if (strncmp(key->ptr, "stickers", key->len) == 0) {
        mpd_state->stickers = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "smartpls", key->len) == 0) {
        mpd_state->smartpls = val->type == JSON_TYPE_TRUE ? true : false;
    }
    else if (strncmp(key->ptr, "maxElementsPerPage", key->len) == 0) {
        int max_elements_per_page = strtoimax(settingvalue, &crap, 10);
        if (max_elements_per_page <= 0 || max_elements_per_page > 999) {
            sds_free(settingvalue);
            return false;
        }
        mpd_state->max_elements_per_page = max_elements_per_page;
    }
    else if (strncmp(key->ptr, "lastPlayedCount", key->len) == 0) {
        int last_played_count = strtoimax(settingvalue, &crap, 10);
        if (last_played_count <= 0) {
            sds_free(settingvalue);
            return false;
        }
        mpd_state->last_played_count = last_played_count;
    }
    else if (strncmp(key->ptr, "random", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_random(mpd_state->conn, uint_buf));
    }
    else if (strncmp(key->ptr, "repeat", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_repeat(mpd_state->conn, uint_buf));
    }
    else if (strncmp(key->ptr, "consume", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_consume(mpd_state->conn, uint_buf));
    }
    else if (strncmp(key->ptr, "single", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_single(mpd_state->conn, uint_buf));
    }
    else if (strncmp(key->ptr, "crossfade", key->len) == 0) {
        unsigned uint_buf = strtoumax(settingvalue, &crap, 10);
        rc = mpd_run_crossfade(mpd_state->conn, uint_buf));
    }
    else if (strncmp(key->ptr, "mixrampdb", key->len) == 0) {
        if (config->mixramp == true) {
            float float_buf = strtof(settingvalue, &crap, 10);
            rc = mpd_run_mixrampdb(mpd_state->conn, float_buf));
        }
    }
    else if (strncmp(key->ptr, "mixrampdelay", key->len) == 0) {
        if (config->mixramp == true) {
            float float_buf = strtof(settingvalue, &crap, 10);
            rc = mpd_run_mixrampdelay(mpd_state->conn, float_buf));
        }
    }
    else if (strncmp(key->ptr, "replaygain", key->len) == 0) {
        rc = mpd_send_command(mpd_state->conn, "replay_gain_mode", settingvalue, NULL));
    }    
    else {
        LOG_ERROR("Setting with name \"%s\" not supported", settingname);
        sds_free(settingvalue);
        return false;
    }

    sds_free(settingvalue);
    return rc;
}

static int mpd_client_put_fingerprint(t_mpd_state *mpd_state, char *buffer, const char *uri) {
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

static int mpd_client_put_songdetails(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri) {
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

//private functions
static int mpd_client_put_browse(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *path, const unsigned int offset, const char *filter, const t_tags *tagcols) {
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

static int mpd_client_put_db_tag(t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter) {
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

static int mpd_client_put_songs_in_album(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *album, const char *search, const char *tag, const t_tags *tagcols) {
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
