/* myMPD
   (c) 2018 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
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
#include <libgen.h>
#include <poll.h>
#include <mpd/client.h>

#include "mpd_client.h"
#include "config.h"
#include "../dist/src/frozen/frozen.h"

const char * mpd_cmd_strs[] = {
    MPD_CMDS(GEN_STR)
};

static inline enum mpd_cmd_ids get_cmd_id(const char *cmd) {
    for (unsigned i = 0; i < sizeof(mpd_cmd_strs) / sizeof(mpd_cmd_strs[0]); i++)
        if (!strncmp(cmd, mpd_cmd_strs[i], strlen(mpd_cmd_strs[i])))
            return i;

    return -1;
}

enum mpd_idle idle_bitmask_save = 0;

void callback_mympd(struct mg_connection *nc, const struct mg_str msg) {
    size_t n = 0;
    char *cmd;
    unsigned int uint_buf1, uint_buf2, uint_rc;
    int je, int_buf1, int_rc; 
    float float_buf;
    char *p_charbuf1, *p_charbuf2, *p_charbuf3, *p_charbuf4;
    char p_char[4];
    enum mpd_cmd_ids cmd_id;
    struct pollfd fds[1];
    int pollrc;
    
    #ifdef DEBUG
    fprintf(stderr, "Got request: %s\n", msg.p);
    #endif
    
    je = json_scanf(msg.p, msg.len, "{cmd: %Q}", &cmd);
    if (je == 1) 
        cmd_id = get_cmd_id(cmd);
    else
        cmd_id = get_cmd_id("MPD_API_UNKNOWN");

    if (cmd_id == -1)
        cmd_id = get_cmd_id("MPD_API_UNKNOWN");

    mpd_send_noidle(mpd.conn);
    //save idle events (processing later)
    fds[0].fd = mpd_connection_get_fd(mpd.conn);
    fds[0].events = POLLIN;
    pollrc = poll(fds, 1, 100);
    if (pollrc > 0) {
        idle_bitmask_save = mpd_recv_idle(mpd.conn, false);
        #ifdef DEBUG
        if (idle_bitmask_save > 0)
            fprintf(stderr, "IDLE EVENT BEFORE REQUEST: %d\n", idle_bitmask_save);
        #endif
    }
    mpd_response_finish(mpd.conn);
    //handle request
    #ifdef DEBUG
    fprintf(stderr, "HANDLE REQUEST: %s\n", cmd);
    #endif
    switch(cmd_id) {
        case MPD_API_UNKNOWN:
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown request\"}");
            break;
        case MPD_API_LIKE:
            if (config.stickers) {
                je = json_scanf(msg.p, msg.len, "{data: {uri: %Q, like: %d}}", &p_charbuf1, &uint_buf1);
                if (je == 2) {        
                    mympd_like_song_uri(p_charbuf1, uint_buf1);
                    n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                    free(p_charbuf1);
                }
            } 
            else {
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"error\", \"data\": \"MPD stickers are disabled\"}");
            }
            break;
        case MPD_API_COLS_SAVE:
            je = json_scanf(msg.p, msg.len, "{data: {table: %Q}}", &p_charbuf1);
            if (je == 1) {
                char column_list[800];
                snprintf(column_list, 800, "%.*s", msg.len, msg.p);
                char *cols = strchr(column_list, '[');
                int len = strlen(cols); 
                if (len > 1)
                    cols[len - 2]  = '\0';
                if (strcmp(p_charbuf1,"colsQueue")==0) {
                    free(mympd_state.colsQueue);
                    mympd_state.colsQueue = strdup(cols);
                }
                else if (strcmp(p_charbuf1,"colsSearch")==0) {
                    free(mympd_state.colsSearch);
                    mympd_state.colsSearch = strdup(cols);
                }
                else if (strcmp(p_charbuf1,"colsBrowseDatabase")==0) {
                    free(mympd_state.colsBrowseDatabase);
                    mympd_state.colsBrowseDatabase = strdup(cols);
                }
                else if (strcmp(p_charbuf1,"colsBrowsePlaylistsDetail")==0) {
                    free(mympd_state.colsBrowsePlaylistsDetail);
                    mympd_state.colsBrowsePlaylistsDetail = strdup(cols);
                }
                else if (strcmp(p_charbuf1,"colsBrowseFilesystem")==0) {
                    free(mympd_state.colsBrowseFilesystem);
                    mympd_state.colsBrowseFilesystem = strdup(cols);
                }
                mympd_state_set(p_charbuf1, cols);
                free(p_charbuf1);
            }
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_SYSCMD:
            if (config.syscmds == true) {
                je = json_scanf(msg.p, msg.len, "{data: {cmd: %Q}}", &p_charbuf1);
                if (je == 1) {
                    int_buf1 = list_get_value(&syscmds, p_charbuf1);
                    if (int_buf1 > -1)
                        n = mympd_syscmd(mpd.buf, p_charbuf1, int_buf1);
                    free(p_charbuf1);
                }
            } 
            else
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"error\", \"data\": \"System commands are disabled.\"}");
            break;
        case MPD_API_PLAYER_STATE:
            n = mympd_put_state(mpd.buf, &mpd.song_id, &mpd.next_song_id, &mpd.last_song_id, &mpd.queue_version, &mpd.queue_length);
            break;
        case MPD_API_SETTINGS_SET:
            je = json_scanf(msg.p, msg.len, "{data: {notificationWeb: %B}}", &mympd_state.notificationWeb);
            if (je == 1)
                mympd_state_set("notificationWeb", (mympd_state.notificationWeb == true ? "true" : "false"));
            
            je = json_scanf(msg.p, msg.len, "{data: {notificationPage: %B}}", &mympd_state.notificationPage);
            if (je == 1)
                mympd_state_set("notificationPage", (mympd_state.notificationPage == true ? "true" : "false"));
            
            je = json_scanf(msg.p, msg.len, "{data: {jukeboxMode: %d}}", &mympd_state.jukeboxMode);
            if (je == 1) {
                snprintf(p_char, 4, "%d", mympd_state.jukeboxMode);
                mympd_state_set("jukeboxMode", p_char);
            }

            je = json_scanf(msg.p, msg.len, "{data: {jukeboxPlaylist: %Q}}", &mympd_state.jukeboxPlaylist);
            if (je == 1)
                mympd_state_set("jukeboxPlaylist", mympd_state.jukeboxPlaylist);

            je = json_scanf(msg.p, msg.len, "{data: {jukeboxQueueLength: %d}}", &mympd_state.jukeboxQueueLength);
            if (je == 1) {
                snprintf(p_char, 4, "%d", mympd_state.jukeboxQueueLength);
                mympd_state_set("jukeboxQueueLength", p_char);
            }
        
            je = json_scanf(msg.p, msg.len, "{data: {random: %u}}", &uint_buf1);
            if (je == 1)        
                mpd_run_random(mpd.conn, uint_buf1);
            
            je = json_scanf(msg.p, msg.len, "{data: {repeat: %u}}", &uint_buf1);
            if (je == 1)        
                mpd_run_repeat(mpd.conn, uint_buf1);
            
            je = json_scanf(msg.p, msg.len, "{data: {consume: %u}}", &uint_buf1);
            if (je == 1)        
                mpd_run_consume(mpd.conn, uint_buf1);
            
            je = json_scanf(msg.p, msg.len, "{data: {single: %u}}", &uint_buf1);
            if (je == 1)
                mpd_run_single(mpd.conn, uint_buf1);
            
            je = json_scanf(msg.p, msg.len, "{data: {crossfade: %u}}", &uint_buf1);
            if (je == 1)
                mpd_run_crossfade(mpd.conn, uint_buf1);
            
            if (config.mixramp) {
                je = json_scanf(msg.p, msg.len, "{data: {mixrampdb: %f}}", &float_buf);
                if (je == 1)        
                    mpd_run_mixrampdb(mpd.conn, float_buf);
            
                je = json_scanf(msg.p, msg.len, "{data: {mixrampdelay: %f}}", &float_buf);
                if (je == 1)        
                    mpd_run_mixrampdelay(mpd.conn, float_buf);
            }
            
            je = json_scanf(msg.p, msg.len, "{data: {replaygain: %Q}}", &p_charbuf1);
            if (je == 1) {
                mpd_send_command(mpd.conn, "replay_gain_mode", p_charbuf1, NULL);
                struct mpd_pair *pair;
                while ((pair = mpd_recv_pair(mpd.conn)) != NULL) {
        	    mpd_return_pair(mpd.conn, pair);
                }
                free(p_charbuf1);            
            }
            if (mympd_state.jukeboxMode > 0)
                mympd_jukebox();
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_WELCOME:
            n = mympd_put_welcome(mpd.buf);
            break;
        case MPD_API_DATABASE_UPDATE:
            uint_rc = mpd_run_update(mpd.conn, NULL);
            if (uint_rc > 0)
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_DATABASE_RESCAN:
            uint_rc = mpd_run_rescan(mpd.conn, NULL);
            if (uint_rc > 0)
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_SMARTPLS_UPDATE_ALL:
            uint_rc = mympd_smartpls_update_all();
            if (uint_rc == 0)
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Smart Playlists updated\"}");
            else
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Smart Playlists update failed\"}");
            break;
        case MPD_API_SMARTPLS_SAVE:
            je = json_scanf(msg.p, msg.len, "{data: {type: %Q}}", &p_charbuf1);
            if (je == 1) {
                if (strcmp(p_charbuf1, "sticker") == 0) {
                    je = json_scanf(msg.p, msg.len, "{data: {playlist: %Q, sticker: %Q, maxentries: %d}}", &p_charbuf2, &p_charbuf3, &int_buf1);
                    n = mympd_smartpls_save(p_charbuf1, p_charbuf2, p_charbuf3, NULL, int_buf1, 0);
                    free(p_charbuf2);
                    free(p_charbuf3);
                }
                else if (strcmp(p_charbuf1, "newest") == 0) {
                    je = json_scanf(msg.p, msg.len, "{data: {playlist: %Q, timerange: %d}}", &p_charbuf2, &int_buf1);
                    n = mympd_smartpls_save(p_charbuf1, p_charbuf2, NULL, NULL, 0, int_buf1);
                    free(p_charbuf2);
                }            
                else if (strcmp(p_charbuf1, "search") == 0) {
                    je = json_scanf(msg.p, msg.len, "{data: {playlist: %Q, tag: %Q, searchstr: %Q}}", &p_charbuf2, &p_charbuf3, &p_charbuf4);
                    n = mympd_smartpls_save(p_charbuf1, p_charbuf2, p_charbuf3, p_charbuf4, 0, 0);
                    free(p_charbuf2);
                    free(p_charbuf3);                    
                    free(p_charbuf4);
                }
                free(p_charbuf1);
            }
            if (n == 0)
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            else
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Error saving playlist\"}");
            break;
        case MPD_API_SMARTPLS_GET:
            je = json_scanf(msg.p, msg.len, "{data: {playlist: %Q}}", &p_charbuf1);
            if (je == 1) {
                n = mympd_smartpls_put(mpd.buf, p_charbuf1);
                free(p_charbuf1);
            }
            break;
        case MPD_API_PLAYER_PAUSE:
            mpd_run_toggle_pause(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_PLAYER_PREV:
            mpd_run_previous(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_PLAYER_NEXT:
            if (config.stickers)
                mympd_count_song_id(mpd.song_id, "skipCount", 1);
            mpd_run_next(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_PLAYER_PLAY:
            mpd_run_play(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_PLAYER_STOP:
            mpd_run_stop(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_QUEUE_CLEAR:
            mpd_run_clear(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_QUEUE_CROP:
            n = mympd_queue_crop(mpd.buf);
            break;
        case MPD_API_QUEUE_RM_TRACK:
            je = json_scanf(msg.p, msg.len, "{data: {track:%u}}", &uint_buf1);
            if (je == 1) {
                mpd_run_delete_id(mpd.conn, uint_buf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_QUEUE_RM_RANGE:
            je = json_scanf(msg.p, msg.len, "{data: {start: %u, end: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                mpd_run_delete_range(mpd.conn, uint_buf1, uint_buf2);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_QUEUE_MOVE_TRACK:
            je = json_scanf(msg.p, msg.len, "{data: {from: %u, to: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2)
                    uint_buf2--;
                mpd_run_move(mpd.conn, uint_buf1, uint_buf2);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_PLAYLIST_MOVE_TRACK:
            je = json_scanf(msg.p, msg.len, "{data: {plist: %Q, from: %u, to: %u }}", &p_charbuf1, &uint_buf1, &uint_buf2);
            if (je == 3) {
                uint_buf1--;
                uint_buf2--;
                if (uint_buf1 < uint_buf2)
                    uint_buf2--;
                mpd_send_playlist_move(mpd.conn, p_charbuf1, uint_buf1, uint_buf2);
                mpd_response_finish(mpd.conn);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_PLAYER_PLAY_TRACK:
            je = json_scanf(msg.p, msg.len, "{data: { track:%u}}", &uint_buf1);
            if (je == 1) {
                mpd_run_play_id(mpd.conn, uint_buf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_PLAYER_OUTPUT_LIST:
            n = mympd_put_outputs(mpd.buf);
            break;
        case MPD_API_PLAYER_TOGGLE_OUTPUT:
            je = json_scanf(msg.p, msg.len, "{data: {output: %u, state: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (uint_buf2)
                    mpd_run_enable_output(mpd.conn, uint_buf1);
                else
                    mpd_run_disable_output(mpd.conn, uint_buf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_PLAYER_VOLUME_SET:
            je = json_scanf(msg.p, msg.len, "{data: {volume:%u}}", &uint_buf1);
            if (je == 1) {
                mpd_run_set_volume(mpd.conn, uint_buf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_PLAYER_VOLUME_GET:
            n = mympd_put_volume(mpd.buf);
            break;            
        case MPD_API_PLAYER_SEEK:
            je = json_scanf(msg.p, msg.len, "{data: {songid: %u, seek: %u}}", &uint_buf1, &uint_buf2);
            if (je == 2) {
                mpd_run_seek_id(mpd.conn, uint_buf1, uint_buf2);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_QUEUE_LIST:
            je = json_scanf(msg.p, msg.len, "{data: {offset: %u}}", &uint_buf1);
            if (je == 1) {
                n = mympd_put_queue(mpd.buf, uint_buf1, &mpd.queue_version, &mpd.queue_length);
            }
            break;
        case MPD_API_PLAYER_CURRENT_SONG:
                n = mympd_put_current_song(mpd.buf);
            break;
        case MPD_API_DATABASE_SONGDETAILS:
            je = json_scanf(msg.p, msg.len, "{data: { uri: %Q}}", &p_charbuf1);
            if (je == 1) {
                n = mympd_put_songdetails(mpd.buf, p_charbuf1);
                free(p_charbuf1);
            }
            break;            
        case MPD_API_DATABASE_TAG_LIST:
            je = json_scanf(msg.p, msg.len, "{data: {offset: %u, filter: %Q, tag: %Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                n = mympd_put_db_tag(mpd.buf, uint_buf1, p_charbuf2, "", "", p_charbuf1);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_LIST:
            je = json_scanf(msg.p, msg.len, "{data: {offset: %u, filter: %Q, search: %Q, tag: %Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 4) {
                n = mympd_put_db_tag(mpd.buf, uint_buf1, "Album", p_charbuf3, p_charbuf2, p_charbuf1);
                free(p_charbuf1);
                free(p_charbuf2);
                free(p_charbuf3);
            }
            break;
        case MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST:
            je = json_scanf(msg.p, msg.len, "{data: {album: %Q, search: %Q, tag: %Q}}", &p_charbuf1, &p_charbuf2, &p_charbuf3);
            if (je == 3) {
                n = mympd_put_songs_in_album(mpd.buf, p_charbuf1, p_charbuf2, p_charbuf3);
                free(p_charbuf1);
                free(p_charbuf2);
                free(p_charbuf3);
            } 
            break;
        case MPD_API_PLAYLIST_RENAME:
            je = json_scanf(msg.p, msg.len, "{data: {from: %Q, to: %Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                //rename smart playlist
                char old_pl_file[400];
                char new_pl_file[400];
                snprintf(old_pl_file, 400, "%s/smartpls/%s", config.varlibdir, p_charbuf1);
                snprintf(new_pl_file, 400, "%s/smartpls/%s", config.varlibdir, p_charbuf2);
                if (access(old_pl_file, F_OK ) != -1) {
                    if (access(new_pl_file, F_OK ) == -1) {
                        rename(old_pl_file, new_pl_file);
                        //rename mpd playlist
                        mpd_run_rename(mpd.conn, p_charbuf1, p_charbuf2);
                        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Renamed playlist %s to %s\"}", p_charbuf1, p_charbuf2);
                    } else 
                        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Renamed playlist %s already exists\"}", p_charbuf2);
                }
                else {
                    mpd_run_rename(mpd.conn, p_charbuf1, p_charbuf2);
                    n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Renamed playlist %s to %s\"}", p_charbuf1, p_charbuf2);
                }
                free(p_charbuf1);
                free(p_charbuf2);                
            }
            break;            
        case MPD_API_PLAYLIST_LIST:
            je = json_scanf(msg.p, msg.len, "{data: {offset: %u, filter: %Q}}", &uint_buf1, &p_charbuf1);
            if (je == 2) {
                n = mympd_put_playlists(mpd.buf, uint_buf1, p_charbuf1);
                free(p_charbuf1);
            }
            break;
        case MPD_API_PLAYLIST_CONTENT_LIST:
            je = json_scanf(msg.p, msg.len, "{data: {uri: %Q, offset:%u, filter:%Q}}", &p_charbuf1, &uint_buf1, &p_charbuf2);
            if (je == 3) {
                n = mympd_put_playlist_list(mpd.buf, p_charbuf1, uint_buf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_PLAYLIST_ADD_TRACK:
            je = json_scanf(msg.p, msg.len, "{data: {plist:%Q, uri:%Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                mpd_run_playlist_add(mpd.conn, p_charbuf1, p_charbuf2);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Added %s to playlist %s\"}", p_charbuf2, p_charbuf1);
                free(p_charbuf1);
                free(p_charbuf2);                
            }
            break;
        case MPD_API_PLAYLIST_CLEAR:
            je = json_scanf(msg.p, msg.len, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                mpd_run_playlist_clear(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_PLAYLIST_RM_TRACK:
            je = json_scanf(msg.p, msg.len, "{data: {uri:%Q, track:%u}}", &p_charbuf1, &uint_buf1);
            if (je == 2) {
                mpd_run_playlist_delete(mpd.conn, p_charbuf1, uint_buf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_DATABASE_FILESYSTEM_LIST:
            je = json_scanf(msg.p, msg.len, "{data: {offset:%u, filter:%Q, path:%Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                n = mympd_put_browse(mpd.buf, p_charbuf2, uint_buf1, p_charbuf1);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_QUEUE_ADD_TRACK_AFTER:
            je = json_scanf(msg.p, msg.len, "{data: {uri:%Q, to:%d}}", &p_charbuf1, &int_buf1);
            if (je == 2) {
                int_rc = mpd_run_add_id_to(mpd.conn, p_charbuf1, int_buf1);
                if (int_rc > -1 ) 
                    n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                free(p_charbuf1);
            }
            break;
        case MPD_API_QUEUE_REPLACE_TRACK:
            je = json_scanf(msg.p, msg.len, "{data: {uri:%Q }}", &p_charbuf1);
            if (je == 1) {
                mpd_run_clear(mpd.conn);
                mpd_run_add(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                mpd_run_play(mpd.conn);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_QUEUE_ADD_TRACK:
            je = json_scanf(msg.p, msg.len, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                mpd_run_add(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_QUEUE_ADD_PLAY_TRACK:
            je = json_scanf(msg.p, msg.len, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                int_buf1 = mpd_run_add_id(mpd.conn, p_charbuf1);
                if (int_buf1 != -1)
                    mpd_run_play_id(mpd.conn, int_buf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_QUEUE_REPLACE_PLAYLIST:
            je = json_scanf(msg.p, msg.len, "{data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                mpd_run_clear(mpd.conn);
                mpd_run_load(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                mpd_run_play(mpd.conn);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_QUEUE_ADD_PLAYLIST:
            je = json_scanf(msg.p, msg.len, "{data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                mpd_run_load(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_QUEUE_SAVE:
            je = json_scanf(msg.p, msg.len, "{ data: {plist:%Q}}", &p_charbuf1);
            if (je == 1) {
                mpd_run_save(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_QUEUE_SEARCH:
            je = json_scanf(msg.p, msg.len, "{data: {offset:%u, filter:%Q, searchstr:%Q}}", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                n = mympd_search_queue(mpd.buf, p_charbuf1, uint_buf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;            
        case MPD_API_DATABASE_SEARCH:
            je = json_scanf(msg.p, msg.len, "{data: {searchstr:%Q, filter:%Q, plist:%Q, offset:%u}}", &p_charbuf1, &p_charbuf2, &p_charbuf3, &uint_buf1);
            if (je == 4) {
                n = mympd_search(mpd.buf, p_charbuf1, p_charbuf2, p_charbuf3, uint_buf1);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_QUEUE_SHUFFLE:
            mpd_run_shuffle(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_MESSAGE_SEND:
            je = json_scanf(msg.p, msg.len, "{data: { channel:%Q, text:%Q}}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                mpd_run_send_message(mpd.conn, p_charbuf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_PLAYLIST_RM:
            je = json_scanf(msg.p, msg.len, "{data: {uri:%Q}}", &p_charbuf1);
            if (je == 1) {
                //remove smart playlist
                char pl_file[400];
                snprintf(pl_file, 400, "%s/smartpls/%s", config.varlibdir, p_charbuf1);
                if (access(pl_file, F_OK ) != -1 )
                    unlink(pl_file);
                //remove mpd playlist
                mpd_run_rm(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_SETTINGS_GET:
            n = mympd_put_settings(mpd.buf);
            break;
        case MPD_API_DATABASE_STATS:
            n = mympd_put_stats(mpd.buf);
            break;
    }

    if (mpd.conn_state == MPD_CONNECTED && mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS) {
        #ifdef DEBUG
        fprintf(stderr, "Error: %s\n", mpd_connection_get_error_message(mpd.conn));
        #endif
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"error\", \"data\": \"%s\"}", 
            mpd_connection_get_error_message(mpd.conn));

        /* Try to recover error */
        if (!mpd_connection_clear_error(mpd.conn))
            mpd.conn_state = MPD_FAILURE;
    }

    mpd_send_idle(mpd.conn);
    
    if (n == 0)
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"error\", \"data\": \"No response for cmd %s\"}", cmd);
    
    if (is_websocket(nc)) {
        #ifdef DEBUG
        fprintf(stderr, "Send websocket response:\n %s\n", mpd.buf);
        #endif
        mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, mpd.buf, n);
    }
    else {
        #ifdef DEBUG
        fprintf(stderr, "Send http response (first 800 chars):\n%*.*s\n", 0, 800, mpd.buf);
        #endif
        mg_send_http_chunk(nc, mpd.buf, n);
    }
    free(cmd);    
}

