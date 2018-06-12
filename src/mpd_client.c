/* myMPD
   (c) 2018 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/ympd
   
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
#include <mpd/client.h>
#include <mpd/message.h>

#include "mpd_client.h"
#include "config.h"
#include "json_encode.h"

/* forward declaration */
static int mpd_notify_callback(struct mg_connection *c, const char *param);

const char * mpd_cmd_strs[] = {
    MPD_CMDS(GEN_STR)
};

char * get_arg1 (const char *p) {
	return strchr(p, ',') + 1;
}

char * get_arg2 (const char *p) {
	return get_arg1(get_arg1(p));
}

static inline enum mpd_cmd_ids get_cmd_id(const char *cmd)
{
    for(int i = 0; i < sizeof(mpd_cmd_strs)/sizeof(mpd_cmd_strs[0]); i++)
        if(!strncmp(cmd, mpd_cmd_strs[i], strlen(mpd_cmd_strs[i])))
            return i;

    return -1;
}

void callback_mpd(struct mg_connection *c,const struct mg_str msg)
{
    enum mpd_cmd_ids cmd_id = get_cmd_id(msg.p);
    size_t n = 0;
    unsigned int uint_buf, uint_buf_2;
    int int_buf;
    float float_buf;
    char *p_charbuf = NULL, *token;
    char *p_charbuf2 = NULL;
    char *searchstr = NULL;
    char request[500];

    fprintf(stdout,"Got request: %s\n",msg.p);
    
    if(cmd_id == -1)
        return 0;

    switch(cmd_id)
    {
        case MPD_API_UPDATE_DB:
            mpd_run_update(mpd.conn, NULL);
            break;
        case MPD_API_SET_PAUSE:
            mpd_run_toggle_pause(mpd.conn);
            break;
        case MPD_API_SET_PREV:
            mpd_run_previous(mpd.conn);
            break;
        case MPD_API_SET_NEXT:
            mpd_run_next(mpd.conn);
            break;
        case MPD_API_SET_PLAY:
            mpd_run_play(mpd.conn);
            break;
        case MPD_API_SET_STOP:
            mpd_run_stop(mpd.conn);
            break;
        case MPD_API_RM_ALL:
            mpd_run_clear(mpd.conn);
            break;
        case MPD_API_RM_TRACK:
            if(sscanf(msg.p, "MPD_API_RM_TRACK,%u", &uint_buf))
                mpd_run_delete_id(mpd.conn, uint_buf);
            break;
        case MPD_API_RM_RANGE:
            if(sscanf(msg.p, "MPD_API_RM_RANGE,%u,%u", &uint_buf, &uint_buf_2))
                mpd_run_delete_range(mpd.conn, uint_buf, uint_buf_2);
            break;
        case MPD_API_MOVE_TRACK:
            if (sscanf(msg.p, "MPD_API_MOVE_TRACK,%u,%u", &uint_buf, &uint_buf_2) == 2)
            {
                uint_buf -= 1;
                uint_buf_2 -= 1;
                mpd_run_move(mpd.conn, uint_buf, uint_buf_2);
            }
            break;
        case MPD_API_PLAY_TRACK:
            if(sscanf(msg.p, "MPD_API_PLAY_TRACK,%u", &uint_buf))
                mpd_run_play_id(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_RANDOM:
            if(sscanf(msg.p, "MPD_API_TOGGLE_RANDOM,%u", &uint_buf))
                mpd_run_random(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_REPEAT:
            if(sscanf(msg.p, "MPD_API_TOGGLE_REPEAT,%u", &uint_buf))
                mpd_run_repeat(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_CONSUME:
            if(sscanf(msg.p, "MPD_API_TOGGLE_CONSUME,%u", &uint_buf))
                mpd_run_consume(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_SINGLE:
            if(sscanf(msg.p, "MPD_API_TOGGLE_SINGLE,%u", &uint_buf))
                mpd_run_single(mpd.conn, uint_buf);
            break;
        case MPD_API_SET_CROSSFADE:
            if(sscanf(msg.p, "MPD_API_SET_CROSSFADE,%u", &uint_buf))
                mpd_run_crossfade(mpd.conn, uint_buf);
            break;
        case MPD_API_SET_MIXRAMPDB:
            if(sscanf(msg.p, "MPD_API_SET_MIXRAMPDB,%f", &float_buf))
                mpd_run_mixrampdb(mpd.conn, float_buf);
            break;
        case MPD_API_SET_MIXRAMPDELAY:
            if(sscanf(msg.p, "MPD_API_SET_MIXRAMPDELAY,%f", &float_buf))
                mpd_run_mixrampdelay(mpd.conn, float_buf);
            break;            
        case MPD_API_GET_OUTPUTS:
            mpd.buf_size = mpd_put_outputs(mpd.buf, 1);
            mpd_notify_callback(c, NULL);
            break;
        case MPD_API_TOGGLE_OUTPUT:
            if (sscanf(msg.p, "MPD_API_TOGGLE_OUTPUT,%u,%u", &uint_buf, &uint_buf_2)) {
                if (uint_buf_2)
                    mpd_run_enable_output(mpd.conn, uint_buf);
                else
                    mpd_run_disable_output(mpd.conn, uint_buf);
            }
            break;
        case MPD_API_SET_VOLUME:
            if(sscanf(msg.p, "MPD_API_SET_VOLUME,%ud", &uint_buf) && uint_buf <= 100)
                mpd_run_set_volume(mpd.conn, uint_buf);
            break;
        case MPD_API_SET_SEEK:
            if(sscanf(msg.p, "MPD_API_SET_SEEK,%u,%u", &uint_buf, &uint_buf_2))
                mpd_run_seek_id(mpd.conn, uint_buf, uint_buf_2);
            break;
        case MPD_API_GET_QUEUE:
            if(sscanf(msg.p, "MPD_API_GET_QUEUE,%u", &uint_buf))
                n = mpd_put_queue(mpd.buf, uint_buf);
            break;
        case MPD_API_GET_CURRENT_SONG:
                n = mpd_put_current_song(mpd.buf);
            break;            

        case MPD_API_GET_ARTISTS:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_GET_ARTISTS"))
		goto out_artist;
            uint_buf = strtoul(strtok(NULL, ","), NULL, 10);
            
            if((token = strtok(NULL, ",")) == NULL) {
                goto out_artist;
            } else {
                p_charbuf2 = strdup(token);
            }
            n = mympd_put_db_tag(mpd.buf, uint_buf, "AlbumArtist","","",p_charbuf2);
            free(p_charbuf2);
out_artist:
            free(p_charbuf);
            break;
        case MPD_API_GET_ARTISTALBUMS:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_GET_ARTISTALBUMS"))
		goto out_artistalbum;
            uint_buf = strtoul(strtok(NULL, ","), NULL, 10);
            
            if((token = strtok(NULL, ",")) == NULL) {
                goto out_artistalbum;
            } else {
                p_charbuf2 = strdup(token);
            }
            
            if((token = strtok(NULL, ",")) == NULL) {
                free(p_charbuf2);
                goto out_artistalbum;
            } else {
                searchstr = strdup(token);
            }
            n = mympd_put_db_tag(mpd.buf, uint_buf, "Album", "AlbumArtist", searchstr, p_charbuf2);
            free(searchstr);
            free(p_charbuf2);
out_artistalbum:
            free(p_charbuf);
            break;
        case MPD_API_GET_ARTISTALBUMTITLES:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_GET_ARTISTALBUMTITLES"))
		goto out_artistalbumtitle;
            
            if((token = strtok(NULL, ",")) == NULL) {
                goto out_artistalbumtitle;
            } else {
                searchstr = strdup(token);
            }
            n = mympd_put_songs_in_album(mpd.buf, searchstr, token+strlen(token)+1);
            free(searchstr);
out_artistalbumtitle:
            free(p_charbuf);        
            break;
        case MPD_API_GET_PLAYLISTS:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_GET_PLAYLISTS"))
		goto out_artist;
            uint_buf = strtoul(strtok(NULL, ","), NULL, 10);
            
            if((token = strtok(NULL, ",")) == NULL) {
                goto out_playlists;
            } else {
                p_charbuf2 = strdup(token);
            }
            n = mympd_put_playlists(mpd.buf, uint_buf, p_charbuf2);
            free(p_charbuf2);
out_playlists:
            free(p_charbuf);        
            break;
        case MPD_API_GET_BROWSE:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_GET_BROWSE"))
                goto out_browse;

            uint_buf = strtoul(strtok(NULL, ","), NULL, 10);

            if((token = strtok(NULL, ",")) == NULL) {
                goto out_browse;
            } else {
                p_charbuf2 = strdup(token);
            }

            if((token = strtok(NULL, ",")) == NULL) {
                free(p_charbuf2);
		goto out_browse;
            } else {
                searchstr = strdup(token);
            }
            n = mpd_put_browse(mpd.buf, p_charbuf2, uint_buf, searchstr);
            free(searchstr);
            free(p_charbuf2);
out_browse:
            free(p_charbuf);
            break;            
            
            break;
        case MPD_API_ADD_TRACK:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_ADD_TRACK"))
                goto out_add_track;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_add_track;

            free(p_charbuf);
            p_charbuf = strdup(msg.p);
            mpd_run_add(mpd.conn, get_arg1(p_charbuf));
out_add_track:
            free(p_charbuf);
            break;
        case MPD_API_ADD_PLAY_TRACK:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_ADD_PLAY_TRACK"))
                goto out_play_track;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_play_track;

			free(p_charbuf);
            p_charbuf = strdup(msg.p);
            int_buf = mpd_run_add_id(mpd.conn, get_arg1(p_charbuf));
            if(int_buf != -1)
                mpd_run_play_id(mpd.conn, int_buf);
out_play_track:
            free(p_charbuf);
            break;
        case MPD_API_ADD_PLAYLIST:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_ADD_PLAYLIST"))
                goto out_playlist;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_playlist;

			free(p_charbuf);
            p_charbuf = strdup(msg.p);
            mpd_run_load(mpd.conn, get_arg1(p_charbuf));
out_playlist:
            free(p_charbuf);
            break;
        case MPD_API_SAVE_QUEUE:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_SAVE_QUEUE"))
                goto out_save_queue;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_save_queue;

			free(p_charbuf);
            p_charbuf = strdup(msg.p);
            mpd_run_save(mpd.conn, get_arg1(p_charbuf));
out_save_queue:
            free(p_charbuf);
            break;
        case MPD_API_SEARCH_QUEUE:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_SEARCH_QUEUE"))
		goto out_search_queue;
            if((token = strtok(NULL, ",")) == NULL) {
                goto out_search_queue;
            } else {
                p_charbuf2 = strdup(token);
            }
            
            uint_buf = strtoul(strtok(NULL, ","), NULL, 10);
            
            if((token = strtok(NULL, ",")) == NULL) {
                free(p_charbuf2);
                goto out_search_queue;
            } else {
                searchstr = strdup(token);
            }
            n = mpd_search_queue(mpd.buf, p_charbuf2, uint_buf, searchstr);
            free(searchstr);
            free(p_charbuf2);
out_search_queue:
            free(p_charbuf);
            break;            
        case MPD_API_SEARCH_ADD:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_SEARCH_ADD"))
		goto out_search_add;
            if((token = strtok(NULL, ",")) == NULL) {
                goto out_search_add;
            } else {
                p_charbuf2 = strdup(token);
            }
            
            if((token = strtok(NULL, ",")) == NULL) {
                free(p_charbuf2);
                goto out_search_add;
            } else {
                searchstr = strdup(token);
            }
            n = mpd_search_add(mpd.buf, p_charbuf2, searchstr);
            free(searchstr);
            free(p_charbuf2);
out_search_add:
            free(p_charbuf);
            break;
        case MPD_API_SEARCH:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_SEARCH"))
		goto out_search;
            if((token = strtok(NULL, ",")) == NULL) {
                goto out_search;
            } else {
                p_charbuf2 = strdup(token);
            }
            
            uint_buf = strtoul(strtok(NULL, ","), NULL, 10);
            
            if((token = strtok(NULL, ",")) == NULL) {
                free(p_charbuf2);
                goto out_search;
            } else {
                searchstr = strdup(token);
            }
            n = mpd_search(mpd.buf, p_charbuf2, uint_buf, searchstr);
            free(searchstr);
            free(p_charbuf2);
out_search:
            free(p_charbuf);
            break;
        case MPD_API_SEND_SHUFFLE:
            mpd_run_shuffle(mpd.conn);
            break;
        case MPD_API_SEND_MESSAGE:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_SEND_MESSAGE"))
				goto out_send_message;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_send_message;

			free(p_charbuf);
            p_charbuf = strdup(get_arg1(msg.p));

            if ( strtok(p_charbuf, ",") == NULL )
                goto out_send_message;

            if ( (token = strtok(NULL, ",")) == NULL )
                goto out_send_message;

            mpd_run_send_message(mpd.conn, p_charbuf, token);
out_send_message:
            free(p_charbuf);
            break;
        case MPD_API_RM_PLAYLIST:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_RM_PLAYLIST"))
                goto out_rm_playlist;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_rm_playlist;

            free(p_charbuf);
            p_charbuf = strdup(msg.p);
            mpd_run_rm(mpd.conn, get_arg1(p_charbuf));
out_rm_playlist:
            free(p_charbuf);
            break;
        case MPD_API_GET_SETTINGS:
            n = mympd_put_settings(mpd.buf);
            break;
        case MPD_API_GET_STATS:
            n = mympd_get_stats(mpd.buf);
        break;
        case MPD_API_SET_REPLAYGAIN:
            p_charbuf = strdup(msg.p);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_SET_REPLAYGAIN"))
                goto out_set_replaygain;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_set_replaygain;

            free(p_charbuf);
            p_charbuf = strdup(msg.p);
            mpd_send_command(mpd.conn, "replay_gain_mode", get_arg1(p_charbuf), NULL);
            struct mpd_pair *pair;
            while ((pair = mpd_recv_pair(mpd.conn)) != NULL) {
        	mpd_return_pair(mpd.conn, pair);
            }            
out_set_replaygain:
            free(p_charbuf);        
        break;
    }

    if(mpd.conn_state == MPD_CONNECTED && mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS)
    {
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"error\", \"data\": \"%s\"}", 
            mpd_connection_get_error_message(mpd.conn));

        /* Try to recover error */
        if (!mpd_connection_clear_error(mpd.conn))
            mpd.conn_state = MPD_FAILURE;
    }

    if(n > 0) {
        //fprintf(stdout,"Send response:\n %s\n",mpd.buf);
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, strlen(mpd.buf));
    }

    return 0;
}

int mpd_close_handler(struct mg_connection *c)
{
    /* Cleanup session data */
    if(c->user_data)
        free(c->user_data);
    return 0;
}

static int mpd_notify_callback(struct mg_connection *c, const char *param) {
    size_t n;

    if(is_websocket(c))
        return 0;

    if(param)
    {
        /* error message? */
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"error\",\"data\":\"%s\"}",param);

        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, strlen(mpd.buf));
        return 0;
    }

    if(!c->user_data)
        c->user_data = calloc(1, sizeof(struct t_mpd_client_session));

    struct t_mpd_client_session *s = (struct t_mpd_client_session *)c->user_data;

    if(mpd.conn_state != MPD_CONNECTED) {
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"disconnected\"}");
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, strlen(mpd.buf));
    }
    else
    {
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, strlen(mpd.buf));
        
        if(s->song_id != mpd.song_id)
        {
            n = mpd_put_current_song(mpd.buf);
            mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, strlen(mpd.buf));
            s->song_id = mpd.song_id;
        }
        
        if(s->queue_version != mpd.queue_version)
        {
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"update_queue\"}");
            mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, mpd.buf, strlen(mpd.buf));
            s->queue_version = mpd.queue_version;
        }
        
    }

    return 0;
}

