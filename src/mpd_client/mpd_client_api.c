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
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <poll.h>
#include <dirent.h>
#include <pthread.h>
#include <mpd/client.h>
#include <inttypes.h>

#include "../dist/src/sds/sds.h"
#include "utility.h"
#include "api.h"
#include "log.h"
#include "list.h"
#include "config_defs.h"
#include "tiny_queue.h"
#include "mpd_client_api.h"
#include "../dist/src/frozen/frozen.h"
#include "../dist/src/sds/sds.h"

//private definitions
static int mpd_client_get_updatedb_state(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_get_state(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_state(t_mpd_state *mpd_state, struct mpd_status *status, char *buffer);
static int mpd_client_put_outputs(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_current_song(t_config *config, t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_browse(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *path, const unsigned int offset, const char *filter, const t_tags *tagcols);
static int mpd_client_put_volume(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_settings(t_mpd_state *mpd_state, char *buffer);
static int mpd_client_put_db_tag(t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter);
static int mpd_client_put_songs_in_album(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *album, const char *search, const char *tag, const t_tags *tagcols);
static int mpd_client_put_songdetails(t_config *config, t_mpd_state *mpd_state, char *buffer, const char *uri);
static int mpd_client_put_fingerprint(t_mpd_state *mpd_state, char *buffer, const char *uri);

//private functions
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

    LOG_VERBOSE("API request (%d): %.*s", request->conn_id, request->length, request->data);
    //create response struct
    t_work_result *response = (t_work_result*)malloc(sizeof(t_work_result));
    assert(response);
    response->conn_id = request->conn_id;
    response->length = 0;
    
    switch(request->cmd_id) {
        case MPD_API_LOVE:
            if (mpd_run_send_message(mpd_state->conn, mpd_state->love_channel, mpd_state->love_message)) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Scrobbled love\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to send love message to channel\"}");
            }
        break;
        case MPD_API_LIKE:
            if (mpd_state->feat_sticker) {
                je = json_scanf(request->data, request->length, "{data: {uri: %Q, like: %d}}", &p_charbuf1, &uint_buf1);
                if (je == 2) {        
                    if (!mpd_client_like_song_uri(mpd_state, p_charbuf1, uint_buf1)) {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set like\"}");
                    }
                    else {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    }
                    FREE_PTR(p_charbuf1);
                }
            } 
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"MPD stickers are disabled\"}");
                LOG_ERROR("MPD stickers are disabled");
            }
            break;
        case MPD_API_PLAYER_STATE:
            response->length = mpd_client_get_state(mpd_state, response->data);
            break;
        case MYMPD_API_SETTINGS_SET:
            //only update mpd_state, already saved and sanitized in mympd_api.c
            je = json_scanf(request->data, request->length, "{data: {jukeboxMode: %d}}", &int_buf1);
            if (je == 1) {
                mpd_state->jukebox_mode = int_buf1;
            }
            je = json_scanf(request->data, request->length, "{data: {jukeboxPlaylist: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->jukebox_playlist, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {jukeboxQueueLength: %d}}", &int_buf1);
            if (je == 1) {
                mpd_state->jukebox_queue_length = int_buf1;
            }
            je = json_scanf(request->data, request->length, "{data: {autoPlay: %B}}", &bool_buf);
            if (je == 1) {
                mpd_state->auto_play = bool_buf;
            }
            je = json_scanf(request->data, request->length, "{data: {coverimage: %B}}", &bool_buf);
            if (je == 1) {
                mpd_state->coverimage = bool_buf;
            }
            je = json_scanf(request->data, request->length, "{data: {coverimageName: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->coverimage_name, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {loveChannel: %Q}}", &p_charbuf1);
            if (je == 1)
                REASSIGN_PTR(mpd_state->love_channel, p_charbuf1);
            je = json_scanf(request->data, request->length, "{data: {loveMessage: %Q}}", &p_charbuf1);
            if (je == 1)
                REASSIGN_PTR(mpd_state->love_message, p_charbuf1);
            je = json_scanf(request->data, request->length, "{data: {love: %B}}", &bool_buf);
            if (je == 1) {
                mpd_state->love = bool_buf;
            }
            je = json_scanf(request->data, request->length, "{data: {taglist: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->taglist, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {searchtaglist: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->searchtaglist, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {browsetaglist: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->browsetaglist, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {stickers: %B}}", &bool_buf);
            if (je == 1) {
                mpd_state->stickers = bool_buf;
            }
            je = json_scanf(request->data, request->length, "{data: {smartpls: %B}}", &bool_buf);
            if (je == 1) {
                mpd_state->smartpls = bool_buf;
            }
            je = json_scanf(request->data, request->length, "{data: {musicDirectory: %Q}}", &p_charbuf1);
            if (je == 1) {
                REASSIGN_PTR(mpd_state->music_directory, p_charbuf1);
            }
            je = json_scanf(request->data, request->length, "{data: {mpdHost: %Q, mpdPort: %d, mpdPass: %Q}}", &p_charbuf1, &int_buf1, &p_charbuf2);
            if (je == 3) {
                if (mpd_state->mpd_host == NULL || mpd_state->mpd_pass == NULL) {
                    REASSIGN_PTR(mpd_state->mpd_host, p_charbuf1);
                    REASSIGN_PTR(mpd_state->mpd_pass, p_charbuf2);
                    mpd_state->mpd_port = int_buf1;
                }
                else if (strcmp(p_charbuf1, mpd_state->mpd_host) != 0 || (strcmp(p_charbuf2, "dontsetpassword") != 0 && strcmp(mpd_state->mpd_pass, p_charbuf2) != 0) || mpd_state->mpd_port != int_buf1) {
                    REASSIGN_PTR(mpd_state->mpd_host, p_charbuf1);
                    if (strcmp(p_charbuf2, "dontsetpassword") != 0) {
                        REASSIGN_PTR(mpd_state->mpd_pass, p_charbuf2);
                    }
                    else {
                        FREE_PTR(p_charbuf2);
                    }
                    mpd_state->mpd_port = int_buf1;
                    mpd_state->conn_state = MPD_DISCONNECT;
                }
                else {
                    FREE_PTR(p_charbuf1);
                    FREE_PTR(p_charbuf2);
                }
            }
            
            je = json_scanf(request->data, request->length, "{data: {lastPlayedCount: %d}}", &uint_buf1);
            if (je == 1) {
                mpd_state->last_played_count = uint_buf1;
            }
            je = json_scanf(request->data, request->length, "{data: {maxElementsPerPage: %d}}", &uint_buf1);
            if (je == 1) {
                mpd_state->max_elements_per_page = uint_buf1;
            }
            //feature detection
            if (mpd_state->conn_state == MPD_CONNECTED) {
                mpd_client_feature_love(mpd_state);
                mpd_client_feature_tags(mpd_state);
                mpd_client_feature_music_directory(mpd_state);
            }
            //set mpd options
            je = json_scanf(request->data, request->length, "{data: {random: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_random(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"random\"}}");
            }
            je = json_scanf(request->data, request->length, "{data: {repeat: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_repeat(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"repeat\"}}");
            }
            je = json_scanf(request->data, request->length, "{data: {consume: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_consume(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"consume\"}}");
            }
            je = json_scanf(request->data, request->length, "{data: {single: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_single(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"single\"}}");
            }
            je = json_scanf(request->data, request->length, "{data: {crossfade: %u}}", &uint_buf1);
            if (je == 1) {
                if (!mpd_run_crossfade(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"crossfade\"}}");
            }
            if (config->mixramp == true) {
                je = json_scanf(request->data, request->length, "{data: {mixrampdb: %f}}", &float_buf);
                if (je == 1) {
                    if (!mpd_run_mixrampdb(mpd_state->conn, float_buf))
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"mixrampdb\"}}");
                }
                je = json_scanf(request->data, request->length, "{data: {mixrampdelay: %f}}", &float_buf);
                if (je == 1) {
                    if (!mpd_run_mixrampdelay(mpd_state->conn, float_buf))
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"mixrampdelay\"}}");
                }
            }
            
            je = json_scanf(request->data, request->length, "{data: {replaygain: %Q}}", &p_charbuf1);
            if (je == 1) {
                if (!mpd_send_command(mpd_state->conn, "replay_gain_mode", p_charbuf1, NULL)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to set mpd state %%{state}\", \"values\": {\"state\": \"replaygain\"}}");
                }
                mpd_response_finish(mpd_state->conn);
                FREE_PTR(p_charbuf1);            
            }
            if (mpd_state->jukebox_mode != JUKEBOX_OFF && mpd_state->conn_state == MPD_CONNECTED) {
                mpd_client_jukebox(mpd_state);
            }
            if (response->length == 0) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_DATABASE_UPDATE:
            uint_rc = mpd_run_update(mpd_state->conn, NULL);
            if (uint_rc > 0)
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_DATABASE_RESCAN:
            uint_rc = mpd_run_rescan(mpd_state->conn, NULL);
            if (uint_rc > 0)
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_SMARTPLS_UPDATE_ALL:
            rc = mpd_client_smartpls_update_all(config, mpd_state);
            if (rc == true) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Smart playlists updated\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Smart playlists update failed\"}");
            }
            break;
        case MPD_API_SMARTPLS_UPDATE:
            je = json_scanf(request->data, request->length, "{data: {playlist: %Q}}", &p_charbuf1);
            if (je == 1) {
                rc = mpd_client_smartpls_update(config, mpd_state, p_charbuf1);
                if (rc == true) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Smart playlist %%{playlist} updated\", \"values\": {\"playlist\": \"%s\"}}", p_charbuf1);
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Smart playlist update failed\"}");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_SMARTPLS_SAVE:
            je = json_scanf(request->data, request->length, "{data: {type: %Q}}", &p_charbuf1);
            response->length = 0;
            rc = false;
            if (je == 1) {
                if (strcmp(p_charbuf1, "sticker") == 0) {
                    je = json_scanf(request->data, request->length, "{data: {playlist: %Q, sticker: %Q, maxentries: %d}}", &p_charbuf2, &p_charbuf3, &int_buf1);
                    if (je == 3) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, p_charbuf3, NULL, int_buf1, 0);
                        FREE_PTR(p_charbuf2);
                        FREE_PTR(p_charbuf3);
                    }
                }
                else if (strcmp(p_charbuf1, "newest") == 0) {
                    je = json_scanf(request->data, request->length, "{data: {playlist: %Q, timerange: %d}}", &p_charbuf2, &int_buf1);
                    if (je == 2) {
                        rc = mpd_client_smartpls_save(config, mpd_state, p_charbuf1, p_charbuf2, NULL, NULL, 0, int_buf1);
                        FREE_PTR(p_charbuf2);
                    }
                }            
                else if (strcmp(p_charbuf1, "search") == 0) {
                    je = json_scanf(request->data, request->length, "{data: {playlist: %Q, tag: %Q, searchstr: %Q}}", &p_charbuf2, &p_charbuf3, &p_charbuf4);
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
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Failed to save playlist\"}");
            }
            break;
        case MPD_API_SMARTPLS_GET:
            je = json_scanf(request->data, request->length, "{data: {playlist: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->length = mpd_client_smartpls_put(config, response->data, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PAUSE:
            if (mpd_run_toggle_pause(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Toggling player pause failed.\"}");
                LOG_ERROR("Error mpd_run_toggle_pause()");
            }
            break;
        case MPD_API_PLAYER_PREV:
            if (mpd_run_previous(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Goto previous song failed\"}");
                LOG_ERROR("Error mpd_run_previous()");
            }
            break;
        case MPD_API_PLAYER_NEXT:
            if (mpd_run_next(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Skip to next song failed\"}");
                LOG_ERROR("Error mpd_run_next()");
            }
            break;
        case MPD_API_PLAYER_PLAY:
            if (mpd_run_play(mpd_state->conn)) {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Begin to play failed\"}");
                LOG_ERROR("Error mpd_run_play()");
            }
            break;
        case MPD_API_PLAYER_STOP:
            if (mpd_run_stop(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Stop playing failed\"}");
                LOG_ERROR("Error mpd_run_stop()");
            }
            break;
        case MPD_API_QUEUE_CLEAR:
            if (mpd_run_clear(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing queue failed\"}");
                LOG_ERROR("Error mpd_run_clear()");
            }
            break;
        case MPD_API_QUEUE_CROP:
            response->length = mpd_client_queue_crop(mpd_state, response->data);
            break;
        case MPD_API_QUEUE_RM_TRACK:
            je = json_scanf(request->data, request->length, "{data: {track:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_delete_id(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Removing track from queue failed\"}");
                    LOG_ERROR("Error mpd_run_delete_id()");
                }
            }
            break;
        case MPD_API_QUEUE_RM_RANGE:
            je = json_scanf(request->data, request->length, "{data: {start: %u, end: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (mpd_run_delete_range(mpd_state->conn, uint_buf1, uint_buf2))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Removing track range from queue failed\"}");
                    LOG_ERROR("Error mpd_run_delete_range()");
                }
            }
            break;
        case MPD_API_QUEUE_MOVE_TRACK:
            je = json_scanf(request->data, request->length, "{data: {from: %u, to: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2)
                    uint_buf2--;
                if (mpd_run_move(mpd_state->conn, uint_buf1, uint_buf2))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Moving track in queue failed\"}");
                    LOG_ERROR("Error mpd_run_move()");
                }
            }
            break;
        case MPD_API_PLAYLIST_MOVE_TRACK:
            je = json_scanf(request->data, request->length, "{data: {plist: %Q, from: %u, to: %u }}", &p_charbuf1, &uint_buf1, &uint_buf2);
            if (je == 3) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2)
                    uint_buf2--;
                if (mpd_send_playlist_move(mpd_state->conn, p_charbuf1, uint_buf1, uint_buf2)) {
                    mpd_response_finish(mpd_state->conn);
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Moving track in playlist failed\"}");
                    LOG_ERROR("Error mpd_send_playlist_move()");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PLAY_TRACK:
            je = json_scanf(request->data, request->length, "{data: { track:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_play_id(mpd_state->conn, uint_buf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Set playing song failed\"}");
                    LOG_ERROR("Error mpd_run_play_id()");
                }
            }
            break;
        case MPD_API_PLAYER_OUTPUT_LIST:
            response->length = mpd_client_put_outputs(mpd_state, response->data);
            break;
        case MPD_API_PLAYER_TOGGLE_OUTPUT:
            je = json_scanf(request->data, request->length, "{data: {output: %u, state: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (uint_buf2) {
                    if (mpd_run_enable_output(mpd_state->conn, uint_buf1))
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    else {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Enabling output failed\"}");
                        LOG_ERROR("Error mpd_run_enable_output()");
                    }
                }
                else {
                    if (mpd_run_disable_output(mpd_state->conn, uint_buf1))
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    else {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Disabling output failed\"}");
                        LOG_ERROR("Error mpd_run_disable_output()");
                    }
                }
            }
            break;
        case MPD_API_PLAYER_VOLUME_SET:
            je = json_scanf(request->data, request->length, "{data: {volume:%u}}", &uint_buf1);
            if (je == 1) {
                if (mpd_run_set_volume(mpd_state->conn, uint_buf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Setting volume failed\"}");
                    LOG_ERROR("MPD_API_PLAYER_PLAY_TRACK: Error mpd_run_set_volume()");
                }
            }
            break;
        case MPD_API_PLAYER_VOLUME_GET:
            response->length = mpd_client_put_volume(mpd_state, response->data);
            break;            
        case MPD_API_PLAYER_SEEK:
            je = json_scanf(request->data, request->length, "{data: {songid: %u, seek: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (mpd_run_seek_id(mpd_state->conn, uint_buf1, uint_buf2))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Seeking song failed\"}");
                    LOG_ERROR("Error mpd_run_seek_id()");
                }
            }
            break;
        case MPD_API_QUEUE_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {offset: %u, cols: %M}}", &uint_buf1, json_to_tags, tagcols);
            if (je == 2) {
                response->length = mpd_client_put_queue(mpd_state, response->data, uint_buf1, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_LAST_PLAYED: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {offset: %u, cols: %M}}", &uint_buf1, json_to_tags, tagcols);
            if (je == 2) {
                response->length = mpd_client_put_last_played_songs(config, mpd_state, response->data, uint_buf1, tagcols);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYER_CURRENT_SONG: {
            response->length = mpd_client_put_current_song(config, mpd_state, response->data);
            break;
        }
        case MPD_API_DATABASE_SONGDETAILS:
            je = json_scanf(request->data, request->length, "{data: { uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->length = mpd_client_put_songdetails(config, mpd_state, response->data, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_FINGERPRINT:
            je = json_scanf(request->data, request->length, "{data: { uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                response->length = mpd_client_put_fingerprint(mpd_state, response->data, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_TAG_LIST:
            je = json_scanf(request->data, request->length, "{data: {offset: %u, filter: %Q, tag: %Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                response->length = mpd_client_put_db_tag(mpd_state, response->data, uint_buf1, p_charbuf2, "", "", p_charbuf1);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_LIST:
            je = json_scanf(request->data, request->length, "{data: {offset: %u, filter: %Q, search: %Q, tag: %Q}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 4) {
                response->length = mpd_client_put_db_tag(mpd_state, response->data, uint_buf1, "Album", p_charbuf3, p_charbuf2, p_charbuf1);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {album: %Q, search: %Q, tag: %Q, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &p_charbuf3, json_to_tags, tagcols);
            if (je == 4) {
                response->length = mpd_client_put_songs_in_album(config, mpd_state, response->data, p_charbuf1, p_charbuf2, p_charbuf3, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYLIST_RENAME:
            je = json_scanf(request->data, request->length, "{data: {from: %Q, to: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                response->length = mpd_client_rename_playlist(config, mpd_state, response->data, p_charbuf1, p_charbuf2);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            break;            
        case MPD_API_PLAYLIST_LIST:
            je = json_scanf(request->data, request->length, "{data: {offset: %u, filter: %Q}}", &uint_buf1, &p_charbuf1);
            if (je == 2) {
                response->length = mpd_client_put_playlists(config, mpd_state, response->data, uint_buf1, p_charbuf1);
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_CONTENT_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {uri: %Q, offset:%u, filter:%Q, cols: %M}}", 
                &p_charbuf1, &uint_buf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                response->length = mpd_client_put_playlist_list(config, mpd_state, response->data, p_charbuf1, uint_buf1, p_charbuf2, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_PLAYLIST_ADD_TRACK:
            je = json_scanf(request->data, request->length, "{data: {plist:%Q, uri:%Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                if (mpd_run_playlist_add(mpd_state->conn, p_charbuf1, p_charbuf2)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Added %%{uri} to playlist %%{playlist}\", "
                        "\"values\": {\"uri\": \"%s\", \"playlist\": \"%s\"}}", p_charbuf2, p_charbuf1);
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding song to playlist failed\"}");
                    LOG_ERROR("Error mpd_run_playlist_add");
                }
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);                
            }
            break;
        case MPD_API_PLAYLIST_CLEAR:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_playlist_clear(mpd_state->conn, p_charbuf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing playlist failed\"}");
                    LOG_ERROR("Error mpd_run_playlist_clear");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_RM_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q, track:%u}}", &p_charbuf1, &uint_buf1);
            if (je == 2) {
                if (mpd_run_playlist_delete(mpd_state->conn, p_charbuf1, uint_buf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Removing track from playlist failed\"}");
                    LOG_ERROR("Error mpd_run_playlist_delete");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_DATABASE_FILESYSTEM_LIST: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {offset:%u, filter:%Q, path:%Q, cols: %M}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                response->length = mpd_client_put_browse(config, mpd_state, response->data, p_charbuf2, uint_buf1, p_charbuf1, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_ADD_TRACK_AFTER:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q, to:%d}}", &p_charbuf1, &int_buf1);
            if (je == 2) {
                int_rc = mpd_run_add_id_to(mpd_state->conn, p_charbuf1, int_buf1);
                if (int_rc > -1 ) 
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add_id_to()");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_REPLACE_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q }}", &p_charbuf1);
            if (je == 1) {
                if (!mpd_run_clear(mpd_state->conn)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing queue failed\"}");
                    LOG_ERROR("Error mpd_run_clear");
                }
                else if (!mpd_run_add(mpd_state->conn, p_charbuf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add");
                }
                else if (!mpd_run_play(mpd_state->conn)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can not start playing\"}");
                    LOG_ERROR("Error mpd_run_play");
                }
                else
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_add(mpd_state->conn, p_charbuf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_PLAY_TRACK:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                int_buf1 = mpd_run_add_id(mpd_state->conn, p_charbuf1);
                if (int_buf1 != -1) {
                    if (mpd_run_play_id(mpd_state->conn, int_buf1)) {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    }
                    else {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can not start playing\"}");
                        LOG_ERROR("Error mpd_run_play_id()");
                    }
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding song to queue failed\"}");
                    LOG_ERROR("Error mpd_run_add_id");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_REPLACE_PLAYLIST:
            je = json_scanf(request->data, request->length, "{data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (!mpd_run_clear(mpd_state->conn)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Clearing queue failed\"}");
                    LOG_ERROR("Error mpd_run_clear");                
                }
                else if (!mpd_run_load(mpd_state->conn, p_charbuf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding playlist to queue failed\"}");
                    LOG_ERROR("Error mpd_run_load");
                }
                else if (!mpd_run_play(mpd_state->conn)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can not start playing\"}");
                    LOG_ERROR("Error mpd_run_play");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_ADD_RANDOM:
            je = json_scanf(request->data, request->length, "{data: {mode:%u, playlist:%Q, quantity:%d}}", &uint_buf1, &p_charbuf1, &int_buf1);
            if (je == 3) {
                rc = mpd_client_jukebox_add(mpd_state, int_buf1, uint_buf1, p_charbuf1);
                FREE_PTR(p_charbuf1);
                if (rc == true) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Sucessfully added random songs to queue\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding random songs to queue failed\"}");
                }
            }
            break;
        case MPD_API_QUEUE_ADD_PLAYLIST:
            je = json_scanf(request->data, request->length, "{data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_load(mpd_state->conn, p_charbuf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Adding playlist to queue failed\"}");
                    LOG_ERROR("Error mpd_run_load");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_SAVE:
            je = json_scanf(request->data, request->length, "{ data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                if (mpd_run_save(mpd_state->conn, p_charbuf1))
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Saving queue as playlist failed\"}");
                    LOG_ERROR("Error mpd_run_save");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_SEARCH: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {offset:%u, filter:%Q, searchstr:%Q, cols: %M}}", 
                &uint_buf1, &p_charbuf1, &p_charbuf2, json_to_tags, tagcols);
            if (je == 4) {
                response->length = mpd_client_search_queue(mpd_state, response->data, p_charbuf1, uint_buf1, p_charbuf2, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
            }
            free(tagcols);
            break;
        }
        case MPD_API_DATABASE_SEARCH: {
            t_tags *tagcols = (t_tags *)malloc(sizeof(t_tags));
            assert(tagcols);
            je = json_scanf(request->data, request->length, "{data: {searchstr:%Q, filter:%Q, plist:%Q, offset:%u, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &p_charbuf3, &uint_buf1, json_to_tags, tagcols);
            if (je == 5) {
                response->length = mpd_client_search(mpd_state, response->data, p_charbuf1, p_charbuf2, p_charbuf3, uint_buf1, tagcols);
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
            je = json_scanf(request->data, request->length, "{data: {expression:%Q, sort:%Q, sortdesc:%B, plist:%Q, offset:%u, cols: %M}}", 
                &p_charbuf1, &p_charbuf2, &bool_buf, &p_charbuf3, &uint_buf1, json_to_tags, tagcols);
            if (je == 6) {
                response->length = mpd_client_search_adv(mpd_state, response->data, p_charbuf1, p_charbuf2, bool_buf, NULL, p_charbuf3, uint_buf1, tagcols);
                FREE_PTR(p_charbuf1);
                FREE_PTR(p_charbuf2);
                FREE_PTR(p_charbuf3);
            }
            free(tagcols);
            break;
        }
        case MPD_API_QUEUE_SHUFFLE:
            if (mpd_run_shuffle(mpd_state->conn))
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else {
                response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Shuffling queue failed\"}");
                LOG_ERROR("Error mpd_run_shuffle");
            }
            break;
        case MPD_API_PLAYLIST_RM:
            je = json_scanf(request->data, request->length, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                //remove smart playlist
                if (validate_string(p_charbuf1) == false) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Invalid filename\"}");
                    break;
                }
                size_t pl_file_len = config->varlibdir_len + strlen(p_charbuf1) + 11;
                char pl_file[pl_file_len];
                snprintf(pl_file, pl_file_len, "%s/smartpls/%s", config->varlibdir, p_charbuf1);
                if (access(pl_file, F_OK ) != -1 ) {
                    if (unlink(pl_file) == -1) {
                        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Deleting smart playlist failed\"}");
                        LOG_ERROR("Deleting smart playlist failed");
                        FREE_PTR(p_charbuf1);
                        break;
                    }
                }
                //remove mpd playlist
                if (mpd_run_rm(mpd_state->conn, p_charbuf1)) {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                }
                else {
                    response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Deleting playlist failed\"}");
                    LOG_ERROR("Error mpd_run_rm()");
                }
                FREE_PTR(p_charbuf1);
            }
            break;
        case MPD_API_SETTINGS_GET:
            response->length = mpd_client_put_settings(mpd_state, response->data);
            break;
        case MPD_API_DATABASE_STATS:
            response->length = mpd_client_put_stats(mpd_state, response->data);
            break;
        default:
            response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown request\"}");
            LOG_ERROR("Unknown API request: %.*s", request->length, request->data);
    }

    if (mpd_state->conn_state == MPD_CONNECTED && mpd_connection_get_error(mpd_state->conn) != MPD_ERROR_SUCCESS) {
        LOG_ERROR("MPD error: %s", mpd_connection_get_error_message(mpd_state->conn));
        response->length = snprintf(response->data, MAX_SIZE, "{\"type\":\"error\", \"data\": \"%s\"}", 
            mpd_connection_get_error_message(mpd_state->conn));

        /* Try to recover error */
        if (!mpd_connection_clear_error(mpd_state->conn))
            mpd_state->conn_state = MPD_FAILURE;
    }

    if (response->length == 0) {
        response->length = snprintf(response->data, MAX_SIZE, "{\"type\": \"error\", \"data\": \"No response for cmd_id %%{cmdId}\", \"values\": {\"cmdId\": %d}}", request->cmd_id);
    }
    if (response->conn_id > -1) {
        LOG_DEBUG("Push response to queue for connection %lu: %s", request->conn_id, response->data);
        tiny_queue_push(web_server_queue, response);
    }
    else {
        FREE_PTR(response);
    }
    FREE_PTR(request);
}