void mympd_notify(struct mg_mgr *s) {
    for (struct mg_connection *c = mg_next(s, NULL); c != NULL; c = mg_next(s, c)) {
        if (!is_websocket(c))
            continue;
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, strlen(mpd.buf));
    }
    #ifdef DEBUG
    fprintf(stderr, "NOTIFY: %s\n", mpd.buf);
    #endif
}

void mympd_parse_idle(struct mg_mgr *s, int idle_bitmask) {
    int len = 0;
    for (unsigned j = 0;; j++) {
        enum mpd_idle idle_event = 1 << j;
        const char *idle_name = mpd_idle_name(idle_event);
        if (idle_name == NULL)
            break;
        if (idle_bitmask & idle_event) {
            #ifdef DEBUG
            fprintf(stderr, "IDLE: %s\n", idle_name);
            #endif
            switch(idle_event) {
                case MPD_IDLE_DATABASE:
                    len = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"update_database\"}");
                    mympd_smartpls_update_all();
                    break;
                case MPD_IDLE_STORED_PLAYLIST:
                    len = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"update_stored_playlist\"}");
                    break;
                case MPD_IDLE_QUEUE:
                    len = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"update_queue\"}");
                    if (mympd_state.jukeboxMode > 0)
                        mympd_jukebox();
                    break;
                case MPD_IDLE_PLAYER:
                    len = mympd_put_state(mpd.buf, &mpd.song_id, &mpd.next_song_id, &mpd.last_song_id, &mpd.queue_version, &mpd.queue_length);
                    if (config.stickers && mpd.song_id != mpd.last_song_id && mpd.last_update_sticker_song_id != mpd.song_id) {
                        mympd_count_song_id(mpd.song_id, "playCount", 1);
                        mympd_last_played_song_id(mpd.song_id);
                        mpd.last_update_sticker_song_id = mpd.song_id;
                    }
                    break;
                case MPD_IDLE_MIXER:
                    len = mympd_put_volume(mpd.buf);
                    break;
                case MPD_IDLE_OUTPUT:
                    len = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"update_outputs\"}");
                    break;
                case MPD_IDLE_OPTIONS:
                    len = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"update_options\"}");
                    break;
                case MPD_IDLE_UPDATE:
                    len = mympd_get_updatedb_state(mpd.buf);
                    break;
                case MPD_IDLE_STICKER:
                    len = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"update_sticker\"}");
                    break;
                case MPD_IDLE_SUBSCRIPTION:
                    len = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"update_subscription\"}");
                    break;
                case MPD_IDLE_MESSAGE:
                    len = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"update_message\"}");
                    break;
                default:
                    len = 0;
            }
            if (len > 0)
                mympd_notify(s);
        }
    }
}