void mpd_poll(struct mg_mgr *s)
{
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
                for (struct mg_connection *c = mg_next(s, NULL); c != NULL; c = mg_next(s, c))
                {
                    mpd_notify_callback(c, mpd_connection_get_error_message(mpd.conn));
                }
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            if(mpd.password && !mpd_run_password(mpd.conn, mpd.password))
            {
                fprintf(stderr, "MPD connection: %s\n", mpd_connection_get_error_message(mpd.conn));
                for (struct mg_connection *c = mg_next(s, NULL); c != NULL; c = mg_next(s, c))
                {
                    mpd_notify_callback(c, mpd_connection_get_error_message(mpd.conn));
                }
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            fprintf(stderr, "MPD connected.\n");
            mpd_connection_set_timeout(mpd.conn, 10000);
            mpd.conn_state = MPD_CONNECTED;
            /* write outputs */
            mpd.buf_size = mpd_put_outputs(mpd.buf, 1);
            for (struct mg_connection *c = mg_next(s, NULL); c != NULL; c = mg_next(s, c))
            {
                mpd_notify_callback(c, NULL);
            }
            break;

        case MPD_FAILURE:
            fprintf(stderr, "MPD connection failed.\n");

        case MPD_DISCONNECT:
        case MPD_RECONNECT:
            if(mpd.conn != NULL)
                mpd_connection_free(mpd.conn);
            mpd.conn = NULL;
            mpd.conn_state = MPD_DISCONNECTED;
            break;

        case MPD_CONNECTED:
            mpd.buf_size = mpd_put_state(mpd.buf, &mpd.song_id, &mpd.next_song_id, &mpd.queue_version);
            for (struct mg_connection *c = mg_next(s, NULL); c != NULL; c = mg_next(s, c))
            {
                mpd_notify_callback(c, NULL);
            }
            mpd.buf_size = mpd_put_outputs(mpd.buf, 0);
            for (struct mg_connection *c = mg_next(s, NULL); c != NULL; c = mg_next(s, c))
            {
                mpd_notify_callback(c, NULL);
            }
            break;
    }
}