static int mpd_client_get_updatedb_state(t_mpd_state *mpd_state, char *buffer) {
    struct mpd_status *status;
    int len, update_id;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    status = mpd_run_status(mpd_state->conn);
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }
    update_id = mpd_status_get_update_id(status);
    LOG_INFO("Update database ID: %d", update_id);
    if ( update_id > 0) {
        len = json_printf(&out, "{type: update_started, data: {jobid: %d}}", update_id);
    }
    else {
        len = json_printf(&out, "{type: update_finished}");
    }
    mpd_status_free(status);    
    
    CHECK_RETURN_LEN();
}

static int mpd_client_get_state(t_mpd_state *mpd_state, char *buffer) {
    int len = 0;
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        LOG_ERROR_AND_RECOVER("mpd_run_status");
        return -1;
    }

    int song_id = mpd_status_get_song_id(status);
    if (mpd_state->song_id != song_id) {
        mpd_state->last_song_id = mpd_state->song_id;
        mpd_state->last_song_end_time = mpd_state->song_end_time;
        mpd_state->last_song_start_time = mpd_state->song_start_time;
        struct mpd_song *song = mpd_run_current_song(mpd_state->conn);
        if (song != NULL) {
            FREE_PTR(mpd_state->last_song_uri);
            if (mpd_state->song_uri != NULL) {
                mpd_state->last_song_uri = mpd_state->song_uri;
            }
            mpd_state->song_uri = strdup(mpd_song_get_uri(song));
            mpd_song_free(song);
        }
        else {
            FREE_PTR(mpd_state->song_uri);
        }
        mpd_response_finish(mpd_state->conn);
    }

    mpd_state->state = mpd_status_get_state(status);
    mpd_state->song_id = song_id;
    mpd_state->next_song_id = mpd_status_get_next_song_id(status);
    mpd_state->queue_version = mpd_status_get_queue_version(status);
    mpd_state->queue_length = mpd_status_get_queue_length(status);
    mpd_state->crossfade = mpd_status_get_crossfade(status);

    const int total_time = mpd_status_get_total_time(status);
    const int elapsed_time =  mpd_status_get_elapsed_time(status);
    if (total_time > 10) {
        time_t now = time(NULL);
        mpd_state->song_end_time = now + total_time - elapsed_time - 10;
        mpd_state->song_start_time = now - elapsed_time;
        int half_time = total_time / 2;
        
        if (half_time > 240) {
            mpd_state->set_song_played_time = now - elapsed_time + 240;
        }
        else {
            mpd_state->set_song_played_time = elapsed_time < half_time ? now - elapsed_time + half_time : now;
        }
    }
    else {
        //don't track songs with length < 10s
        mpd_state->song_end_time = 0;
        mpd_state->song_start_time = 0;
        mpd_state->set_song_played_time = 0;
    }
    
    if (buffer != NULL) {
        len = mpd_client_put_state(mpd_state, status, buffer);
    }
    mpd_status_free(status);
    return len;
}