void mympd_mpd_features() {
    struct mpd_pair *pair;
    char s[2] = ",";
    char *taglist = strdup(config.taglist);
    char *searchtaglist = strdup(config.searchtaglist);    
    char *browsetaglist = strdup(config.browsetaglist);
    char *token;    

    mpd.protocol = mpd_connection_get_server_version(mpd.conn);

    // Defaults
    mpd.feat_sticker = false;
    mpd.feat_playlists = false;
    mpd.feat_tags = false;

    mpd_send_allowed_commands(mpd.conn);
    while ((pair = mpd_recv_command_pair(mpd.conn)) != NULL) {
        if (strcmp(pair->value, "sticker") == 0)
            mpd.feat_sticker = true;
        if (strcmp(pair->value, "listplaylists") == 0)
            mpd.feat_playlists = true;
        mpd_return_pair(mpd.conn, pair);
    }
    mpd_response_finish(mpd.conn);
    printf("MPD protocoll version: %i.%i.%i\n", mpd.protocol[0], mpd.protocol[1], mpd.protocol[2]);
    if (mpd.feat_sticker == false && config.stickers == true) {
        printf("MPD don't support stickers, disabling myMPD feature\n");
        config.stickers = false;
    }
    if (config.stickers == false && config.smartpls == true) {
        printf("Stickers are disabled, disabling smartplaylists\n");
        config.smartpls = false;
    }
    if (mpd.feat_playlists == false && config.smartpls == true) {
        printf("Playlists are disabled, disabling smartplaylists\n");
        config.smartpls = false;
    }
    
    printf("MPD supported tags: ");
    list_free(&mpd_tags);
    mpd_send_list_tag_types(mpd.conn);
    while ((pair = mpd_recv_tag_type_pair(mpd.conn)) != NULL) {
        printf("%s ", pair->value);
        list_push(&mpd_tags, pair->value, 1);
        mpd_return_pair(mpd.conn, pair);
    }
    mpd_response_finish(mpd.conn);
    list_free(&mympd_tags);
    if (mpd_tags.length == 0) {
        printf("none\nTags are disabled\n");
        mpd.feat_tags = false;
    }
    else {
        mpd.feat_tags = true;
        printf("\nmyMPD enabled tags: ");
        token = strtok(taglist, s);
        while (token != NULL) {
            if (list_get_value(&mpd_tags, token) == 1) {
                list_push(&mympd_tags, token, 1);
                printf("%s ", token);
            }
            token = strtok(NULL, s);
        }
        printf("\nmyMPD enabled searchtags: ");
        token = strtok(searchtaglist, s);
        while (token != NULL) {
            if (list_get_value(&mpd_tags, token) == 1) {
                list_push(&mympd_searchtags, token, 1);
                printf("%s ", token);
            }
            token = strtok(NULL, s);
        }
        printf("\nmyMPD enabled browsetags: ");
        token = strtok(browsetaglist, s);
        while (token != NULL) {
            if (list_get_value(&mpd_tags, token) == 1) {
                list_push(&mympd_browsetags, token, 1);
                printf("%s ", token);
            }
            token = strtok(NULL, s);
        }
        printf("\n");
    }
    free(taglist);
    free(searchtaglist);
    free(browsetaglist);
}