char* mpd_get_title(struct mpd_song const *song)
{
    char *str;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    if(str == NULL){
        str = basename((char *)mpd_song_get_uri(song));
    }

    return str;
}

char* mpd_get_track(struct mpd_song const *song)
{
    char *str;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_TRACK, 0);
    if(str == NULL){
        str = "-";
    }

    return str;
}


char* mpd_get_album(struct mpd_song const *song)
{
    char *str;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    if(str == NULL){
        str = "-";
    }

    return str;
}

char* mpd_get_artist(struct mpd_song const *song)
{
    char *str;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    if(str == NULL){
        str = "-";
    }

    return str;
}

char* mpd_get_album_artist(struct mpd_song const *song)
{
    char *str;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0);
    if(str == NULL){
        str = "-";
    }

    return str;
}

char* mpd_get_year(struct mpd_song const *song)
{
    char *str;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_DATE, 0);
    if(str == NULL){
        str = "-";
    }

    return str;
}

int mpd_put_state(char *buffer, int *current_song_id, int *next_song_id,  unsigned *queue_version)
{
    struct mpd_status *status;
    const struct mpd_audio_format *audioformat;
    int len;

    status = mpd_run_status(mpd.conn);
    if (!status) {
        fprintf(stderr, "MPD mpd_run_status: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return 0;
    }
    if (status) {
     audioformat = mpd_status_get_audio_format(status);
    }

    len = snprintf(buffer, MAX_SIZE,
        "{\"type\":\"state\", \"data\":{"
        "\"state\":%d, \"volume\":%d, \"songpos\": %d, \"elapsedTime\": %d, "
        "\"totalTime\":%d, \"currentsongid\": %d, \"kbitrate\": %d, "
        "\"audioformat\": { \"sample_rate\": %d, \"bits\": %d, \"channels\": %d}, "
        "\"queue_length\": %d, \"nextsongpos\": %d, \"nextsongid\": %d, "
        "\"queue_version\": %d"
        "}}", 
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

    *current_song_id = mpd_status_get_song_id(status);
    *next_song_id = mpd_status_get_next_song_id(status);
    *queue_version = mpd_status_get_queue_version(status);
    mpd_status_free(status);
    return len;
}

int mympd_put_settings(char *buffer)
{
    struct mpd_status *status;
    int len;
    char *replaygain;

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
			
    len = snprintf(buffer, MAX_SIZE,
        "{\"type\":\"settings\", \"data\":{"
        "\"repeat\":%d, \"single\":%d, \"crossfade\":%d, \"consume\":%d, \"random\":%d, "
        "\"mixrampdb\": %f, \"mixrampdelay\": %f, \"mpdhost\" : \"%s\", \"mpdport\": \"%d\", \"passwort_set\": %s, "
        "\"streamport\": \"%d\",\"coverimage\": \"%s\", \"max_elements_per_page\": %d, \"replaygain\": \"%s\""
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
        replaygain
    );
    mpd_status_free(status);
    return len;
}


int mpd_put_outputs(char *buffer, int names)
{
    struct mpd_output *out;
    int nout;
    char *str, *strend;

    str = buffer;
    strend = buffer+MAX_SIZE;
    str += snprintf(str, strend-str, "{\"type\":\"%s\", \"data\":{",
            names ? "outputnames" : "outputs");

    mpd_send_outputs(mpd.conn);
    nout = 0;
    while ((out = mpd_recv_output(mpd.conn)) != NULL) {
        if (nout++)
            *str++ = ',';
        if (names)
            str += snprintf(str, strend - str, " \"%d\":\"%s\"",
                    mpd_output_get_id(out), mpd_output_get_name(out));
        else
            str += snprintf(str, strend-str, " \"%d\":%d",
                    mpd_output_get_id(out), mpd_output_get_enabled(out));
        mpd_output_free(out);
    }
    if (!mpd_response_finish(mpd.conn)) {
        fprintf(stderr, "MPD outputs: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd_connection_clear_error(mpd.conn);
        return 0;
    }
    str += snprintf(str, strend-str, " }}");
    return str-buffer;
}

int mpd_put_current_song(char *buffer)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_song *song;

    song = mpd_run_current_song(mpd.conn);
    if(song == NULL)
        return 0;

    cur += json_emit_raw_str(cur, end - cur, "{\"type\": \"song_change\", \"data\":{\"pos\":");
    cur += json_emit_int(cur, end - cur, mpd_song_get_pos(song));
    cur += json_emit_raw_str(cur, end - cur, ",\"title\":");
    cur += json_emit_quoted_str(cur, end - cur, mpd_get_title(song));
    cur += json_emit_raw_str(cur, end - cur, ",\"artist\":");
    cur += json_emit_quoted_str(cur, end - cur, mpd_get_artist(song));
    cur += json_emit_raw_str(cur, end - cur, ",\"album\":");
    cur += json_emit_quoted_str(cur, end - cur, mpd_get_album(song));
    cur += json_emit_raw_str(cur, end - cur, ",\"uri\":");
    cur += json_emit_quoted_str(cur, end - cur, mpd_song_get_uri(song));
    cur += json_emit_raw_str(cur, end - cur, ",\"currentsongid\":");
    cur += json_emit_int(cur, end - cur, mpd.song_id);
    cur += json_emit_raw_str(cur, end - cur, "}}");
    
    mpd_song_free(song);
    mpd_response_finish(mpd.conn);

    return cur - buffer;
}

int mpd_put_queue(char *buffer, unsigned int offset)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_entity *entity;
    unsigned long totalTime = 0;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;

    if (!mpd_send_list_queue_range_meta(mpd.conn, 0, -1))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_queue_meta");
        
    cur += json_emit_raw_str(cur, end  - cur, "{\"type\":\"queue\",\"data\":[ ");

    while((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        const struct mpd_song *song;
        unsigned int drtn;

        if(mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            song = mpd_entity_get_song(entity);
            drtn = mpd_song_get_duration(song);
            totalTime += drtn;
            entity_count ++;
            if(entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
                entities_returned ++;
                cur += json_emit_raw_str(cur, end - cur, "{\"id\":");
                cur += json_emit_int(cur, end - cur, mpd_song_get_id(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"pos\":");
                cur += json_emit_int(cur, end - cur, mpd_song_get_pos(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"duration\":");
                cur += json_emit_int(cur, end - cur, mpd_song_get_duration(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"artist\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_get_artist(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"album\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_get_album(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"title\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_get_title(song));
                cur += json_emit_raw_str(cur, end - cur, "},");
            }

        }
        mpd_entity_free(entity);
    }

    /* remove last ',' */
    cur--;

    cur += json_emit_raw_str(cur, end - cur, "],\"totalTime\":");
    cur += json_emit_int(cur, end - cur, totalTime);
    cur += json_emit_raw_str(cur, end - cur, ",\"totalEntities\":");
    cur += json_emit_int(cur, end - cur, entity_count);
    cur += json_emit_raw_str(cur, end - cur, ",\"offset\":");
    cur += json_emit_int(cur, end - cur, offset);
    cur += json_emit_raw_str(cur, end - cur, ",\"returnedEntities\":");
    cur += json_emit_int(cur, end - cur, entities_returned);
    cur += json_emit_raw_str(cur, end - cur, "}");
    return cur - buffer;
}

int mpd_put_browse(char *buffer, char *path, unsigned int offset, char *filter)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_entity *entity;
    const struct mpd_playlist *pl;
    unsigned int entity_count = 0;
    unsigned int entities_returned = 0;
    const char *entityName;

    if (!mpd_send_list_meta(mpd.conn, path))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");

    cur += json_emit_raw_str(cur, end  - cur, "{\"type\":\"browse\",\"data\":[ ");

    while((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        const struct mpd_song *song;
        const struct mpd_directory *dir;
        entity_count ++;
        if(entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
            switch (mpd_entity_get_type(entity)) {
                case MPD_ENTITY_TYPE_UNKNOWN:
                        entity_count --;
                    break;
                case MPD_ENTITY_TYPE_SONG:
                    song = mpd_entity_get_song(entity);
                    entityName = mpd_get_title(song);
                    if (strncmp(filter,"-",1) == 0 || strncasecmp(filter,entityName,1) == 0 ||
                        ( strncmp(filter,"0",1) == 0 && isalpha(*entityName) == 0 )
                    ) {
                        entities_returned ++;
                        cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"song\",\"uri\":");
                        cur += json_emit_quoted_str(cur, end - cur, mpd_song_get_uri(song));
                        cur += json_emit_raw_str(cur, end - cur, ",\"album\":");
                        cur += json_emit_quoted_str(cur, end - cur, mpd_get_album(song));
                        cur += json_emit_raw_str(cur, end - cur, ",\"artist\":");
                        cur += json_emit_quoted_str(cur, end - cur, mpd_get_artist(song));
                        cur += json_emit_raw_str(cur, end - cur, ",\"duration\":");
                        cur += json_emit_int(cur, end - cur, mpd_song_get_duration(song));
                        cur += json_emit_raw_str(cur, end - cur, ",\"title\":");
                        cur += json_emit_quoted_str(cur, end - cur, entityName);
                        cur += json_emit_raw_str(cur, end - cur, "},");
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
                        entities_returned ++;
                        cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"directory\",\"dir\":");
                        cur += json_emit_quoted_str(cur, end - cur, entityName);
                        cur += json_emit_raw_str(cur, end - cur, "},");
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
                        entities_returned ++;
                        cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"playlist\",\"plist\":");
                        cur += json_emit_quoted_str(cur, end - cur, entityName );
                        cur += json_emit_raw_str(cur, end - cur, "},");
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

    /* remove last ',' */
    cur--;
    cur += json_emit_raw_str(cur, end - cur, "],\"totalEntities\":");
    cur += json_emit_int(cur, end - cur, entity_count);
    cur += json_emit_raw_str(cur, end - cur, ",\"offset\":");
    cur += json_emit_int(cur, end - cur, offset);
    cur += json_emit_raw_str(cur, end - cur, ",\"returnedEntities\":");
    cur += json_emit_int(cur, end - cur, entities_returned);
    cur += json_emit_raw_str(cur, end - cur, ",\"filter\":");
    cur += json_emit_quoted_str(cur, end - cur, filter);    
    cur += json_emit_raw_str(cur, end - cur, "}");
    return cur - buffer;
}

int mympd_put_db_tag(char *buffer, unsigned int offset, char *mpdtagtype, char *mpdsearchtagtype, char *searchstr, char *filter)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_pair *pair;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;

    if(mpd_search_db_tags(mpd.conn, mpd_tag_name_parse(mpdtagtype)) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_db_tags");

    if (mpd_tag_name_parse(mpdsearchtagtype) != MPD_TAG_UNKNOWN) {
        if (mpd_search_add_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdsearchtagtype), searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
    }

    if(mpd_search_commit(mpd.conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");

    cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"listDBtags\",\"data\":[ ");
    while((pair = mpd_recv_pair_tag(mpd.conn, mpd_tag_name_parse(mpdtagtype))) != NULL) {
        entity_count ++;
        if(entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
            if (strncmp(filter,"-",1) == 0 || strncasecmp(filter,pair->value,1) == 0 ||
                    ( strncmp(filter,"0",1) == 0 && isalpha(*pair->value) == 0 )
            ) {
                entities_returned ++;
                cur += json_emit_raw_str(cur, end - cur, "{\"type\":");
                cur += json_emit_quoted_str(cur, end - cur, mpdtagtype);
                cur += json_emit_raw_str(cur, end - cur, ",\"value\":");
                cur += json_emit_quoted_str(cur, end - cur, pair->value);
                cur += json_emit_raw_str(cur, end - cur, "},");
            } else {
                entity_count --;
            }
        }
        mpd_return_pair(mpd.conn, pair);
    }
        
    /* remove last ',' */
    cur--;

    cur += json_emit_raw_str(cur, end - cur, "]");
    cur += json_emit_raw_str(cur, end - cur, ",\"totalEntities\":");
    cur += json_emit_int(cur, end - cur, entity_count);
    cur += json_emit_raw_str(cur, end - cur, ",\"offset\":");
    cur += json_emit_int(cur, end - cur, offset);
    cur += json_emit_raw_str(cur, end - cur, ",\"returnedEntities\":");
    cur += json_emit_int(cur, end - cur, entities_returned);
    cur += json_emit_raw_str(cur, end - cur, ",\"tagtype\":");
    cur += json_emit_quoted_str(cur, end - cur, mpdtagtype);
    cur += json_emit_raw_str(cur, end - cur, ",\"searchtagtype\":");
    cur += json_emit_quoted_str(cur, end - cur, mpdsearchtagtype);
    cur += json_emit_raw_str(cur, end - cur, ",\"searchstr\":");
    cur += json_emit_quoted_str(cur, end - cur, searchstr);
    cur += json_emit_raw_str(cur, end - cur, ",\"filter\":");
    cur += json_emit_quoted_str(cur, end - cur, filter);
    cur += json_emit_raw_str(cur, end - cur, "}");

    return cur - buffer;
}

int mympd_put_songs_in_album(char *buffer, char *albumartist, char *album)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_song *song;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;

    if(mpd_search_db_songs(mpd.conn, true) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_search_db_songs");
    }
    
    if (mpd_search_add_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM_ARTIST, albumartist) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");

    if (mpd_search_add_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
        
    if(mpd_search_commit(mpd.conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    else {
        cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"listTitles\",\"data\":[ ");

        while((song = mpd_recv_song(mpd.conn)) != NULL) {
            entity_count ++;
            if(entity_count <= MAX_ELEMENTS_PER_PAGE) {
                entities_returned ++;
                cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"song\"");
                cur += json_emit_raw_str(cur, end - cur, ",\"uri\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_song_get_uri(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"duration\":");
                cur += json_emit_int(cur, end - cur, mpd_song_get_duration(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"title\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_get_title(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"track\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_get_track(song));
                cur += json_emit_raw_str(cur, end - cur, "},");
            }
            mpd_song_free(song);
        }
        
        /* remove last ',' */
        cur--;

        cur += json_emit_raw_str(cur, end - cur, "]");
        cur += json_emit_raw_str(cur, end - cur, ",\"totalEntities\":");
        cur += json_emit_int(cur, end - cur, entity_count);
        cur += json_emit_raw_str(cur, end - cur, ",\"returnedEntities\":");
        cur += json_emit_int(cur, end - cur, entities_returned);
        cur += json_emit_raw_str(cur, end - cur, ",\"albumartist\":");
        cur += json_emit_quoted_str(cur, end - cur, albumartist);
        cur += json_emit_raw_str(cur, end - cur, ",\"album\":");
        cur += json_emit_quoted_str(cur, end - cur, album);
        cur += json_emit_raw_str(cur, end - cur, "}");
    }
    return cur - buffer;
}

int mympd_put_playlists(char *buffer, unsigned int offset, char *filter)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_playlist *pl;
    unsigned int entity_count = 0;
    unsigned int entities_returned = 0;
    const char *plpath;

    if (!mpd_send_list_playlists(mpd.conn))
        RETURN_ERROR_AND_RECOVER("mpd_send_lists_playlists");

    cur += json_emit_raw_str(cur, end  - cur, "{\"type\":\"playlists\",\"data\":[ ");

    while((pl = mpd_recv_playlist(mpd.conn)) != NULL) {
        entity_count ++;
        if(entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
            plpath = mpd_playlist_get_path(pl);
            if (strncmp(filter,"-",1) == 0 || strncasecmp(filter,plpath,1) == 0 ||
                    ( strncmp(filter,"0",1) == 0 && isalpha(*plpath) == 0 )
            ) {
                entities_returned ++;
                cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"playlist\",\"plist\":");
                cur += json_emit_quoted_str(cur, end - cur, plpath);
                cur += json_emit_raw_str(cur, end - cur, ",\"last_modified\":");
                cur += json_emit_int(cur, end - cur, mpd_playlist_get_last_modified(pl));
                cur += json_emit_raw_str(cur, end - cur, "},");
            } else {
                entity_count --;
            }
        }
        mpd_playlist_free(pl);
    }

    if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS || !mpd_response_finish(mpd.conn))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_playlists");
        
    /* remove last ',' */
    cur--;

    cur += json_emit_raw_str(cur, end - cur, "],\"totalEntities\":");
    cur += json_emit_int(cur, end - cur, entity_count);
        cur += json_emit_raw_str(cur, end - cur, ",\"offset\":");
        cur += json_emit_int(cur, end - cur, offset);
        cur += json_emit_raw_str(cur, end - cur, ",\"returnedEntities\":");
        cur += json_emit_int(cur, end - cur, entities_returned);    
    cur += json_emit_raw_str(cur, end - cur, "}");
    return cur - buffer;
}

int mpd_search(char *buffer, char *mpdtagtype, unsigned int offset, char *searchstr)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_song *song;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;

    if(mpd_search_db_songs(mpd.conn, false) == false) {
        RETURN_ERROR_AND_RECOVER("mpd_search_db_songs");
    }
    
    if (mpd_tag_name_parse(mpdtagtype) != MPD_TAG_UNKNOWN) {
       if (mpd_search_add_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, mpd_tag_name_parse(mpdtagtype), searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_tag_constraint");
    }
    else {
        if (mpd_search_add_any_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, searchstr) == false)
            RETURN_ERROR_AND_RECOVER("mpd_search_add_any_tag_constraint");        
    }    
        
    if(mpd_search_commit(mpd.conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    else {
        cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"search\",\"data\":[ ");

        while((song = mpd_recv_song(mpd.conn)) != NULL) {
            entity_count ++;
            if(entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
                entities_returned ++;
                cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"song\"");
                cur += json_emit_raw_str(cur, end - cur, ",\"uri\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_song_get_uri(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"album\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_get_album(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"artist\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_get_artist(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"duration\":");
                cur += json_emit_int(cur, end - cur, mpd_song_get_duration(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"title\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_get_title(song));
                cur += json_emit_raw_str(cur, end - cur, "},");
            }
            mpd_song_free(song);
        }
        
        /* remove last ',' */
        cur--;

        cur += json_emit_raw_str(cur, end - cur, "]");
        cur += json_emit_raw_str(cur, end - cur, ",\"totalEntities\":");
        cur += json_emit_int(cur, end - cur, entity_count);
        cur += json_emit_raw_str(cur, end - cur, ",\"offset\":");
        cur += json_emit_int(cur, end - cur, offset);
        cur += json_emit_raw_str(cur, end - cur, ",\"returnedEntities\":");
        cur += json_emit_int(cur, end - cur, entities_returned);
        cur += json_emit_raw_str(cur, end - cur, ",\"mpdtagtype\":");
        cur += json_emit_quoted_str(cur, end - cur, mpdtagtype);        
        cur += json_emit_raw_str(cur, end - cur, "}");
    }
    return cur - buffer;
}

int mpd_search_add(char *buffer,char *mpdtagtype, char *searchstr)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_song *song;

    if(mpd_search_add_db_songs(mpd.conn, false) == false) {
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
        
    if(mpd_search_commit(mpd.conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    
    while((song = mpd_recv_song(mpd.conn)) != NULL) {
        mpd_song_free(song);
    }
        
    return cur - buffer;
}

int mpd_search_queue(char *buffer, char *mpdtagtype, unsigned int offset, char *searchstr)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_song *song;
    unsigned long entity_count = 0;
    unsigned long entities_returned = 0;

    if(mpd_search_queue_songs(mpd.conn, false) == false) {
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

    if(mpd_search_commit(mpd.conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    else {
        cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"queuesearch\",\"data\":[ ");

        while((song = mpd_recv_song(mpd.conn)) != NULL) {
          entity_count ++;
          if(entity_count > offset && entity_count <= offset+MAX_ELEMENTS_PER_PAGE) {
            entities_returned ++;
            cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"song\"");
            cur += json_emit_raw_str(cur, end - cur, ",\"id\":");
            cur += json_emit_int(cur, end - cur, mpd_song_get_id(song));            
            cur += json_emit_raw_str(cur, end - cur, ",\"pos\":");
            cur += json_emit_int(cur, end - cur, mpd_song_get_pos(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"album\":");
            cur += json_emit_quoted_str(cur, end - cur, mpd_get_album(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"artist\":");
            cur += json_emit_quoted_str(cur, end - cur, mpd_get_artist(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"duration\":");
            cur += json_emit_int(cur, end - cur, mpd_song_get_duration(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"title\":");
            cur += json_emit_quoted_str(cur, end - cur, mpd_get_title(song));
            cur += json_emit_raw_str(cur, end - cur, "},");
            mpd_song_free(song);
          }
        }
        
        cur--;

        cur += json_emit_raw_str(cur, end - cur, "]");
        cur += json_emit_raw_str(cur, end - cur, ",\"totalEntities\":");
        cur += json_emit_int(cur, end - cur, entity_count);
        cur += json_emit_raw_str(cur, end - cur, ",\"offset\":");
        cur += json_emit_int(cur, end - cur, offset);
        cur += json_emit_raw_str(cur, end - cur, ",\"returnedEntities\":");
        cur += json_emit_int(cur, end - cur, entities_returned);        
        cur += json_emit_raw_str(cur, end - cur, ",\"mpdtagtype\":");
        cur += json_emit_quoted_str(cur, end - cur, mpdtagtype);
        cur += json_emit_raw_str(cur, end - cur, "}");
    }
    return cur - buffer;
}

int mympd_get_stats(char *buffer)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_stats *stats = mpd_run_stats(mpd.conn);
    const unsigned *version = mpd_connection_get_server_version(mpd.conn);
    char mpd_version[20];
    
    snprintf(mpd_version,20,"%i.%i.%i", version[0], version[1], version[2]);
    
    if (stats == NULL)
        RETURN_ERROR_AND_RECOVER("mympd_get_stats");
    cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"mpdstats\",\"data\": {");
    cur += json_emit_raw_str(cur, end - cur, "\"artists\":");
    cur += json_emit_int(cur, end - cur, mpd_stats_get_number_of_artists(stats));
    cur += json_emit_raw_str(cur, end - cur, ",\"albums\":");
    cur += json_emit_int(cur, end - cur, mpd_stats_get_number_of_albums(stats));
    cur += json_emit_raw_str(cur, end - cur, ",\"songs\":");
    cur += json_emit_int(cur, end - cur, mpd_stats_get_number_of_songs(stats));
    cur += json_emit_raw_str(cur, end - cur, ",\"playtime\":");
    cur += json_emit_int(cur, end - cur, mpd_stats_get_play_time(stats));
    cur += json_emit_raw_str(cur, end - cur, ",\"uptime\":");
    cur += json_emit_int(cur, end - cur, mpd_stats_get_uptime(stats));
    cur += json_emit_raw_str(cur, end - cur, ",\"dbupdated\":");
    cur += json_emit_int(cur, end - cur, mpd_stats_get_db_update_time(stats));
    cur += json_emit_raw_str(cur, end - cur, ",\"dbplaytime\":");
    cur += json_emit_int(cur, end - cur, mpd_stats_get_db_play_time(stats));
    cur += json_emit_raw_str(cur, end - cur, ",\"mympd_version\":");
    cur += json_emit_quoted_str(cur, end - cur, MYMPD_VERSION);
    cur += json_emit_raw_str(cur, end - cur, ",\"mpd_version\":");
    cur += json_emit_quoted_str(cur, end - cur, mpd_version);
    cur += json_emit_raw_str(cur, end - cur, "}}");
    
    mpd_stats_free(stats);
    return cur - buffer;
}

void mpd_disconnect()
{
    mpd.conn_state = MPD_DISCONNECT;
    mpd_poll(NULL);
}
