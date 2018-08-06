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
   
#ifndef __MPD_CLIENT_H__
#define __MPD_CLIENT_H__

#include "../dist/src/mongoose/mongoose.h"

#define RETURN_ERROR_AND_RECOVER(X) do { \
    fprintf(stderr, "MPD X: %s\n", mpd_connection_get_error_message(mpd.conn)); \
    len = json_printf(&out, "{ type:error, data : %Q }", \
        mpd_connection_get_error_message(mpd.conn) \
    ); \
    if (!mpd_connection_clear_error(mpd.conn)) \
        mpd.conn_state = MPD_FAILURE; \
    return len; \
} while(0)


#define MAX_SIZE 1024 * 100
#define MAX_ELEMENTS_PER_PAGE 100

#define GEN_ENUM(X) X,
#define GEN_STR(X) #X,
#define MPD_CMDS(X) \
    X(MPD_API_GET_QUEUE) \
    X(MPD_API_GET_FILESYSTEM) \
    X(MPD_API_ADD_TRACK_AFTER) \
    X(MPD_API_ADD_TRACK) \
    X(MPD_API_ADD_PLAY_TRACK) \
    X(MPD_API_REPLACE_TRACK) \
    X(MPD_API_ADD_PLAYLIST) \
    X(MPD_API_REPLACE_PLAYLIST) \
    X(MPD_API_RM_PLAYLIST_TRACK) \
    X(MPD_API_PLAYLIST_CLEAR) \
    X(MPD_API_PLAYLIST_RENAME) \
    X(MPD_API_PLAYLIST_MOVE_TRACK) \
    X(MPD_API_ADD_TO_PLAYLIST) \
    X(MPD_API_PLAY_TRACK) \
    X(MPD_API_SAVE_QUEUE) \
    X(MPD_API_RM_TRACK) \
    X(MPD_API_RM_RANGE) \
    X(MPD_API_QUEUE_CLEAR) \
    X(MPD_API_QUEUE_CROP) \
    X(MPD_API_MOVE_TRACK) \
    X(MPD_API_SEARCH_QUEUE) \
    X(MPD_API_SEARCH_ADD_PLIST) \
    X(MPD_API_SEARCH_ADD) \
    X(MPD_API_SEARCH) \
    X(MPD_API_SEND_MESSAGE) \
    X(MPD_API_SET_VOLUME) \
    X(MPD_API_SET_PAUSE) \
    X(MPD_API_SET_PLAY) \
    X(MPD_API_SET_STOP) \
    X(MPD_API_SET_SEEK) \
    X(MPD_API_SET_NEXT) \
    X(MPD_API_SET_PREV) \
    X(MPD_API_UPDATE_DB) \
    X(MPD_API_GET_OUTPUTNAMES) \
    X(MPD_API_TOGGLE_OUTPUT) \
    X(MPD_API_SEND_SHUFFLE) \
    X(MPD_API_GET_STATS) \
    X(MPD_API_GET_PLAYLISTS) \
    X(MPD_API_GET_PLAYLIST_LIST) \
    X(MPD_API_RM_PLAYLIST) \
    X(MPD_API_GET_ARTISTALBUMS) \
    X(MPD_API_GET_ARTISTALBUMTITLES) \
    X(MPD_API_GET_ARTISTS) \
    X(MPD_API_GET_CURRENT_SONG) \
    X(MPD_API_GET_SONGDETAILS) \
    X(MPD_API_WELCOME) \
    X(MPD_API_GET_SETTINGS) \
    X(MPD_API_SET_SETTINGS) \
    X(MPD_API_UNKNOWN)

enum mpd_cmd_ids {
    MPD_CMDS(GEN_ENUM)
};

enum mpd_conn_states {
    MPD_DISCONNECTED,
    MPD_FAILURE,
    MPD_CONNECTED,
    MPD_RECONNECT,
    MPD_DISCONNECT
};

struct t_mpd {
    int port;
    char host[128];
    char *password;
    char *statefile;

    struct mpd_connection *conn;
    enum mpd_conn_states conn_state;

    /* Reponse Buffer */
    char buf[MAX_SIZE];
    size_t buf_size;

    int song_id;
    int next_song_id;
    unsigned queue_version;
} mpd;

int streamport;
char coverimage[40];

static int is_websocket(const struct mg_connection *nc) {
  return nc->flags & MG_F_IS_WEBSOCKET;
}

struct t_mpd_client_session {
    int song_id;
    int next_song_id;
    unsigned queue_version;
};

void mympd_poll(struct mg_mgr *s);
void callback_mympd(struct mg_connection *nc, const struct mg_str msg);
int mympd_close_handler(struct mg_connection *c);
int mympd_put_state(char *buffer, int *current_song_id, int *next_song_id, unsigned *queue_version);
int mympd_put_outputnames(char *buffer);
int mympd_put_current_song(char *buffer);
int mympd_put_queue(char *buffer, unsigned int offset);
int mympd_put_browse(char *buffer, char *path, unsigned int offset, char *filter);
int mympd_search(char *buffer, char *mpdtagtype, unsigned int offset, char *searchstr);
int mympd_search_add(char *buffer, char *mpdtagtype, char *searchstr);
int mympd_search_add_plist(char *plist, char *mpdtagtype, char *searchstr);
int mympd_search_queue(char *buffer, char *mpdtagtype, unsigned int offset, char *searchstr);
int mympd_put_welcome(char *buffer);
int mympd_get_stats(char *buffer);
int mympd_put_settings(char *buffer);
int mympd_put_db_tag(char *buffer, unsigned int offset, char *mpdtagtype, char *mpdsearchtagtype, char *searchstr, char *filter);
int mympd_put_songs_in_album(char *buffer, char *albumartist, char *album);
int mympd_put_playlists(char *buffer, unsigned int offset, char *filter);
int mympd_put_playlist_list(char *buffer, char *uri, unsigned int offset, char *filter);
int mympd_put_songdetails(char *buffer, char *uri);
int mympd_queue_crop(char *buffer);
void mympd_disconnect();
#endif