void mympd_idle(struct mg_mgr *s, int timeout) {
    struct pollfd fds[1];
    int pollrc;
    
    switch (mpd.conn_state) {
        case MPD_DISCONNECTED:
            /* Try to connect */
            printf("MPD Connecting to %s:%ld\n", config.mpdhost, config.mpdport);
            mpd.conn = mpd_connection_new(config.mpdhost, config.mpdport, mpd.timeout);
            if (mpd.conn == NULL) {
                printf("MPD connection failed.");
                snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"disconnected\"}");
                mympd_notify(s);
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS) {
                printf("MPD connection: %s\n", mpd_connection_get_error_message(mpd.conn));
                snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"error\", \"data\": \"%s\"}", mpd_connection_get_error_message(mpd.conn));
                mympd_notify(s);
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            if (config.mpdpass && !mpd_run_password(mpd.conn, config.mpdpass)) {
                printf("MPD connection: %s\n", mpd_connection_get_error_message(mpd.conn));
                snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"error\", \"data\": \"%s\"}", mpd_connection_get_error_message(mpd.conn));
                mympd_notify(s);
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            printf("MPD connected.\n");
            mpd_connection_set_timeout(mpd.conn, mpd.timeout);
            mpd.conn_state = MPD_CONNECTED;
            mympd_mpd_features();
            mympd_smartpls_update_all();
            if (mympd_state.jukeboxMode > 0)
                mympd_jukebox();
            mpd_send_idle(mpd.conn);
            break;

        case MPD_FAILURE:
            printf("MPD connection failed.\n");
            snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"disconnected\"}");
            mympd_notify(s);

        case MPD_DISCONNECT:
        case MPD_RECONNECT:
            if (mpd.conn != NULL)
                mpd_connection_free(mpd.conn);
            mpd.conn = NULL;
            mpd.conn_state = MPD_DISCONNECTED;
            break;

        case MPD_CONNECTED:
            fds[0].fd = mpd_connection_get_fd(mpd.conn);
            fds[0].events = POLLIN;
            pollrc = poll(fds, 1, timeout);
            if (pollrc > 0 || idle_bitmask_save > 0) {
                //Handle idle event
                mpd_send_noidle(mpd.conn);
                if (pollrc > 0) {
                    enum mpd_idle idle_bitmask = mpd_recv_idle(mpd.conn, false);
                    mympd_parse_idle(s, idle_bitmask);
                } 
                else {
                    mpd_response_finish(mpd.conn);
                }
                if (idle_bitmask_save > 0) {
                    //Handle idle event saved in mympd_callback
                    #ifdef DEBUG
                    fprintf(stderr, "HANDLE SAVED IDLE EVENT\n");
                    #endif
                    mympd_parse_idle(s, idle_bitmask_save);
                    idle_bitmask_save = 0;
                }
                mpd_send_idle(mpd.conn);
            }
            break;
    }
}

int mympd_get_updatedb_state(char *buffer) {
    struct mpd_status *status;
    int len, update_id;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    status = mpd_run_status(mpd.conn);
    if (!status)
        RETURN_ERROR_AND_RECOVER("replay_gain_status");
    update_id = mpd_status_get_update_id(status);
    #ifdef DEBUG
    fprintf(stderr, "Update database ID: %d\n", update_id);
    #endif
    if ( update_id > 0)
        len = json_printf(&out, "{type: update_started, data: {jobid: %d}}", update_id);
    else
        len = json_printf(&out, "{type: update_finished}");

    mpd_status_free(status);    
    
    CHECK_RETURN_LEN();
    return len;
}


void mympd_get_sticker(const char *uri, t_sticker *sticker) {
    struct mpd_pair *pair;
    char *crap;
    sticker->playCount = 0;
    sticker->skipCount = 0;
    sticker->lastPlayed = 0;
    sticker->like = 1;
    if (uri == NULL || strncasecmp("http:", uri, 5) == 0 || strncasecmp("https:", uri, 6) == 0)
        return;

    if (mpd_send_sticker_list(mpd.conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(mpd.conn)) != NULL) {
            if (strcmp(pair->name, "playCount") == 0)
                sticker->playCount = strtol(pair->value, &crap, 10);
            else if (strcmp(pair->name, "skipCount") == 0)
                sticker->skipCount = strtol(pair->value, &crap, 10);
            else if (strcmp(pair->name, "lastPlayed") == 0)
                sticker->lastPlayed = strtol(pair->value, &crap, 10);
            else if (strcmp(pair->name, "like") == 0)
                sticker->like = strtol(pair->value, &crap, 10);
            mpd_return_sticker(mpd.conn, pair);
        }
    }
    else
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_list");
}

void mympd_count_song_id(int song_id, char *name, int value) {
    struct mpd_song *song;
    if (song_id > -1) {
        song = mpd_run_get_queue_song_id(mpd.conn, song_id);
        if (song) {
            mympd_count_song_uri(mpd_song_get_uri(song), name, value);
            mpd_song_free(song);
        }
    }
}

void mympd_count_song_uri(const char *uri, char *name, int value) {
    if (uri == NULL || strncasecmp("http:", uri, 5) == 0 || strncasecmp("https:", uri, 6) == 0)
        return;
    struct mpd_pair *pair;
    char *crap;
    int old_value = 0;
    char v[4];
    if (mpd_send_sticker_list(mpd.conn, "song", uri)) {
        while ((pair = mpd_recv_sticker(mpd.conn)) != NULL) {
            if (strcmp(pair->name, name) == 0)
                old_value = strtol(pair->value, &crap, 10);
            mpd_return_sticker(mpd.conn, pair);
        }
    } 
    old_value += value;
    if (old_value > 999)
        old_value = 999;
    else if (old_value < 0)
        old_value = 0;
    snprintf(v, 4, "%d", old_value);
    #ifdef DEBUG
    fprintf(stderr, "STICKER_COUNT_SONG: \"%s\" -> %s: %s\n", uri, name, v);
    #endif    
    if (!mpd_run_sticker_set(mpd.conn, "song", uri, name, v))
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_set");
}

void mympd_like_song_uri(const char *uri, int value) {
    if (uri == NULL || strncasecmp("http:", uri, 5) == 0 || strncasecmp("https:", uri, 6) == 0)
        return;
    char v[2];
    if (value > 2)
        value = 2;
    else if (value < 0)
        value = 0;
    snprintf(v, 2, "%d", value);
    if (!mpd_run_sticker_set(mpd.conn, "song", uri, "like", v))
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_set");
}

void mympd_last_played_song_id(int song_id) {
    struct mpd_song *song;
    if (song_id > -1) {
        song = mpd_run_get_queue_song_id(mpd.conn, song_id);
        if (song) {
            mympd_last_played_song_uri(mpd_song_get_uri(song));
            mpd_song_free(song);
        }
    }
}

void mympd_last_played_song_uri(const char *uri) {
    if (uri == NULL || strncasecmp("http:", uri, 5) == 0 || strncasecmp("https:", uri, 6) == 0)
        return;
    char v[20];
    snprintf(v, 20, "%lu", time(NULL));
    if (!mpd_run_sticker_set(mpd.conn, "song", uri, "lastPlayed", v))
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_set");
}


char* mympd_get_tag(struct mpd_song const *song, enum mpd_tag_type tag) {
    char *str;
    str = (char *)mpd_song_get_tag(song, tag, 0);
    if (str == NULL) {
        if (tag == MPD_TAG_TITLE)
            str = basename((char *)mpd_song_get_uri(song));
        else if (tag == MPD_TAG_ALBUM_ARTIST)
            str = (char *)mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        else
            str = "-";
    }
    return str;
}

void mympd_jukebox() {
    struct mpd_status *status;
    status = mpd_run_status(mpd.conn);
    int queue_length, addSongs, i;
    struct mpd_entity *entity;
    const struct mpd_song *song;
    struct mpd_pair *pair;
    int lineno = 1;
    int nkeep = 0;

    if (!status) {
        LOG_ERROR_AND_RECOVER("mpd_run_status");
        return;
    }
    queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);
    if (queue_length > mympd_state.jukeboxQueueLength)
        return;

    if (mympd_state.jukeboxMode == 1) 
        addSongs = mympd_state.jukeboxQueueLength - queue_length;
    else
        addSongs = 1;
    
    if (addSongs < 1)
        return;
        
    if (mpd.feat_playlists == false && strcmp(mympd_state.jukeboxPlaylist, "Database") != 0) {
        printf("Jukebox: Playlists are disabled\n");
        return;
    }
    
    srand((unsigned int)time(NULL));

    struct list add_list;
    list_init(&add_list);
    
    if (mympd_state.jukeboxMode == 1) {
        //add songs
        if (strcmp(mympd_state.jukeboxPlaylist, "Database") == 0) {
            if (!mpd_send_list_all(mpd.conn, "/")) {
                LOG_ERROR_AND_RECOVER("mpd_send_list_all");
                list_free(&add_list);
                return;
            }
        }
        else {
            if (!mpd_send_list_playlist(mpd.conn, mympd_state.jukeboxPlaylist)) {
                LOG_ERROR_AND_RECOVER("mpd_send_list_playlist");
                list_free(&add_list);
                return;
            }
        }
        while ((entity = mpd_recv_entity(mpd.conn)) != NULL) {
            if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
                if (randrange(lineno) < addSongs) {
		    if (nkeep < addSongs) {
		        song = mpd_entity_get_song(entity);
		        list_push(&add_list, mpd_song_get_uri(song), lineno);
		        nkeep++;
                    }
                    else {
		        i = 0;
		        if (addSongs > 1)
		            i = randrange(addSongs);
                        if (addSongs == 1) {
                            song = mpd_entity_get_song(entity);
                            list_replace(&add_list, 0, mpd_song_get_uri(song), lineno);
                        }
                        else {
                            song = mpd_entity_get_song(entity);
                            list_replace(&add_list, i, mpd_song_get_uri(song), lineno);
                        }
                    }
                }
                lineno++;
            }
            mpd_entity_free(entity);
        }
    }
    else if (mympd_state.jukeboxMode == 2) {
        //add album
        if (!mpd_search_db_tags(mpd.conn, MPD_TAG_ALBUM)) {
            LOG_ERROR_AND_RECOVER("mpd_search_db_tags");
            list_free(&add_list);
            return;
        }
        if (!mpd_search_commit(mpd.conn)) {
            LOG_ERROR_AND_RECOVER("mpd_search_commit");
            list_free(&add_list);
            return;
        }
        while ((pair = mpd_recv_pair_tag(mpd.conn, MPD_TAG_ALBUM )) != NULL)  {
            if (randrange(lineno) < addSongs) {
		if (nkeep < addSongs) {
                    list_push(&add_list, strdup(pair->value), lineno);
                    nkeep++;
                }
		else {
		    i = 0;
		    if (addSongs > 1)
		        i = randrange(addSongs);
                    if (addSongs == 1) {
                        list_replace(&add_list, 0, strdup(pair->value), lineno);
                    }
                    else {
                        list_replace(&add_list, i, strdup(pair->value), lineno);
                    }
                }
            }
            lineno++;
            mpd_return_pair(mpd.conn, pair);
        }
    }

    if (nkeep < addSongs) {
        fprintf(stderr, "Warning: input didn't contain %d entries\n", addSongs);
    }

    list_shuffle(&add_list);

    struct node *current = add_list.list;
    while (current != NULL) {
        if (mympd_state.jukeboxMode == 1) {
	    printf("Jukebox adding song: %s\n", current->data);
	    if (!mpd_run_add(mpd.conn, current->data)) {
                LOG_ERROR_AND_RECOVER("mpd_run_add");
            }
        }
        else {
            printf("Jukebox adding album: %s\n", current->data);
            if (!mpd_send_command(mpd.conn, "searchadd", "Album", current->data, NULL)) {
                LOG_ERROR_AND_RECOVER("mpd_send_command");
                return;
            }
            mpd_response_finish(mpd.conn);
        }
        current = current->next;
    }
    list_free(&add_list);
    mpd_run_play(mpd.conn);
}

