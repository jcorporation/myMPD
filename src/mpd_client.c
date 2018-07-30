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
#include <mpd/client.h>
#include <mpd/message.h>

#include "mpd_client.h"
#include "config.h"
#include "../dist/src/frozen/frozen.h"

/* forward declaration */
static int mympd_notify_callback(struct mg_connection *c, const char *param);

const char * mpd_cmd_strs[] = {
    MPD_CMDS(GEN_STR)
};

static inline enum mpd_cmd_ids get_cmd_id(const char *cmd)
{
    for(int i = 0; i < sizeof(mpd_cmd_strs)/sizeof(mpd_cmd_strs[0]); i++)
        if (!strncmp(cmd, mpd_cmd_strs[i], strlen(mpd_cmd_strs[i])))
            return i;

    return -1;
}

void callback_mympd(struct mg_connection *nc, const struct mg_str msg)
{
    size_t n = 0;
    char *cmd;
    unsigned int uint_buf1, uint_buf2, uint_rc;
    int je, int_buf, int_rc; 
    float float_buf;
    char *p_charbuf1, *p_charbuf2;
    struct mympd_state { int a; int b; } state = { .a = 0, .b = 0 };
    enum mpd_cmd_ids cmd_id;
    
    #ifdef DEBUG
    fprintf(stdout,"Got request: %s\n",msg.p);
    #endif
    
    je = json_scanf(msg.p, msg.len, "{cmd:%Q}", &cmd);
    if (je == 1) 
        cmd_id = get_cmd_id(cmd);
    else
        cmd_id = get_cmd_id("MPD_API_UNKNOWN");

    if (cmd_id == -1)
        cmd_id = get_cmd_id("MPD_API_UNKNOWN");
    
    switch(cmd_id) {
        case MPD_API_UNKNOWN:
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown request\"}");
            break;
        case MPD_API_SET_SETTINGS:
            json_scanf(msg.p, msg.len, "{ data: { notificationWeb: %d, notificationPage: %d} }", &state.a, &state.b);
            char tmpfile[200];
            snprintf(tmpfile,200,"%s.tmp", mpd.statefile);
            json_fprintf(tmpfile, "{ notificationWeb: %d, notificationPage: %d}", state.a, state.b);
            rename(tmpfile,mpd.statefile);
            
            je = json_scanf(msg.p, msg.len, "{ data: { random:%u } }", &uint_buf1);
            if (je == 1)        
                mpd_run_random(mpd.conn, uint_buf1);
            
            je = json_scanf(msg.p, msg.len, "{ data: { repeat:%u } }", &uint_buf1);
            if (je == 1)        
                mpd_run_repeat(mpd.conn, uint_buf1);
            
            je = json_scanf(msg.p, msg.len, "{ data: { consume:%u } }", &uint_buf1);
            if (je == 1)        
                mpd_run_consume(mpd.conn, uint_buf1);
            
            je = json_scanf(msg.p, msg.len, "{ data: { single:%u } }", &uint_buf1);
            if (je == 1)
                mpd_run_single(mpd.conn, uint_buf1);
            
            je = json_scanf(msg.p, msg.len, "{ data: { crossfade:%u } }", &uint_buf1);
            if (je == 1)
                mpd_run_crossfade(mpd.conn, uint_buf1);
            
            je = json_scanf(msg.p, msg.len, "{ data: { mixrampdb:%f } }", &float_buf);
            if (je == 1)        
                mpd_run_mixrampdb(mpd.conn, float_buf);
            
            je = json_scanf(msg.p, msg.len, "{ data: { mixrampdelay:%f } }", &float_buf);
            if (je == 1)        
                mpd_run_mixrampdelay(mpd.conn, float_buf);
            
            je = json_scanf(msg.p, msg.len, "{ data: { replaygain:%Q } }", &p_charbuf1);
            if (je == 1) {
                mpd_send_command(mpd.conn, "replay_gain_mode", p_charbuf1, NULL);
                struct mpd_pair *pair;
                while ((pair = mpd_recv_pair(mpd.conn)) != NULL) {
        	    mpd_return_pair(mpd.conn, pair);
                }
                free(p_charbuf1);            
            }
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_GET_ARTISTALBUMTITLES:
            je = json_scanf(msg.p, msg.len, "{ data: { albumartist:%Q, album:%Q } }", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                n = mympd_put_songs_in_album(mpd.buf, p_charbuf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
            } 
            break;
        case MPD_API_WELCOME:
            n = mympd_put_welcome(mpd.buf);
            break;
        case MPD_API_UPDATE_DB:
            uint_rc = mpd_run_update(mpd.conn, NULL);
            if (uint_rc > 0)
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_SET_PAUSE:
            mpd_run_toggle_pause(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_SET_PREV:
            mpd_run_previous(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_SET_NEXT:
            mpd_run_next(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_SET_PLAY:
            mpd_run_play(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_SET_STOP:
            mpd_run_stop(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_RM_ALL:
            mpd_run_clear(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_RM_TRACK:
            je = json_scanf(msg.p, msg.len, "{ data: { track:%u } }", &uint_buf1);
            if (je == 1) {
                mpd_run_delete_id(mpd.conn, uint_buf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_RM_RANGE:
            je = json_scanf(msg.p, msg.len, "{ data: { start:%u, end:%u } }", &uint_buf1, &uint_buf2);
            if (je == 2) {
                mpd_run_delete_range(mpd.conn, uint_buf1, uint_buf2);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_MOVE_TRACK:
            je = json_scanf(msg.p, msg.len, "{ data: { from:%u, to:%u } }", &uint_buf1, &uint_buf2);
            if (je == 2) {
                uint_buf1 --;
                uint_buf2 --;
                if (uint_buf1 < uint_buf2)
                    uint_buf2 --;
                mpd_run_move(mpd.conn, uint_buf1, uint_buf2);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_PLAYLIST_MOVE_TRACK:
            je = json_scanf(msg.p, msg.len, "{ data: { plist: %Q, from:%u, to:%u } }", &p_charbuf1, &uint_buf1, &uint_buf2);
            if (je == 3) {
                uint_buf1 --;
                uint_buf2 --;
                if (uint_buf1 < uint_buf2)
                    uint_buf2 --;
                mpd_send_playlist_move(mpd.conn, p_charbuf1, uint_buf1, uint_buf2);
                mpd_response_finish(mpd.conn);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_PLAY_TRACK:
            je = json_scanf(msg.p, msg.len, "{ data: { track:%u } }", &uint_buf1);
            if (je == 1) {
                mpd_run_play_id(mpd.conn, uint_buf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_GET_OUTPUTNAMES:
            n = mympd_put_outputnames(mpd.buf);
            break;
        case MPD_API_TOGGLE_OUTPUT:
            je = json_scanf(msg.p, msg.len, "{ data: { output:%u, state:%u } }", &uint_buf1, &uint_buf2);
            if (je == 2) {
                if (uint_buf2)
                    mpd_run_enable_output(mpd.conn, uint_buf1);
                else
                    mpd_run_disable_output(mpd.conn, uint_buf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_SET_VOLUME:
            je = json_scanf(msg.p, msg.len, "{ data: { volume:%u } }", &uint_buf1);
            if (je == 1) {
                mpd_run_set_volume(mpd.conn, uint_buf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_SET_SEEK:
            je = json_scanf(msg.p, msg.len, "{ data: { songid:%u, seek:%u } }", &uint_buf1, &uint_buf2);
            if (je == 2) {
                mpd_run_seek_id(mpd.conn, uint_buf1, uint_buf2);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_GET_QUEUE:
            je = json_scanf(msg.p, msg.len, "{ data: { offset:%u } }", &uint_buf1);
            if (je == 1) {
                n = mympd_put_queue(mpd.buf, uint_buf1);
            }
            break;
        case MPD_API_GET_CURRENT_SONG:
                n = mympd_put_current_song(mpd.buf);
            break;
        case MPD_API_GET_SONGDETAILS:
            je = json_scanf(msg.p, msg.len, "{ data: { uri:%Q } }", &p_charbuf1);
            if (je == 1) {
                n = mympd_put_songdetails(mpd.buf, p_charbuf1);
                free(p_charbuf1);
            }
            break;            
        case MPD_API_GET_ARTISTS:
            je = json_scanf(msg.p, msg.len, "{ data: { offset:%u, filter:%Q } }", &uint_buf1, &p_charbuf1);
            if (je == 2) {
                n = mympd_put_db_tag(mpd.buf, uint_buf1, "AlbumArtist", "", "", p_charbuf1);
                free(p_charbuf1);
            }
            break;
        case MPD_API_GET_ARTISTALBUMS:
            je = json_scanf(msg.p, msg.len, "{ data: { offset:%u, filter:%Q, albumartist:%Q } }", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                n = mympd_put_db_tag(mpd.buf, uint_buf1, "Album", "AlbumArtist", p_charbuf2, p_charbuf1);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_PLAYLIST_RENAME:
            je = json_scanf(msg.p, msg.len, "{ data: { from:%Q, to:%Q } }", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                mpd_run_rename(mpd.conn, p_charbuf1, p_charbuf2);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Renamed playlist %s to %s\"}", p_charbuf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);                
            }
            break;            
        case MPD_API_GET_PLAYLISTS:
            je = json_scanf(msg.p, msg.len, "{ data: { offset:%u, filter:%Q } }", &uint_buf1, &p_charbuf1);
            if (je == 2) {
                n = mympd_put_playlists(mpd.buf, uint_buf1, p_charbuf1);
                free(p_charbuf1);
            }
            break;
        case MPD_API_GET_PLAYLIST_LIST:
            je = json_scanf(msg.p, msg.len, "{ data: { uri: %Q, offset:%u, filter:%Q } }", &p_charbuf1, &uint_buf1, &p_charbuf2);
            if (je == 3) {
                n = mympd_put_playlist_list(mpd.buf, p_charbuf1, uint_buf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_ADD_TO_PLAYLIST:
            je = json_scanf(msg.p, msg.len, "{ data: { plist:%Q, uri:%Q } }", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                mpd_run_playlist_add(mpd.conn, p_charbuf1, p_charbuf2);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Added %s to playlist %s\"}", p_charbuf2, p_charbuf1);
                free(p_charbuf1);
                free(p_charbuf2);                
            }
            break;
        case MPD_API_PLAYLIST_CLEAR:
            je = json_scanf(msg.p, msg.len, "{ data: { uri:%Q } }", &p_charbuf1);
            if (je == 1) {
                mpd_run_playlist_clear(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_RM_PLAYLIST_TRACK:
            je = json_scanf(msg.p, msg.len, "{ data: { uri:%Q, track:%u } }", &p_charbuf1, &uint_buf1);
            if (je == 2) {
                mpd_run_playlist_delete(mpd.conn, p_charbuf1, uint_buf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_GET_FILESYSTEM:
            je = json_scanf(msg.p, msg.len, "{ data: { offset:%u, filter:%Q, path:%Q } }", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                n = mympd_put_browse(mpd.buf, p_charbuf2, uint_buf1, p_charbuf1);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_ADD_TRACK_AFTER:
            je = json_scanf(msg.p, msg.len, "{ data: { uri:%Q, to:%d } }", &p_charbuf1, &int_buf);
            if (je == 2) {
                int_rc = mpd_run_add_id_to(mpd.conn, p_charbuf1, int_buf);
                if (int_rc > -1 ) 
                    n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
                free(p_charbuf1);
            }
            break;
        case MPD_API_REPLACE_TRACK:
            je = json_scanf(msg.p, msg.len, "{ data: { uri:%Q } }", &p_charbuf1);
            if (je == 1) {
                mpd_run_clear(mpd.conn);
                mpd_run_add(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                mpd_run_play(mpd.conn);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_ADD_TRACK:
            je = json_scanf(msg.p, msg.len, "{ data: { uri:%Q } }", &p_charbuf1);
            if (je == 1) {
                mpd_run_add(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_ADD_PLAY_TRACK:
            je = json_scanf(msg.p, msg.len, "{ data: { uri:%Q } }", &p_charbuf1);
            if (je == 1) {
                int_buf = mpd_run_add_id(mpd.conn, p_charbuf1);
                if (int_buf != -1)
                    mpd_run_play_id(mpd.conn, int_buf);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_REPLACE_PLAYLIST:
            je = json_scanf(msg.p, msg.len, "{ data: { plist:%Q } }", &p_charbuf1);
            if (je == 1) {
                mpd_run_clear(mpd.conn);
                mpd_run_load(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                mpd_run_play(mpd.conn);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_ADD_PLAYLIST:
            je = json_scanf(msg.p, msg.len, "{ data: { plist:%Q } }", &p_charbuf1);
            if (je == 1) {
                mpd_run_load(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_SAVE_QUEUE:
            je = json_scanf(msg.p, msg.len, "{ data: { plist:%Q } }", &p_charbuf1);
            if (je == 1) {
                mpd_run_save(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_SEARCH_QUEUE:
            je = json_scanf(msg.p, msg.len, "{ data: { offset:%u, mpdtag:%Q, searchstr:%Q } }", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                n = mympd_search_queue(mpd.buf, p_charbuf1, uint_buf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;            
        case MPD_API_SEARCH_ADD:
            je = json_scanf(msg.p, msg.len, "{ data: { filter:%Q, searchstr:%Q } }", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                n = mympd_search_add(mpd.buf, p_charbuf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_SEARCH:
            je = json_scanf(msg.p, msg.len, "{ data: { offset:%u, mpdtag:%Q, searchstr:%Q } }", &uint_buf1, &p_charbuf1, &p_charbuf2);
            if (je == 3) {
                n = mympd_search(mpd.buf, p_charbuf1, uint_buf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
            }
            break;
        case MPD_API_SEND_SHUFFLE:
            mpd_run_shuffle(mpd.conn);
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            break;
        case MPD_API_SEND_MESSAGE:
            je = json_scanf(msg.p, msg.len, "{ data: { channel:%Q, text:%Q } }", &p_charbuf1, &p_charbuf2);
            if (je == 2) {
                mpd_run_send_message(mpd.conn, p_charbuf1, p_charbuf2);
                free(p_charbuf1);
                free(p_charbuf2);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_RM_PLAYLIST:
            je = json_scanf(msg.p, msg.len, "{ data: { uri:%Q } }", &p_charbuf1);
            if (je == 1) {
                mpd_run_rm(mpd.conn, p_charbuf1);
                free(p_charbuf1);
                n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"result\", \"data\": \"ok\"}");
            }
            break;
        case MPD_API_GET_SETTINGS:
            n = mympd_put_settings(mpd.buf);
            break;
        case MPD_API_GET_STATS:
            n = mympd_get_stats(mpd.buf);
        break;
    }

    if (mpd.conn_state == MPD_CONNECTED && mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS) {
        #ifdef DEBUG
        fprintf(stderr,"Error: %s\n", mpd_connection_get_error_message(mpd.conn));
        #endif
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"error\", \"data\": \"%s\"}", 
            mpd_connection_get_error_message(mpd.conn));

        /* Try to recover error */
        if (!mpd_connection_clear_error(mpd.conn))
            mpd.conn_state = MPD_FAILURE;
    }

    if (n == 0)
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\": \"error\", \"data\": \"No response for cmd %s\"}", cmd);
    
    if (is_websocket(nc)) {
        #ifdef DEBUG
        fprintf(stdout,"Send websocket response:\n %s\n",mpd.buf);
        #endif
        mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, mpd.buf, n);
    }
    else {
        #ifdef DEBUG
        fprintf(stdout,"Send http response:\n %s\n",mpd.buf);
        #endif
        mg_send_http_chunk(nc, mpd.buf, n);
    }
    free(cmd);    
}

int mympd_close_handler(struct mg_connection *c) {
    /* Cleanup session data */
    if (c->user_data)
        free(c->user_data);
    return 0;
}

static int mympd_notify_callback(struct mg_connection *c, const char *param) {
    size_t n;
    if (!is_websocket(c))
        return 0;

    if (param) {
        /* error message? */
        n=snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"error\",\"data\":\"%s\"}",param);
        #ifdef DEBUG
        fprintf(stdout,"Error in mpd_notify_callback: %s\n",param);
        #endif
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, n);
        return 0;
    }

    if (!c->user_data)
        c->user_data = calloc(1, sizeof(struct t_mpd_client_session));

    struct t_mpd_client_session *s = (struct t_mpd_client_session *)c->user_data;

    if (mpd.conn_state != MPD_CONNECTED) {
        n=snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"disconnected\"}");
        #ifdef DEBUG
        fprintf(stdout,"Notify: disconnected\n");
        #endif
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, n);
    }
    else {
        #ifdef DEBUG
        fprintf(stdout,"Notify: %s\n",mpd.buf);
        #endif
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, strlen(mpd.buf));
        
        if (s->song_id != mpd.song_id) {
            n=mympd_put_current_song(mpd.buf);
            #ifdef DEBUG
            fprintf(stdout,"Notify: %s\n",mpd.buf);
            #endif
            mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, n);
            s->song_id = mpd.song_id;
        }
        
        if (s->queue_version != mpd.queue_version) {
            n=snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"update_queue\"}");
            #ifdef DEBUG
            fprintf(stdout,"Notify: update_queue\n");
            #endif
            mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, n);
            s->queue_version = mpd.queue_version;
        }
    }
    return 0;
}

void mympd_poll(struct mg_mgr *s) {
    switch (mpd.conn_state) {
        case MPD_DISCONNECTED:
            /* Try to connect */
            fprintf(stdout, "MPD Connecting to %s:%d\n", mpd.host, mpd.port);
            mpd.conn = mpd_connection_new(mpd.host, mpd.port, 3000);
            if (mpd.conn == NULL) {
                fprintf(stderr, "Out of memory.");
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS) {
                fprintf(stderr, "MPD connection: %s\n", mpd_connection_get_error_message(mpd.conn));
                for (struct mg_connection *c = mg_next(s, NULL); c != NULL; c = mg_next(s, c)) {
                    mympd_notify_callback(c, mpd_connection_get_error_message(mpd.conn));
                }
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            if (mpd.password && !mpd_run_password(mpd.conn, mpd.password)) {
                fprintf(stderr, "MPD connection: %s\n", mpd_connection_get_error_message(mpd.conn));
                for (struct mg_connection *c = mg_next(s, NULL); c != NULL; c = mg_next(s, c)) {
                    mympd_notify_callback(c, mpd_connection_get_error_message(mpd.conn));
                }
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            fprintf(stderr, "MPD connected.\n");
            mpd_connection_set_timeout(mpd.conn, 10000);
            mpd.conn_state = MPD_CONNECTED;
            break;

        case MPD_FAILURE:
            fprintf(stderr, "MPD connection failed.\n");

        case MPD_DISCONNECT:
        case MPD_RECONNECT:
            if (mpd.conn != NULL)
                mpd_connection_free(mpd.conn);
            mpd.conn = NULL;
            mpd.conn_state = MPD_DISCONNECTED;
            break;

        case MPD_CONNECTED:
            mpd.buf_size = mympd_put_state(mpd.buf, &mpd.song_id, &mpd.next_song_id, &mpd.queue_version);
            for (struct mg_connection *c = mg_next(s, NULL); c != NULL; c = mg_next(s, c)) {
                mympd_notify_callback(c, NULL);
            }
            break;
    }
}

char* mympd_get_tag(struct mpd_song const *song, enum mpd_tag_type tag) {
    char *str;
    str = (char *)mpd_song_get_tag(song, tag, 0);
    if (str == NULL) {
        if (tag == MPD_TAG_TITLE)
            str = basename((char *)mpd_song_get_uri(song));
        else
            str = "-";
    }
    return str;
}

int mympd_put_state(char *buffer, int *current_song_id, int *next_song_id,  unsigned *queue_version) {
    struct mpd_status *status;
    const struct mpd_audio_format *audioformat;
    struct mpd_output *output;
    int len;
    int nr;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    status = mpd_run_status(mpd.conn);
    if (!status) {
        fprintf(stderr, "MPD mpd_run_status: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return 0;
    }
    if (status) {
     audioformat = mpd_status_get_audio_format(status);
    }
    
    len = json_printf(&out,"{type:state, data:{"
        "state:%d, volume:%d, songpos: %d, elapsedTime: %d, "
        "totalTime:%d, currentsongid: %d, kbitrate: %d, "
        "audioformat: { sample_rate: %d, bits: %d, channels: %d}, "
        "queue_length: %d, nextsongpos: %d, nextsongid: %d, "
        "queue_version: %d", 
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
        mpd_status_get_queue_version(status)
    );
    
    len += json_printf(&out, ",outputs: [");

    mpd_send_outputs(mpd.conn);
    nr=0;
    while ((output = mpd_recv_output(mpd.conn)) != NULL) {
        if (nr++) len += json_printf(&out, ",");
        len += json_printf(&out, "{id: %d, state: %d}",
            mpd_output_get_id(output), 
            mpd_output_get_enabled(output)
        );
        mpd_output_free(output);
    }
    if (!mpd_response_finish(mpd.conn)) {
        fprintf(stderr, "MPD outputs: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd_connection_clear_error(mpd.conn);
    }

    len += json_printf(&out, "]}}");

    *current_song_id = mpd_status_get_song_id(status);
    *next_song_id = mpd_status_get_next_song_id(status);
    *queue_version = mpd_status_get_queue_version(status);
    mpd_status_free(status);

    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_put_welcome(char *buffer) {
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out, "{type: welcome, data: { version: %Q}}", MYMPD_VERSION);
    
    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_put_settings(char *buffer) {
    struct mpd_status *status;
    char *replaygain;
    int len;
    int je;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    struct mympd_state { int a; int b; } state = { .a = 0, .b = 0 };
    if (access( mpd.statefile, F_OK ) != -1 ) {
        char *content = json_fread(mpd.statefile);
        je = json_scanf(content, strlen(content), "{notificationWeb: %d, notificationPage: %d}", &state.a, &state.b);
        if (je != 2) {
          state.a=0;
          state.b=0;
        }
    } else {
        state.a=0;
        state.b=0;
    }

    status = mpd_run_status(mpd.conn);
    if (!status) {
        fprintf(stderr, "MPD mpd_run_status: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return 0;
    }

    mpd_send_command(mpd.conn, "replay_gain_status", NULL);
    struct mpd_pair *pair;
    while ((pair = mpd_recv_pair(mpd.conn)) != NULL) {
        replaygain=strdup(pair->value);
	mpd_return_pair(mpd.conn, pair);
    }
    
    len = json_printf(&out,
        "{type:settings, data:{"
        "repeat:%d, single:%d, crossfade:%d, consume:%d, random:%d, "
        "mixrampdb: %f, mixrampdelay: %f, mpdhost : %Q, mpdport: %d, passwort_set: %B, "
        "streamport: %d, coverimage: %Q, max_elements_per_page: %d, replaygain: %Q,"
        "notificationWeb: %d, notificationPage: %d"
        "}}", 
        mpd_status_get_repeat(status),
        mpd_status_get_single(status),
        mpd_status_get_crossfade(status),
        mpd_status_get_consume(status),
        mpd_status_get_random(status),
        mpd_status_get_mixrampdb(status),
        mpd_status_get_mixrampdelay(status),
        mpd.host, mpd.port, 
        mpd.password ? "true" : "false", 
        streamport, coverimage,
        MAX_ELEMENTS_PER_PAGE,
        replaygain,
        state.a,
        state.b
    );
    mpd_status_free(status);

    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}


int mympd_put_outputnames(char *buffer) {
    struct mpd_output *output;
    int len;
    int nr;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    len = json_printf(&out,"{type: outputnames, data: { outputs: [");
    
    mpd_send_outputs(mpd.conn);
    nr=0;    
    while ((output = mpd_recv_output(mpd.conn)) != NULL) {
        if (nr++) len += json_printf(&out, ",");
        len += json_printf(&out,"{id: %d, name: %Q}",
            mpd_output_get_id(output),
            mpd_output_get_name(output)
        );
        mpd_output_free(output);
    }
    if (!mpd_response_finish(mpd.conn)) {
        fprintf(stderr, "MPD outputs: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd_connection_clear_error(mpd.conn);
    }

    len += json_printf(&out,"]}}");
    
    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_get_cover(const char *uri, char *cover, int cover_len) {
    char *path=strdup(uri);
    int len;
    if (strncasecmp("http:",path,5) == 0 ) {
      len=snprintf(cover,cover_len,"/assets/coverimage-httpstream.png");
    }
    else {
      dirname(path);
      snprintf(cover,cover_len,"%s/library/%s/%s",SRC_PATH,path,coverimage);
      if ( access(cover, F_OK ) == -1 ) {
        len = snprintf(cover,cover_len,"/assets/coverimage-notavailable.png");
      } else {
        len = snprintf(cover,cover_len,"/library/%s/%s",path,coverimage);
      }
    }
    return len;
}

int mympd_put_current_song(char *buffer) {
    struct mpd_song *song;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    char cover[500];
    
    song = mpd_run_current_song(mpd.conn);
    if (song == NULL) {
        len = json_printf(&out,"{type: result, data: ok}");
        return len;
    }
        
    mympd_get_cover(mpd_song_get_uri(song),cover,500);

    len = json_printf(&out,"{type: song_change, data: { pos: %d, title: %Q, "
        "artist: %Q, album: %Q, uri: %Q, currentsongid: %d, albumartist: %Q, "
        "duration: %d, cover: %Q }}",
        mpd_song_get_pos(song),
        mympd_get_tag(song, MPD_TAG_TITLE),
        mympd_get_tag(song, MPD_TAG_ARTIST),
        mympd_get_tag(song, MPD_TAG_ALBUM),
        mpd_song_get_uri(song),
        mpd.song_id,
        mympd_get_tag(song, MPD_TAG_ALBUM_ARTIST),
        mpd_song_get_duration(song),
        cover
    );
    
    mpd_song_free(song);
    mpd_response_finish(mpd.conn);

    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_put_songdetails(char *buffer, char *uri) {
    struct mpd_entity *entity;
    const struct mpd_song *song;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    char cover[500];
    
    len = json_printf(&out, "{type: song_details, data: {");
    mpd_send_list_all_meta(mpd.conn, uri);
    while ((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        song = mpd_entity_get_song(entity);
        mympd_get_cover(mpd_song_get_uri(song),cover,500);
        len += json_printf(&out, "duration: %d, artist: %Q, album: %Q, title: %Q, albumartist: %Q, cover: %Q, uri: %Q, genre: %Q, track: %Q, date: %Q",
                    mpd_song_get_duration(song),
                    mympd_get_tag(song, MPD_TAG_ARTIST),
                    mympd_get_tag(song, MPD_TAG_ALBUM),
                    mympd_get_tag(song, MPD_TAG_TITLE),
                    mympd_get_tag(song, MPD_TAG_ALBUM_ARTIST),
                    cover,
                    uri,
                    mympd_get_tag(song, MPD_TAG_GENRE),
                    mympd_get_tag(song, MPD_TAG_TRACK),
                    mympd_get_tag(song, MPD_TAG_DATE)
               );
        mpd_entity_free(entity);
    }
    len += json_printf(&out, "}}");
    
    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_put_queue(char *buffer, unsigned int offset) {
    struct mpd_entity *entity;
    unsigned long totalTime = 0;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (!mpd_send_list_queue_range_meta(mpd.conn, 0, -1))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_queue_meta");
        
    len = json_printf(&out, "{type:queue, data :[ ");

    while ((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        const struct mpd_song *song;
        unsigned int drtn;
        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            song = mpd_entity_get_song(entity);
            drtn = mpd_song_get_duration(song);
            totalTime += drtn;
            entity_count ++;
            if (entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
                if (entities_returned ++) len += json_printf(&out,",");
                len += json_printf(&out, "{ id: %d, pos: %d, duration: %d, artist: %Q, album: %Q, title: %Q, uri: %Q }",
                    mpd_song_get_id(song),
                    mpd_song_get_pos(song),
                    mpd_song_get_duration(song),
                    mympd_get_tag(song, MPD_TAG_ARTIST),
                    mympd_get_tag(song, MPD_TAG_ALBUM),
                    mympd_get_tag(song, MPD_TAG_TITLE),
                    mpd_song_get_uri(song)
                );
            }
        }
        mpd_entity_free(entity);
    }

    len += json_printf(&out, "],totalTime: %d, totalEntities: %d, offset: %d, returnedEntities: %d, queue_version: %d }",
        totalTime,
        entity_count,
        offset,
        entities_returned,
        mpd.queue_version
    );
    
    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_put_browse(char *buffer, char *path, unsigned int offset, char *filter) {
    struct mpd_entity *entity;
    const struct mpd_playlist *pl;
    unsigned int entity_count = 0;
    unsigned int entities_returned = 0;
    const char *entityName;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (!mpd_send_list_meta(mpd.conn, path))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");

    len = json_printf(&out, "{ type: browse, data: [ ");

    while((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        const struct mpd_song *song;
        const struct mpd_directory *dir;
        entity_count ++;
        if (entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
            switch (mpd_entity_get_type(entity)) {
                case MPD_ENTITY_TYPE_UNKNOWN:
                        entity_count --;
                    break;
                case MPD_ENTITY_TYPE_SONG:
                    song = mpd_entity_get_song(entity);
                    entityName = mympd_get_tag(song, MPD_TAG_TITLE);
                    if (strncmp(filter,"-",1) == 0 || strncasecmp(filter,entityName,1) == 0 ||
                        ( strncmp(filter,"0",1) == 0 && isalpha(*entityName) == 0 )
                    ) {
                        if (entities_returned ++) len += json_printf(&out,",");
                        len += json_printf(&out, "{type: song, uri: %Q, album: %Q, artist: %Q, duration: %d, title: %Q, name: %Q }",
                            mpd_song_get_uri(song),
                            mympd_get_tag(song, MPD_TAG_ALBUM),
                            mympd_get_tag(song, MPD_TAG_ARTIST),
                            mpd_song_get_duration(song),
                            entityName,
                            entityName
                        );
                    } else {
                        entity_count --;
                    }
                    break;

                case MPD_ENTITY_TYPE_DIRECTORY:
                    dir = mpd_entity_get_directory(entity);                
                    entityName = mpd_directory_get_path(dir);
                    char *dirName = strrchr(entityName, '/');
                    if (dirName != NULL) {
                        dirName ++;
                    } else {
                        dirName = strdup(entityName);
                    }
                    if (strncmp(filter,"-",1) == 0 || strncasecmp(filter,dirName,1) == 0 ||
                        ( strncmp(filter,"0",1) == 0 && isalpha(*dirName) == 0 )
                    ) {                
                        if (entities_returned ++) len += json_printf(&out,",");
                        len += json_printf(&out, "{type: dir, uri: %Q, name: %Q }",
                            entityName,
                            dirName
                        );
                    } else {
                        entity_count --;
                    }
                    break;
                    
                case MPD_ENTITY_TYPE_PLAYLIST:
                    pl = mpd_entity_get_playlist(entity);
                    entityName = mpd_playlist_get_path(pl);
                    char *plName = strrchr(entityName, '/');
                    if (plName != NULL) {
                        plName ++;
                    } else {
                        plName = strdup(entityName);
                    }
                    if (strncmp(filter,"-",1) == 0 || strncasecmp(filter,plName,1) == 0 ||
                        ( strncmp(filter,"0",1) == 0 && isalpha(*plName) == 0 )
                    ) {
                        if (entities_returned ++) len += json_printf(&out,",");
                        len += json_printf(&out, "{ type: plist, uri: %Q, name: %Q }",
                            entityName,
                            plName
                        );
                    } else {
                        entity_count --;
                    }
                    break;
            }
        }
        mpd_entity_free(entity);
    }

    if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS || !mpd_response_finish(mpd.conn)) {
        fprintf(stderr, "MPD mpd_send_list_meta: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return 0;
    }

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, filter: %Q }",
        entity_count,
        offset,
        entities_returned,
        filter
    );

    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
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

    len = json_printf(&out, "{type: listDBtags, data: [ ");
    while((pair = mpd_recv_pair_tag(mpd.conn, mpd_tag_name_parse(mpdtagtype))) != NULL) {
        entity_count ++;
        if (entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
            if (strncmp(filter,"-",1) == 0 || strncasecmp(filter,pair->value,1) == 0 ||
                    ( strncmp(filter,"0",1) == 0 && isalpha(*pair->value) == 0 )
            ) {
                if (entities_returned ++) len += json_printf(&out,", ");
                len += json_printf(&out, "{type: %Q, value: %Q }",
                    mpdtagtype,
                    pair->value    
                );
            } else {
                entity_count --;
            }
        }
        mpd_return_pair(mpd.conn, pair);
    }
        
    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, "
        "tagtype: %Q, searchtagtype: %Q, searchstr: %Q, filter: %Q }",
        entity_count,
        offset,
        entities_returned,
        mpdtagtype,
        mpdsearchtagtype,
        searchstr,
        filter
    );

    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_put_songs_in_album(char *buffer, char *albumartist, char *album) {
    struct mpd_song *song;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;
    int len;
    char cover[500];
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_db_songs(mpd.conn, true) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_search_db_songs");
    }
    
    if (mpd_search_add_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM_ARTIST, albumartist) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");

    if (mpd_search_add_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
        
    if (mpd_search_commit(mpd.conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    else {
        len = json_printf(&out, "{ type: listTitles, data: [ ");

        while((song = mpd_recv_song(mpd.conn)) != NULL) {
            entity_count ++;
            if (entity_count <= MAX_ELEMENTS_PER_PAGE) {
                if (entities_returned ++) len += json_printf(&out, ", ");
                else mympd_get_cover(mpd_song_get_uri(song),cover,500);
                len += json_printf(&out, "{ type: song, uri: %Q, duration: %d, title: %Q, track: %Q }",
                    mpd_song_get_uri(song),
                    mpd_song_get_duration(song),
                    mympd_get_tag(song, MPD_TAG_TITLE),
                    mympd_get_tag(song, MPD_TAG_TRACK)
                );
            }
            mpd_song_free(song);
        }
        
        len += json_printf(&out, "], totalEntities: %d, returnedEntities: %d, albumartist: %Q, album: %Q, cover: %Q }",
            entity_count,
            entities_returned,
            albumartist,
            album,
            cover
        );
    }

    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_put_playlists(char *buffer, unsigned int offset, char *filter) {
    struct mpd_playlist *pl;
    unsigned int entity_count = 0;
    unsigned int entities_returned = 0;
    const char *plpath;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (!mpd_send_list_playlists(mpd.conn))
        RETURN_ERROR_AND_RECOVER("mpd_send_lists_playlists");

    len = json_printf(&out, "{ type: playlists, data: [ ");

    while((pl = mpd_recv_playlist(mpd.conn)) != NULL) {
        entity_count ++;
        if (entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
            plpath = mpd_playlist_get_path(pl);
            if (strncmp(filter,"-",1) == 0 || strncasecmp(filter,plpath,1) == 0 ||
                    ( strncmp(filter,"0",1) == 0 && isalpha(*plpath) == 0 )
            ) {
                if (entities_returned ++) len += json_printf(&out, ", ");
                len += json_printf(&out, "{ type: plist, uri: %Q, name: %Q, last_modified: %d }",
                    plpath,
                    plpath,
                    mpd_playlist_get_last_modified(pl)
                );
            } else {
                entity_count --;
            }
        }
        mpd_playlist_free(pl);
    }

    if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS || !mpd_response_finish(mpd.conn))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_playlists");
        
    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d }",
        entity_count,
        offset,
        entities_returned
    );

    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_put_playlist_list(char *buffer, char *uri, unsigned int offset, char *filter) {
    struct mpd_entity *entity;
    unsigned int entity_count = 0;
    unsigned int entities_returned = 0;
    const char *entityName;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (!mpd_send_list_playlist_meta(mpd.conn, uri))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");

    len = json_printf(&out, "{ type: playlist_detail, data: [ ");

    while((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        const struct mpd_song *song;
        entity_count ++;
        if (entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
            song = mpd_entity_get_song(entity);
            entityName = mympd_get_tag(song, MPD_TAG_TITLE);
            if (strncmp(filter,"-",1) == 0 || strncasecmp(filter,entityName,1) == 0 ||
               ( strncmp(filter,"0",1) == 0 && isalpha(*entityName) == 0 )
            ) {
                if (entities_returned ++) len += json_printf(&out,",");
                len += json_printf(&out, "{type: song, uri: %Q, album: %Q, artist: %Q, duration: %d, title: %Q, name: %Q }",
                    mpd_song_get_uri(song),
                    mympd_get_tag(song, MPD_TAG_ALBUM),
                    mympd_get_tag(song, MPD_TAG_ARTIST),
                    mpd_song_get_duration(song),
                    entityName,
                    entityName
                );
            } else {
                entity_count --;
            }
        }
        mpd_entity_free(entity);
    }

    len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, filter: %Q, uri: %Q }",
        entity_count,
        offset,
        entities_returned,
        filter,
        uri
    );

    if (len > MAX_SIZE) 
        fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_search(char *buffer, char *mpdtagtype, unsigned int offset, char *searchstr) {
    struct mpd_song *song;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);

    if (mpd_search_db_songs(mpd.conn, false) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_db_songs");
    
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
        len = json_printf(&out, "{ type: search, data: [ ");

        while((song = mpd_recv_song(mpd.conn)) != NULL) {
            entity_count ++;
            if (entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
                if (entities_returned ++) len += json_printf(&out, ", ");
                len += json_printf(&out, "{ type: song, uri: %Q, album: %Q, artist: %Q, duration: %d, title: %Q, name: %Q }",
                    mpd_song_get_uri(song),
                    mympd_get_tag(song, MPD_TAG_ALBUM),
                    mympd_get_tag(song, MPD_TAG_ARTIST),
                    mpd_song_get_duration(song),
                    mympd_get_tag(song, MPD_TAG_TITLE),
                    mympd_get_tag(song, MPD_TAG_TITLE)
                );
            }
            mpd_song_free(song);
        }

        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, mpdtagtype: %Q }",
            entity_count,
            offset,
            entities_returned,
            mpdtagtype
        );
    }

    if (len > MAX_SIZE)
        fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_search_add(char *buffer,char *mpdtagtype, char *searchstr) {
    struct mpd_song *song;
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    len = 0;

    if (mpd_search_add_db_songs(mpd.conn, false) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_search_add_db_songs");
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
    
    while((song = mpd_recv_song(mpd.conn)) != NULL) {
        mpd_song_free(song);
    }
        
    if (len > MAX_SIZE)
        fprintf(stderr,"Buffer truncated\n");
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
        len = json_printf(&out, "{ type: queuesearch, data: [ ");

        while((song = mpd_recv_song(mpd.conn)) != NULL) {
          entity_count ++;
          if (entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
            if (entities_returned ++) len += json_printf(&out, ", ");
            len += json_printf(&out, "{ type: song, id: %d, pos: %d, album: %Q, artist: %Q, duration: %d, title: %Q }",
                mpd_song_get_id(song),
                mpd_song_get_pos(song),
                mympd_get_tag(song, MPD_TAG_ALBUM),
                mympd_get_tag(song, MPD_TAG_ARTIST),
                mpd_song_get_duration(song),
                mympd_get_tag(song, MPD_TAG_TITLE)
            );
            mpd_song_free(song);
          }
        }
        
        len += json_printf(&out, "], totalEntities: %d, offset: %d, returnedEntities: %d, mpdtagtype: %Q }",
            entity_count,
            offset,
            entities_returned,
            mpdtagtype
        );
    }

    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}

int mympd_get_stats(char *buffer) {
    struct mpd_stats *stats = mpd_run_stats(mpd.conn);
    const unsigned *version = mpd_connection_get_server_version(mpd.conn);
    char mpd_version[20];
    int len;
    struct json_out out = JSON_OUT_BUF(buffer, MAX_SIZE);
    
    snprintf(mpd_version,20,"%i.%i.%i", version[0], version[1], version[2]);
    
    if (stats == NULL)
        RETURN_ERROR_AND_RECOVER("mympd_get_stats");
    len = json_printf(&out, "{ type: mpdstats, data: { artists: %d, albums: %d, songs: %d, "
        "playtime: %d, uptime: %d, dbupdated: %d, dbplaytime: %d, mympd_version: %Q, mpd_version: %Q }}",
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

    if (len > MAX_SIZE) fprintf(stderr,"Buffer truncated\n");
    return len;
}

void mympd_disconnect() {
    mpd.conn_state = MPD_DISCONNECT;
    mympd_poll(NULL);
}