static int mpd_client_put_state(t_mpd_state *mpd_state, struct mpd_status *status, char *buffer) {
    int len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    const struct mpd_audio_format *audioformat = mpd_status_get_audio_format(status);

    len = json_printf(&out, "{type: update_state, data: {state: %d, volume: %d, songPos: %d, elapsedTime: %d, "
        "totalTime: %d, currentSongId: %d, kbitrate: %d, audioFormat: { sampleRate: %d, bits: %d, channels: %d}, "
        "queueLength: %d, nextSongPos: %d, nextSongId: %d, lastSongId: %d, queueVersion: %d}}", 
        mpd_status_get_state(status),
        mpd_status_get_volume(status), 
        mpd_status_get_song_pos(status),
        mpd_status_get_elapsed_time(status),
        mpd_status_get_total_time(status),
        mpd_status_get_song_id(status),
        mpd_status_get_kbit_rate(status),
        audioformat ? audioformat->sample_rate : 0, 
        audioformat ? audioformat->bits : 0, 
        audioformat ? audioformat->channels : 0,
        mpd_status_get_queue_length(status),
        mpd_status_get_next_song_pos(status),
        mpd_status_get_next_song_id(status),
        mpd_state->last_song_id ? mpd_state->last_song_id : -1,
        mpd_status_get_queue_version(status)
    );
   
    CHECK_RETURN_LEN();
}