int randrange(int n) {
    return rand() / (RAND_MAX / (n + 1) + 1);
}

int mympd_put_state(char *buffer, int *current_song_id, int *next_song_id, int *last_song_id, unsigned *queue_version, unsigned *queue_length) {
    struct mpd_status *status;
    const struct mpd_audio_format *audioformat;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    status = mpd_run_status(mpd.conn);
    if (!status) {
        printf("MPD mpd_run_status: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return 0;
    }
    audioformat = mpd_status_get_audio_format(status);
    int song_id = mpd_status_get_song_id(status);
    if (*current_song_id != song_id)
        *last_song_id = *current_song_id;
    
    len = json_printf(&out, "{type: update_state, data: {"
        "state: %d, volume: %d, songPos: %d, elapsedTime: %d, "
        "totalTime: %d, currentSongId: %d, kbitrate: %d, "
        "audioFormat: { sampleRate: %d, bits: %d, channels: %d}, "
        "queueLength: %d, nextSongPos: %d, nextSongId: %d, lastSongId: %d, "
        "queueVersion: %d", 
        mpd_status_get_state(status),
        mpd_status_get_volume(status), 
        mpd_status_get_song_pos(status),
        mpd_status_get_elapsed_time(status),
        mpd_status_get_total_time(status),
        song_id,
        mpd_status_get_kbit_rate(status),
        audioformat ? audioformat->sample_rate : 0, 
        audioformat ? audioformat->bits : 0, 
        audioformat ? audioformat->channels : 0,
        mpd_status_get_queue_length(status),
        mpd_status_get_next_song_pos(status),
        mpd_status_get_next_song_id(status),
        *last_song_id ? *last_song_id : -1,
        mpd_status_get_queue_version(status)
    );
    
    len += json_printf(&out, "}}");
    
    *current_song_id = song_id;
    *next_song_id = mpd_status_get_next_song_id(status);
    *queue_version = mpd_status_get_queue_version(status);
    *queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);

    CHECK_RETURN_LEN();
    return len;
}

int mympd_put_volume(char *buffer) {
    struct mpd_status *status;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    status = mpd_run_status(mpd.conn);
    if (!status) {
        printf("MPD mpd_run_status: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return 0;
    }
    len = json_printf(&out, "{type: update_volume, data: {"
        "volume: %d}}",
        mpd_status_get_volume(status)
    );
    mpd_status_free(status);

    CHECK_RETURN_LEN();
    return len;
}

int mympd_put_welcome(char *buffer) {
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out, "{type: welcome, data: {version: %Q}}", MYMPD_VERSION);
    
    CHECK_RETURN_LEN();
    return len;
}

bool mympd_state_get(char *name, char *value) {
    char cfgfile[400];
    char *line;
    size_t n = 0;
    ssize_t read;
    
    snprintf(cfgfile, 400, "%s/state/%s", config.varlibdir, name);
    FILE *fp = fopen(cfgfile, "r");
    if (fp == NULL) {
        printf("Error opening %s\n", cfgfile);
        return false;
    }
    read = getline(&line, &n, fp);
    snprintf(value, 400, "%s", line);
    #ifdef DEBUG
    printf("State %s: %s\n", name, value);
    #endif
    fclose(fp);
    if (read > 0)
        return true;
    else
        return false;
}

bool mympd_state_set(const char *name, const char *value) {
    char tmpfile[400];
    char cfgfile[400];
    snprintf(cfgfile, 400, "%s/state/%s", config.varlibdir, name);
    snprintf(tmpfile, 400, "%s/tmp/%s", config.varlibdir, name);
        
    FILE *fp = fopen(tmpfile, "w");
    if (fp == NULL) {
        printf("Error opening %s\n", tmpfile);
        return false;
    }
    fprintf(fp, value);
    fclose(fp);
    rename(tmpfile, cfgfile);
    return true;
}

int mympd_syscmd(char *buffer, char *cmd, int order) {
    int len;
    char filename[400];
    char *line;
    size_t n = 0;
    ssize_t read;
    
    snprintf(filename, 400, "%s/syscmds/%d%s", config.etcdir, order, cmd);
    FILE *fp = fopen(filename, "r");    
    if (fp == NULL) {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't execute cmd %s\"}", cmd);
        return len;
    }
    read = getline(&line, &n, fp);
    fclose(fp);
    if (read > 0) {
        strtok(line, "\n");
        system(line);
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Executed cmd %s\"}", line);
    } else {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't execute cmd %s\"}", cmd);
    }
    CHECK_RETURN_LEN();    
    return len;
}

int mympd_put_settings(char *buffer) {
    struct mpd_status *status;
    char *replaygain = strdup("");
    int len;
    int nr = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    status = mpd_run_status(mpd.conn);
    if (!status) {
        printf("MPD mpd_run_status: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return 0;
    }

    if (!mpd_send_command(mpd.conn, "replay_gain_status", NULL))
        RETURN_ERROR_AND_RECOVER("replay_gain_status");
    struct mpd_pair *pair;
    while ((pair = mpd_recv_pair(mpd.conn)) != NULL) {
        replaygain = strdup(pair->value);
	mpd_return_pair(mpd.conn, pair);
    }
    
    len = json_printf(&out, "{type: settings, data: {"
        "repeat: %d, single: %d, crossfade: %d, consume: %d, random: %d, "
        "mixrampdb: %f, mixrampdelay: %f, mpdhost: %Q, mpdport: %d, passwort_set: %B, featSyscmds: %B, featPlaylists: %B, featTags: %B, "
        "localplayer: %B, streamport: %d, streamurl: %Q, coverimage: %Q, featStickers: %B, mixramp: %B, featSmartpls: %B, maxElementsPerPage: %d, "
        "replaygain: %Q, notificationWeb: %B, notificationPage: %B, jukeboxMode: %d, jukeboxPlaylist: %Q, jukeboxQueueLength: %d, "
        "tags: [", 
        mpd_status_get_repeat(status),
        mpd_status_get_single(status),
        mpd_status_get_crossfade(status),
        mpd_status_get_consume(status),
        mpd_status_get_random(status),
        mpd_status_get_mixrampdb(status),
        mpd_status_get_mixrampdelay(status),
        config.mpdhost, 
        config.mpdport, 
        config.mpdpass ? "true" : "false",
        config.syscmds,
        mpd.feat_playlists,
        mpd.feat_tags,
        config.localplayer,
        config.streamport,
        config.streamurl,
        config.coverimage,
        config.stickers,
        config.mixramp,
        config.smartpls,
        config.max_elements_per_page,
        replaygain,
        mympd_state.notificationWeb,
        mympd_state.notificationPage,
        mympd_state.jukeboxMode,
        mympd_state.jukeboxPlaylist,
        mympd_state.jukeboxQueueLength
    );
    mpd_status_free(status);
    free(replaygain);
    
    nr = 0;
    struct node *current = mympd_tags.list;
    while (current != NULL) {
        if (nr++) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", current->data);
        current = current->next;
    }
    len += json_printf(&out, "], searchtags: [");
    nr = 0;
    current = mympd_searchtags.list;
    while (current != NULL) {
        if (nr++) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", current->data);
        current = current->next;
    }
    len += json_printf(&out, "], browsetags: [");
    nr = 0;
    current = mympd_browsetags.list;
    while (current != NULL) {
        if (nr++) 
            len += json_printf(&out, ",");
        len += json_printf(&out, "%Q", current->data);
        current = current->next;
    }
    len += json_printf(&out, "]");
    
    if (config.syscmds == true) {
        len += json_printf(&out, ", syscmds: [");
        nr = 0;
        current = syscmds.list;
        while (current != NULL) {
            if (nr++) 
                len += json_printf(&out, ",");
            len += json_printf(&out, "%Q", current->data);
            current = current->next;
        }
        len += json_printf(&out, "]");
    }
    len += json_printf(&out, ", colsQueue: %s", mympd_state.colsQueue);
    len += json_printf(&out, ", colsSearch: %s", mympd_state.colsSearch);
    len += json_printf(&out, ", colsBrowseDatabase: %s", mympd_state.colsBrowseFilesystem);
    len += json_printf(&out, ", colsBrowsePlaylistsDetail: %s", mympd_state.colsBrowsePlaylistsDetail);
    len += json_printf(&out, ", colsBrowseFilesystem: %s", mympd_state.colsBrowseFilesystem);
    len += json_printf(&out, "}}");    

    CHECK_RETURN_LEN();
    return len;
}


int mympd_put_outputs(char *buffer) {
    struct mpd_output *output;
    int len;
    int nr;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    if (!mpd_send_outputs(mpd.conn))
        RETURN_ERROR_AND_RECOVER("outputs");

    len = json_printf(&out, "{type: outputs, data: {outputs: [");
    nr = 0;    
    while ((output = mpd_recv_output(mpd.conn)) != NULL) {
        if (nr++) 
            len += json_printf(&out, ",");
        len += json_printf(&out,"{id: %d, name: %Q, state: %d}",
            mpd_output_get_id(output),
            mpd_output_get_name(output),
            mpd_output_get_enabled(output)
        );
        mpd_output_free(output);
    }
    if (!mpd_response_finish(mpd.conn))
        LOG_ERROR_AND_RECOVER("outputs");

    len += json_printf(&out,"]}}");
    
    CHECK_RETURN_LEN();
    return len;
}

int replacechar(char *str, char orig, char rep) {
    char *ix = str;
    int n = 0;
    while ((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}

int mympd_get_cover(const char *uri, char *cover, int cover_len) {
    char *path = strdup(uri);
    int len;
    if (strncasecmp("http:", path, 5) == 0 || strncasecmp("https:", path, 6) == 0) {
        if(strlen(path) > 8) {
            if (strncasecmp("http:", path, 5) == 0)
                path += 7;
            else if (strncasecmp("https:", path, 6) == 0)
                path += 8;
            replacechar(path, '/', '_');
            replacechar(path, '.', '_');
            snprintf(cover, cover_len, "%s/pics/%s.png", SRC_PATH, path);
            if (access(cover, F_OK ) == -1 ) {
                len = snprintf(cover, cover_len, "/assets/coverimage-httpstream.png");
            } else {
                len = snprintf(cover, cover_len, "/pics/%s.png", path);
            }
        } else {
            len = snprintf(cover, cover_len, "/assets/coverimage-httpstream.png");
        }
    }
    else {
        dirname(path);
        snprintf(cover, cover_len, "%s/library/%s/%s", SRC_PATH, path, config.coverimage);
        if (access(cover, F_OK ) == -1 ) {
            len = snprintf(cover, cover_len, "/assets/coverimage-notavailable.png");
        } else {
            len = snprintf(cover, cover_len, "/library/%s/%s", path, config.coverimage);
        }
    }
    free(path);
    return len;
}

int mympd_put_current_song(char *buffer) {
    struct mpd_song *song;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    char cover[500];
    
    song = mpd_run_current_song(mpd.conn);
    if (song == NULL) {
        len = json_printf(&out, "{type: result, data: ok}");
        return len;
    }
        
    mympd_get_cover(mpd_song_get_uri(song), cover, 500);
    
    len = json_printf(&out, "{type: song_change, data: {pos: %d, currentSongId: %d, cover: %Q, ",
        mpd_song_get_pos(song),
        mpd.song_id,
        cover
    );
    if (mpd.feat_tags == true)
        PUT_SONG_TAGS();
    else
        PUT_MIN_SONG_TAGS();

    mpd_response_finish(mpd.conn);
    
    if (config.stickers) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        mympd_get_sticker(mpd_song_get_uri(song), sticker);
        len += json_printf(&out, ", playCount: %d, skipCount: %d, like: %d, lastPlayed: %d",
            sticker->playCount,
            sticker->skipCount,
            sticker->like,
            sticker->lastPlayed
        );
        free(sticker);
    }
    len += json_printf(&out, "}}");
    mpd_song_free(song);

    CHECK_RETURN_LEN();
    return len;
}

int mympd_put_songdetails(char *buffer, char *uri) {
    struct mpd_entity *entity;
    const struct mpd_song *song;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    char cover[500];
    
    len = json_printf(&out, "{type: song_details, data: {");
    if (!mpd_send_list_all_meta(mpd.conn, uri))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_all_meta");
    if ((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        song = mpd_entity_get_song(entity);
        mympd_get_cover(uri, cover, 500);
        len += json_printf(&out, "cover: %Q, ", cover);
        if (mpd.feat_tags == true)
            PUT_SONG_TAGS();
        else
            PUT_MIN_SONG_TAGS();
        mpd_entity_free(entity);
    }
    mpd_response_finish(mpd.conn);

    if (config.stickers) {
        t_sticker *sticker = (t_sticker *) malloc(sizeof(t_sticker));
        mympd_get_sticker(uri, sticker);
        len += json_printf(&out, ", playCount: %d, skipCount: %d, like: %d, lastPlayed: %d",
            sticker->playCount,
            sticker->skipCount,
            sticker->like,
            sticker->lastPlayed
        );
        free(sticker);
    }
    len += json_printf(&out, "}}");
    
    CHECK_RETURN_LEN();
    return len;
}

int mympd_put_queue(char *buffer, unsigned int offset, unsigned *queue_version, unsigned *queue_length) {
    struct mpd_entity *entity;
    unsigned long totalTime = 0;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    struct mpd_status *status = mpd_run_status(mpd.conn);
    
    if (!status)
        RETURN_ERROR_AND_RECOVER("mpd_run_status");
        
    if (!mpd_send_list_queue_range_meta(mpd.conn, offset, offset + config.max_elements_per_page))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_queue_meta");
        
    len = json_printf(&out, "{type: queue, data: [");

    while ((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        const struct mpd_song *song;
        unsigned int drtn;
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            song = mpd_entity_get_song(entity);
            drtn = mpd_song_get_duration(song);
            totalTime += drtn;
            entity_count++;
            if (entities_returned++) 
                len += json_printf(&out, ",");
            len += json_printf(&out, "{id: %d, Pos: %d, ",
                mpd_song_get_id(song),
                mpd_song_get_pos(song)
            );
            if (mpd.feat_tags == true)
                PUT_SONG_TAGS();
            else
                PUT_MIN_SONG_TAGS();
            len += json_printf(&out, "}");
        }
        mpd_entity_free(entity);
    }

    len += json_printf(&out, "], totalTime: %d, totalEntities: %d, offset: %d, returnedEntities: %d, queue_version: %d}",
        totalTime,
        mpd_status_get_queue_length(status),
        offset,
        entities_returned,
        mpd_status_get_queue_version(status)
    );

    *queue_version = mpd_status_get_queue_version(status);
    *queue_length = mpd_status_get_queue_length(status);
    mpd_status_free(status);
    
    CHECK_RETURN_LEN();
    return len;
}

int mympd_put_browse(char *buffer, char *path, unsigned int offset, char *filter) {
    struct mpd_entity *entity;
    const struct mpd_playlist *pl;
    unsigned int entity_count = 0;
    unsigned int entities_returned = 0;
    const char *entityName;
    char smartpls_file[400];
    bool smartpls;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (!mpd_send_list_meta(mpd.conn, path))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");

    len = json_printf(&out, "{type: browse, data: [");

    while ((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        const struct mpd_song *song;
        const struct mpd_directory *dir;
        entity_count++;
        if (entity_count > offset && entity_count <= offset + config.max_elements_per_page) {
            switch (mpd_entity_get_type(entity)) {
                case MPD_ENTITY_TYPE_UNKNOWN:
                    entity_count--;
                    break;
                case MPD_ENTITY_TYPE_SONG:
                    song = mpd_entity_get_song(entity);
                    entityName = mympd_get_tag(song, MPD_TAG_TITLE);
                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, entityName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*entityName) == 0 )
                    ) {
                        if (entities_returned++) 
                            len += json_printf(&out, ",");
                        len += json_printf(&out, "{Type: song, ");
                        if (mpd.feat_tags == true)
                            PUT_SONG_TAGS();
                        else
                            PUT_MIN_SONG_TAGS();

                        len += json_printf(&out, "}");
                    } else {
                        entity_count--;
                    }
                    break;

                case MPD_ENTITY_TYPE_DIRECTORY:
                    dir = mpd_entity_get_directory(entity);                
                    entityName = mpd_directory_get_path(dir);
                    char *dirName = strrchr(entityName, '/');

                    if (dirName != NULL)
                        dirName++;
                    else 
                        dirName = strdup(entityName);

                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, dirName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*dirName) == 0 )
                    ) {                
                        if (entities_returned++) 
                            len += json_printf(&out, ",");
                        len += json_printf(&out, "{Type: dir, uri: %Q, name: %Q}",
                            entityName,
                            dirName
                        );
                    } else {
                        entity_count--;
                    }
                    break;
                    
                case MPD_ENTITY_TYPE_PLAYLIST:
                    pl = mpd_entity_get_playlist(entity);
                    entityName = mpd_playlist_get_path(pl);
                    char *plName = strrchr(entityName, '/');
                    if (plName != NULL) {
                        plName++;
                    } else {
                        plName = strdup(entityName);
                    }
                    if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, plName, 1) == 0 ||
                        (strncmp(filter, "0", 1) == 0 && isalpha(*plName) == 0 )
                    ) {
                        if (entities_returned++) 
                            len += json_printf(&out, ",");
                        snprintf(smartpls_file, 400, "%s/smartpls/%s", config.varlibdir, plName);
                        if (access(smartpls_file, F_OK ) != -1)
                            smartpls = true;
                        else
                            smartpls = false;                    
                        len += json_printf(&out, "{Type: %Q, uri: %Q, name: %Q}",
                            (smartpls == true ? "smartpls" : "plist"),
                            entityName,
                            plName
                        );
                    } else {
                        entity_count--;
                    }
                    break;
            }
        }
        mpd_entity_free(entity);
    }

    if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS || !mpd_response_finish(mpd.conn)) {
        printf("MPD mpd_send_list_meta: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return 0;
    }

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, filter: %Q}",
        entity_count,
        offset,
        entities_returned,
        filter
    );

    CHECK_RETURN_LEN();
    return len;
}