static int mpd_client_put_volume(t_mpd_state *mpd_state, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }
    len = json_printf(&out, "{type: update_volume, data: {volume: %d}}",
        mpd_status_get_volume(status)
    );
    mpd_status_free(status);

    CHECK_RETURN_LEN();
}

static int mpd_client_put_settings(t_mpd_state *mpd_state, char *buffer) {
    char *replaygain = NULL;
    size_t len, nr;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    struct mpd_status *status = mpd_run_status(mpd_state->conn);
    if (!status) {
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
    }

    if (!mpd_send_command(mpd_state->conn, "replay_gain_status", NULL)) {
        RETURN_ERROR_AND_RECOVER("replay_gain_status");
    }
    struct mpd_pair *pair = mpd_recv_pair(mpd_state->conn);
    if (!pair) {
        RETURN_ERROR_AND_RECOVER("replay_gain_status");
    }
    replaygain = strdup(pair->value);
    mpd_return_pair(mpd_state->conn, pair);
    mpd_response_finish(mpd_state->conn);
    
    len = json_printf(&out, "{type: settings, data: {"
        "repeat: %d, single: %d, crossfade: %d, consume: %d, random: %d, "
        "mixrampdb: %f, mixrampdelay: %f, replaygain: %Q, featPlaylists: %B,"
        "featTags: %B, featLibrary: %B, featAdvsearch: %B, featStickers: %B,"
        "featSmartpls: %B, featLove: %B, featCoverimage: %B, featFingerprint: %B, "
        "musicDirectoryValue: %Q, mpdConnected: %B, tags: [", 
        mpd_status_get_repeat(status),
        mpd_status_get_single(status),
        mpd_status_get_crossfade(status),
        mpd_status_get_consume(status),
        mpd_status_get_random(status),
        mpd_status_get_mixrampdb(status),
        mpd_status_get_mixrampdelay(status),
        replaygain == NULL ? "" : replaygain,
        mpd_state->feat_playlists,
        mpd_state->feat_tags,
        mpd_state->feat_library,
        mpd_state->feat_advsearch,
        mpd_state->feat_sticker,
        mpd_state->feat_smartpls,
        mpd_state->feat_love,
        mpd_state->feat_coverimage,
        mpd_state->feat_fingerprint,
        mpd_state->music_directory_value,
        true
    );
    mpd_status_free(status);
    FREE_PTR(replaygain);
    
    for (nr = 0; nr < mpd_state->mympd_tag_types_len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->mympd_tag_types[nr]));
    }
    
    len += json_printf(&out, "], searchtags: [");
        for (nr = 0; nr < mpd_state->search_tag_types_len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->search_tag_types[nr]));
    }
    
    len += json_printf(&out, "], browsetags: [");
    for (nr = 0; nr < mpd_state->browse_tag_types_len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->browse_tag_types[nr]));
    }

    len += json_printf(&out, "], allmpdtags: [");
    for (nr = 0; nr < mpd_state->mpd_tag_types_len; nr++) {
        if (nr > 0) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", mpd_tag_name(mpd_state->mpd_tag_types[nr]));
    }

    len += json_printf(&out, "]}}");
    
    CHECK_RETURN_LEN();
}