int mympd_put_db_tag(char *buffer, unsigned int offset, char *mpdtagtype, char *mpdsearchtagtype, char *searchstr, char *filter) {
    struct mpd_pair *pair;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_db_tags(mpd.conn, mpd_tag_name_parse(mpdtagtype)) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_db_tags");

    if (mpd_tag_name_parse(mpdsearchtagtype) != MPD_TAG_UNKNOWN) {
        if (mpd_search_add_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdsearchtagtype), searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
    }

    if (mpd_search_commit(mpd.conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");

    len = json_printf(&out, "{type: listDBtags, data: [");
    while ((pair = mpd_recv_pair_tag(mpd.conn, mpd_tag_name_parse(mpdtagtype))) != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + config.max_elements_per_page) {
            if (strcmp(pair->value, "") == 0) {
                entity_count--;
            }
            else if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, pair->value, 1) == 0 ||
                    (strncmp(filter, "0", 1) == 0 && isalpha(*pair->value) == 0 )
            ) {
                if (entities_returned++) 
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{type: %Q, value: %Q}",
                    mpdtagtype,
                    pair->value    
                );
            } else {
                entity_count--;
            }
        }
        mpd_return_pair(mpd.conn, pair);
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
    return len;
}

int mympd_put_songs_in_album(char *buffer, char *album, char *search, char *tag) {
    struct mpd_song *song;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;
    int len;
    char cover[500];
    char *albumartist;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_db_songs(mpd.conn, true) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_db_songs");
    
    if (mpd_search_add_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(tag), search) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");

    if (mpd_search_add_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
        
    if (mpd_search_commit(mpd.conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    else {
        len = json_printf(&out, "{type: listTitles, data: [");

        while ((song = mpd_recv_song(mpd.conn)) != NULL) {
            entity_count++;
            if (entity_count <= config.max_elements_per_page) {
                if (entities_returned++) 
                    len += json_printf(&out, ", ");
                else {
                    mympd_get_cover(mpd_song_get_uri(song), cover, 500);
                    albumartist = strdup(mympd_get_tag(song, MPD_TAG_ALBUM_ARTIST));
                }
                len += json_printf(&out, "{type: song, uri: %Q, Duration: %d, Title: %Q, Track: %Q}",
                    mpd_song_get_uri(song),
                    mpd_song_get_duration(song),
                    mympd_get_tag(song, MPD_TAG_TITLE),
                    mympd_get_tag(song, MPD_TAG_TRACK)
                );
            }
            mpd_song_free(song);
        }
        
        len += json_printf(&out, "], totalEntities: %d, returnedEntities: %d, Album: %Q, search: %Q, tag: %Q, cover: %Q, AlbumArtist: %Q}",
            entity_count,
            entities_returned,
            album,
            search,
            tag,
            cover,
            albumartist
        );
    }
    free(albumartist);

    CHECK_RETURN_LEN();
    return len;
}

int mympd_put_playlists(char *buffer, unsigned int offset, char *filter) {
    struct mpd_playlist *pl;
    unsigned int entity_count = 0;
    unsigned int entities_returned = 0;
    const char *plpath;
    int len;
    bool smartpls;
    char smartpls_file[400];
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (!mpd_send_list_playlists(mpd.conn))
        RETURN_ERROR_AND_RECOVER("mpd_send_lists_playlists");

    len = json_printf(&out, "{type: playlists, data: [");

    while ((pl = mpd_recv_playlist(mpd.conn)) != NULL) {
        entity_count++;
        if (entity_count > offset && entity_count <= offset + config.max_elements_per_page) {
            plpath = mpd_playlist_get_path(pl);
            if (strncmp(filter,"-",1) == 0 || strncasecmp(filter, plpath, 1) == 0 ||
                    (strncmp(filter, "0", 1) == 0 && isalpha(*plpath) == 0 )
            ) {
                if (entities_returned++)
                    len += json_printf(&out, ", ");
                snprintf(smartpls_file, 400, "%s/smartpls/%s", config.varlibdir, plpath);
                if (access(smartpls_file, F_OK ) != -1)
                    smartpls = true;
                else
                    smartpls = false;
                len += json_printf(&out, "{Type: %Q, uri: %Q, name: %Q, last_modified: %d}",
                    (smartpls == true ? "smartpls" : "plist"), 
                    plpath,
                    plpath,
                    mpd_playlist_get_last_modified(pl)
                );
            } else {
                entity_count--;
            }
        }
        mpd_playlist_free(pl);
    }

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d}",
        entity_count,
        offset,
        entities_returned
    );

    CHECK_RETURN_LEN();
    return len;
}

int mympd_put_playlist_list(char *buffer, char *uri, unsigned int offset, char *filter) {
    struct mpd_entity *entity;
    unsigned int entity_count = 0;
    unsigned int entities_returned = 0;
    const char *entityName;
    char smartpls_file[400];
    bool smartpls;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (!mpd_send_list_playlist_meta(mpd.conn, uri))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");

    len = json_printf(&out, "{type: playlist_detail, data: [");

    while ((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        const struct mpd_song *song;
        entity_count++;
        if (entity_count > offset && entity_count <= offset + config.max_elements_per_page) {
            song = mpd_entity_get_song(entity);
            entityName = mympd_get_tag(song, MPD_TAG_TITLE);
            if (strncmp(filter, "-", 1) == 0 || strncasecmp(filter, entityName, 1) == 0 ||
               (strncmp(filter, "0", 1) == 0 && isalpha(*entityName) == 0 )
            ) {
                if (entities_returned++) 
                    len += json_printf(&out, ",");
                len += json_printf(&out, "{Type: song, ");
                if (mpd.feat_tags == true)
                    PUT_SONG_TAGS();
                else
                    PUT_MIN_SONG_TAGS();
                len += json_printf(&out, ", Pos: %d", entity_count);
                len += json_printf(&out, "}");
            } else {
                entity_count--;
            }
        }
        mpd_entity_free(entity);
    }
    snprintf(smartpls_file, 400, "%s/smartpls/%s", config.varlibdir, uri);
    if (access(smartpls_file, F_OK ) != -1)
        smartpls = true;
    else
        smartpls = false;
    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, filter: %Q, uri: %Q, smartpls: %B}",
        entity_count,
        offset,
        entities_returned,
        filter,
        uri,
        smartpls
    );

    CHECK_RETURN_LEN();
    return len;
}

int mympd_search(char *buffer, char *searchstr, char *filter, char *plist, unsigned int offset) {
    struct mpd_song *song;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;
    int len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    if (strcmp(plist, "") == 0) {
        if (mpd_send_command(mpd.conn, "search", filter, searchstr, NULL) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search");
        len = json_printf(&out, "{type: search, data: [");
    }
    else if (strcmp(plist, "queue") == 0) {
        if (mpd_send_command(mpd.conn, "searchadd", filter, searchstr, NULL) == false)
            RETURN_ERROR_AND_RECOVER("mpd_searchadd");
    }
    else {
        if (mpd_send_command(mpd.conn, "searchaddpl", plist, filter, searchstr, NULL) == false)
            RETURN_ERROR_AND_RECOVER("mpd_searchaddpl");
    }

    if (strcmp(plist, "") == 0) {
        while ((song = mpd_recv_song(mpd.conn)) != NULL) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + config.max_elements_per_page) {
                if (entities_returned++) 
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{Type: song, ");
                if (mpd.feat_tags == true)
                    PUT_SONG_TAGS();
                else
                    PUT_MIN_SONG_TAGS();
                len += json_printf(&out, "}");
            }
            mpd_song_free(song);
        }
    }
    else
        mpd_response_finish(mpd.conn);

    if (strcmp(plist, "") == 0) {
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, searchstr: %Q}",
            entity_count,
            offset,
            entities_returned,
            searchstr
        );
    } 
    else
        len = json_printf(&out, "{type: result, data: ok}");

    CHECK_RETURN_LEN();
    return len;
}

int mympd_queue_crop(char *buffer) {
    int len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    struct mpd_status *status = mpd_run_status(mpd.conn);
    int length = mpd_status_get_queue_length(status) - 1;
    int playing_song_pos = mpd_status_get_song_pos(status);

    if (length < 0) {
        len = json_printf(&out, "{type: error, data: %Q}", "A playlist longer than 1 song in length is required to crop.");
    }
    else if (mpd_status_get_state(status) == MPD_STATE_PLAY || mpd_status_get_state(status) == MPD_STATE_PAUSE) {
        playing_song_pos++;
        if (playing_song_pos < length)
            mpd_run_delete_range(mpd.conn, playing_song_pos, -1);
        playing_song_pos--;
        if (playing_song_pos > 0 )
            mpd_run_delete_range(mpd.conn, 0, playing_song_pos--);            
        len = json_printf(&out, "{type: result, data: ok}");
    } else {
        len = json_printf(&out, "{type: error, data: %Q}", "You need to be playing to crop the playlist");
    }
    
    mpd_status_free(status);
    
    CHECK_RETURN_LEN();
    return len;    
}

int mympd_search_queue(char *buffer, char *mpdtagtype, unsigned int offset, char *searchstr) {
    struct mpd_song *song;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_queue_songs(mpd.conn, false) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_search_queue_songs");
    }
    
    if (mpd_tag_name_parse(mpdtagtype) != MPD_TAG_UNKNOWN) {
       if (mpd_search_add_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdtagtype), searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
    }
    else {
        if (mpd_search_add_any_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_any_tag_constraint");        
    }

    if (mpd_search_commit(mpd.conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    else {
        len = json_printf(&out, "{type: queuesearch, data: [");

        while ((song = mpd_recv_song(mpd.conn)) != NULL) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + config.max_elements_per_page) {
                if (entities_returned++)
                    len += json_printf(&out, ", ");
                len += json_printf(&out, "{type: song, id: %d, pos: %d, ",
                    mpd_song_get_id(song),
                    mpd_song_get_pos(song)
                );
                if (mpd.feat_tags == true)
                    PUT_SONG_TAGS();
                else
                    PUT_MIN_SONG_TAGS();
                len += json_printf(&out, "}");
                mpd_song_free(song);
            }
        }
        
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, mpdtagtype: %Q}",
            entity_count,
            offset,
            entities_returned,
            mpdtagtype
        );
    }

    CHECK_RETURN_LEN();
    return len;
}

int mympd_put_stats(char *buffer) {
    struct mpd_stats *stats = mpd_run_stats(mpd.conn);
    const unsigned *version = mpd_connection_get_server_version(mpd.conn);
    int len;
    char mpd_version[20];
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    snprintf(mpd_version, 20, "%i.%i.%i", version[0], version[1], version[2]);
    
    if (stats == NULL)
        RETURN_ERROR_AND_RECOVER("mympd_put_stats");
    len = json_printf(&out, "{type: mpdstats, data: {artists: %d, albums: %d, songs: %d, "
        "playtime: %d, uptime: %d, dbUpdated: %d, dbPlaytime: %d, mympdVersion: %Q, mpdVersion: %Q}}",
        mpd_stats_get_number_of_artists(stats),
        mpd_stats_get_number_of_albums(stats),
        mpd_stats_get_number_of_songs(stats),
        mpd_stats_get_play_time(stats),
        mpd_stats_get_uptime(stats),
        mpd_stats_get_db_update_time(stats),
        mpd_stats_get_db_play_time(stats),
        MYMPD_VERSION,
        mpd_version
    );
    mpd_stats_free(stats);

    CHECK_RETURN_LEN();
    return len;
}

void mympd_disconnect() {
    mpd.conn_state = MPD_DISCONNECT;
    mympd_idle(NULL, 0);
}

int mympd_smartpls_put(char *buffer, char *playlist) {
    char pl_file[400];
    char *smartpltype;
    char *p_charbuf1, *p_charbuf2;
    int je, int_buf1;
    int len = 0;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    snprintf(pl_file, 400, "%s/smartpls/%s", config.varlibdir, playlist);
    char *content = json_fread(pl_file);
    je = json_scanf(content, strlen(content), "{type: %Q }", &smartpltype);
    if (je == 1) {
        if (strcmp(smartpltype, "sticker") == 0) {
            je = json_scanf(content, strlen(content), "{sticker: %Q, maxentries: %d}", &p_charbuf1, &int_buf1);
            if (je == 2) {
                len = json_printf(&out, "{type: smartpls, data: {playlist: %Q, type: %Q, sticker: %Q, maxentries: %d}}",
                    playlist,
                    smartpltype,
                    p_charbuf1,
                    int_buf1);
                free(p_charbuf1);
            }
        }
        else if (strcmp(smartpltype, "newest") == 0) {
            je = json_scanf(content, strlen(content), "{timerange: %d}", &int_buf1);
            if (je == 1) {
                len = json_printf(&out, "{type: smartpls, data: {playlist: %Q, type: %Q, timerange: %d}}",
                    playlist,
                    smartpltype,
                    int_buf1);
            }
        }
        else if (strcmp(smartpltype, "search") == 0) {
            je = json_scanf(content, strlen(content), "{tag: %Q, searchstr: %Q}", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                len = json_printf(&out, "{type: smartpls, data: {playlist: %Q, type: %Q, tag: %Q, searchstr: %Q}}",
                    playlist,
                    smartpltype,
                    p_charbuf1,
                    p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
            }
        }
        free(smartpltype);        
    }
    return len;
}

int mympd_smartpls_save(char *smartpltype, char *playlist, char *tag, char *searchstr, int maxentries, int timerange) {
    char tmp_file[400];
    char pl_file[400];
    snprintf(tmp_file, 400, "%s/tmp/%s", config.varlibdir, playlist);
    snprintf(pl_file, 400, "%s/smartpls/%s", config.varlibdir, playlist);
    if (strcmp(smartpltype, "sticker") == 0) {
        json_fprintf(tmp_file, "{type: %Q, sticker: %Q, maxentries: %d}", smartpltype, tag, maxentries);
        rename(tmp_file, pl_file);
        mympd_smartpls_update(playlist, tag, maxentries);
    }
    else if (strcmp(smartpltype, "newest") == 0) {
        json_fprintf(tmp_file, "{type: %Q, timerange: %d}", smartpltype, timerange);
        rename(tmp_file, pl_file);
        mympd_smartpls_update_newest(playlist, timerange);
    }
    else if (strcmp(smartpltype, "search") == 0) {
        json_fprintf(tmp_file, "{type: %Q, tag: %Q, searchstr: %Q}", smartpltype, tag, searchstr);
        rename(tmp_file, pl_file);
        mympd_smartpls_update_search(playlist, tag, searchstr);
    }
    return 0;
}

int mympd_smartpls_update_all() {
    DIR *dir;
    struct dirent *ent;
    char *smartpltype;
    char filename[400];
    char dirname[400];
    int je;
    char *p_charbuf1, *p_charbuf2;
    int int_buf1;
    
    if (!config.smartpls)
        return 0;
    
    snprintf(dirname, 400, "%s/smartpls", config.varlibdir);
    if ((dir = opendir (dirname)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, ".", 1) == 0)
                continue;
            snprintf(filename, 400, "%s/smartpls/%s", config.varlibdir, ent->d_name);
            char *content = json_fread(filename);
            je = json_scanf(content, strlen(content), "{type: %Q }", &smartpltype);
            if (je != 1)
                continue;
            if (strcmp(smartpltype, "sticker") == 0) {
                je = json_scanf(content, strlen(content), "{sticker: %Q, maxentries: %d}", &p_charbuf1, &int_buf1);
                if (je == 2) {
                    mympd_smartpls_update(ent->d_name, p_charbuf1, int_buf1);
                    free(p_charbuf1);
                }
                else
                    printf("Can't parse file %s\n", filename);
            }
            else if (strcmp(smartpltype, "newest") == 0) {
                je = json_scanf(content, strlen(content), "{timerange: %d}", &int_buf1);
                if (je == 1) {
                    mympd_smartpls_update_newest(ent->d_name, int_buf1);
                }
                else
                    printf("Can't parse file %s\n", filename);                
            }
            else if (strcmp(smartpltype, "search") == 0) {
                je = json_scanf(content, strlen(content), "{tag: %Q, searchstr: %Q}", &p_charbuf1, &p_charbuf2);
                if (je == 2) {
                    mympd_smartpls_update_search(ent->d_name, p_charbuf1, p_charbuf2);
                    free(p_charbuf1);
                    free(p_charbuf2);
                }
                else
                    printf("Can't parse file %s\n", filename);
            }
            free(smartpltype);
            free(content);
        }
        closedir (dir);
    } else {
        printf("Can't open dir %s\n", dirname);
        return 1;
    }
    return 0;
}

int mympd_smartpls_clear(char *playlist) {
    struct mpd_playlist *pl;
    const char *plpath;
    bool exists = false;
    if (!mpd_send_list_playlists(mpd.conn)) {
        LOG_ERROR_AND_RECOVER("mpd_send_list_playlists");
        return 1;
    }
    while ((pl = mpd_recv_playlist(mpd.conn)) != NULL) {
        plpath = mpd_playlist_get_path(pl);
        if (strcmp(playlist, plpath) == 0)
            exists = true;
        mpd_playlist_free(pl);
        if (exists)
            break;
    }
    mpd_response_finish(mpd.conn);
    
    if (exists) {
        if (!mpd_run_rm(mpd.conn, playlist)) {
            LOG_ERROR_AND_RECOVER("mpd_run_rm");
            return 1;
        }
    }
    return 0;
}

int mympd_smartpls_update_search(char *playlist, char *tag, char *searchstr) {
    mympd_smartpls_clear(playlist);
    mympd_search(mpd.buf, searchstr, tag, playlist, 0);
    printf("Updated %s\n", playlist);
    return 0;
}

int mympd_smartpls_update(char *playlist, char *sticker, int maxentries) {
    struct mpd_pair *pair;
    char *uri = NULL;
    const char *p_value;
    char *crap;
    long value;
    long value_max = 0;
    long i = 0;
    unsigned int j;

    if (!mpd_send_sticker_find(mpd.conn, "song", "", sticker)) {
        LOG_ERROR_AND_RECOVER("mpd_send_sticker_find");
        return 1;    
    }

    struct list add_list;
    list_init(&add_list);

    while ((pair = mpd_recv_pair(mpd.conn)) != NULL) {
        if (strcmp(pair->name, "file") == 0) {
            uri = strdup(pair->value);
        } 
        else if (strcmp(pair->name, "sticker") == 0) {
            p_value = mpd_parse_sticker(pair->value, &j);
            if (p_value != NULL) {
                value = strtol(p_value, &crap, 10);
                if (value > 1)
                    list_push(&add_list, uri, value);
                if (value > value_max)
                    value_max = value;
            }
        }
        mpd_return_pair(mpd.conn, pair);
    }
    mpd_response_finish(mpd.conn);
    free(uri);

    mympd_smartpls_clear(playlist);
     
    if (value_max > 2)
        value_max = value_max / 2;

    struct node *current = add_list.list;
    while (current != NULL) {
        if (current->value >= value_max) {
            if (!mpd_run_playlist_add(mpd.conn, playlist, current->data)) {
                LOG_ERROR_AND_RECOVER("mpd_run_playlist_add");
                list_free(&add_list);
                return 1;        
            }
            i++;
            if (i >= maxentries)
                break;
        }
        current = current->next;
    }
    list_free(&add_list);
    printf("Updated %s with %ld songs, minValue: %ld\n", playlist, i, value_max);
    return 0;
}

int mympd_smartpls_update_newest(char *playlist, int timerange) {
    unsigned long value_max = 0;
    char searchstr[20];
    
    struct mpd_stats *stats = mpd_run_stats(mpd.conn);
    if (stats != NULL) {
        value_max = mpd_stats_get_db_update_time(stats);
        mpd_stats_free(stats);
    }
    else {
        LOG_ERROR_AND_RECOVER("mpd_run_stats");
        return 1;
    }

    mympd_smartpls_clear(playlist);
    value_max -= timerange;
    if (value_max > 0) {
        snprintf(searchstr, 20, "%lu", value_max);
        mympd_search(mpd.buf, searchstr, "modified-since", playlist, 0);
        printf("Updated %s\n", playlist);
    }
    return 0;
}