static int mpd_client_put_outputs(t_mpd_state *mpd_state, char *buffer) {
    struct mpd_output *output;
    size_t len;
    int nr;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    if (!mpd_send_outputs(mpd_state->conn)) {
        RETURN_ERROR_AND_RECOVER("outputs");
    }

    len = json_printf(&out, "{type: outputs, data: {outputs: [");
    nr = 0;    
    while ((output = mpd_recv_output(mpd_state->conn)) != NULL && len < MAX_LIST_SIZE) {
        if (nr++) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "{id: %d, name: %Q, state: %d}",
            mpd_output_get_id(output),
            mpd_output_get_name(output),
            mpd_output_get_enabled(output)
        );
        mpd_output_free(output);
    }
    if (!mpd_response_finish(mpd_state->conn))
        LOG_ERROR_AND_RECOVER("outputs");

    len += json_printf(&out,"]}}");
    
    CHECK_RETURN_LEN();
}

static int mpd_client_put_current_song(t_config *config, t_mpd_state *mpd_state, char *buffer) {
    size_t len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    struct mpd_song *song = mpd_run_current_song(mpd_state->conn);
    if (song == NULL) {
        len = json_printf(&out, "{type: result, data: ok}");
        return len;
    }
    
    len = json_printf(&out, "{type: song_change, data: {pos: %d, currentSongId: %d, ",
        mpd_song_get_pos(song),
        mpd_state->song_id
    );
    PUT_SONG_TAG_ALL();

    mpd_response_finish(mpd_state->conn);
    
    sds cover = sdsempty();
    cover = mpd_client_get_cover(config, mpd_state, mpd_song_get_uri(song), cover);
    len += json_printf(&out, ", cover: %Q", cover);
    
    if (mpd_state->feat_sticker) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        assert(sticker);
        mpd_client_get_sticker(mpd_state, mpd_song_get_uri(song), sticker);
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
    mpd_song_free(song);
    sdsfree(cover);

    CHECK_RETURN_LEN();
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
