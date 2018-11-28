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
#include "list.h"

#define RETURN_ERROR_AND_RECOVER(X) do { \
    printf("MPD %s: %s\n", X, mpd_connection_get_error_message(mpd.conn)); \
    len = json_printf(&out, "{ type:error, data : %Q }", mpd_connection_get_error_message(mpd.conn)); \
    if (!mpd_connection_clear_error(mpd.conn)) \
        mpd.conn_state = MPD_FAILURE; \
    return len; \
} while (0)

#define LOG_ERROR_AND_RECOVER(X) do { \
    printf("MPD %s: %s\n", X, mpd_connection_get_error_message(mpd.conn)); \
    if (!mpd_connection_clear_error(mpd.conn)) \
        mpd.conn_state = MPD_FAILURE; \
} while (0)

#define CHECK_RETURN_LEN() do { \
    if (len > MAX_SIZE) \
        printf("Buffer truncated: %d, %d\n", len, MAX_SIZE); \
    return len; \
} while (0)

#define PUT_SONG_TAGS() do { \
    struct node *current = mympd_tags.list; \
    int tagnr = 0; \
    while (current != NULL) { \
        if (tagnr ++) \
            len += json_printf(&out, ","); \
        len += json_printf(&out, "%Q: %Q", current->data, mympd_get_tag(song, mpd_tag_name_parse(current->data))); \
        current = current->next; \
    } \
    len += json_printf(&out, ", Duration: %d, uri: %Q", mpd_song_get_duration(song), mpd_song_get_uri(song)); \
} while (0)

#define PUT_MIN_SONG_TAGS() do { \
    len += json_printf(&out, "Title: %Q, Duration: %d, uri: %Q", mympd_get_tag(song, MPD_TAG_TITLE), mpd_song_get_duration(song), mpd_song_get_uri(song)); \
} while (0)


#define LOG_INFO() if (config.loglevel >= 1) 
#define LOG_VERBOSE() if (config.loglevel >= 2) 
#define LOG_DEBUG() if (config.loglevel == 3) 

#define MAX_SIZE 2048 * 400
#define MAX_ELEMENTS_PER_PAGE 400

#define GEN_ENUM(X) X,
#define GEN_STR(X) #X,
#define MPD_CMDS(X) \
    X(MPD_API_QUEUE_CLEAR) \
    X(MPD_API_QUEUE_CROP) \
    X(MPD_API_QUEUE_SAVE) \
    X(MPD_API_QUEUE_LIST) \
    X(MPD_API_QUEUE_SEARCH) \
    X(MPD_API_QUEUE_RM_TRACK) \
    X(MPD_API_QUEUE_RM_RANGE) \
    X(MPD_API_QUEUE_MOVE_TRACK) \
    X(MPD_API_QUEUE_ADD_TRACK_AFTER) \
    X(MPD_API_QUEUE_ADD_TRACK) \
    X(MPD_API_QUEUE_ADD_PLAY_TRACK) \
    X(MPD_API_QUEUE_REPLACE_TRACK) \
    X(MPD_API_QUEUE_ADD_PLAYLIST) \
    X(MPD_API_QUEUE_REPLACE_PLAYLIST) \
    X(MPD_API_QUEUE_SHUFFLE) \
    X(MPD_API_QUEUE_LAST_PLAYED) \
    X(MPD_API_PLAYLIST_CLEAR) \
    X(MPD_API_PLAYLIST_RENAME) \
    X(MPD_API_PLAYLIST_MOVE_TRACK) \
    X(MPD_API_PLAYLIST_ADD_TRACK) \
    X(MPD_API_PLAYLIST_RM_TRACK) \
    X(MPD_API_PLAYLIST_RM) \
    X(MPD_API_PLAYLIST_LIST) \
    X(MPD_API_PLAYLIST_CONTENT_LIST) \
    X(MPD_API_SMARTPLS_UPDATE_ALL) \
    X(MPD_API_SMARTPLS_SAVE) \
    X(MPD_API_SMARTPLS_GET) \
    X(MPD_API_DATABASE_SEARCH_ADV) \
    X(MPD_API_DATABASE_SEARCH) \
    X(MPD_API_DATABASE_UPDATE) \
    X(MPD_API_DATABASE_RESCAN) \
    X(MPD_API_DATABASE_FILESYSTEM_LIST) \
    X(MPD_API_DATABASE_TAG_LIST) \
    X(MPD_API_DATABASE_TAG_ALBUM_LIST) \
    X(MPD_API_DATABASE_TAG_ALBUM_TITLE_LIST) \
    X(MPD_API_DATABASE_STATS) \
    X(MPD_API_DATABASE_SONGDETAILS) \
    X(MPD_API_PLAYER_PLAY_TRACK) \
    X(MPD_API_PLAYER_VOLUME_SET) \
    X(MPD_API_PLAYER_VOLUME_GET) \
    X(MPD_API_PLAYER_PAUSE) \
    X(MPD_API_PLAYER_PLAY) \
    X(MPD_API_PLAYER_STOP) \
    X(MPD_API_PLAYER_SEEK) \
    X(MPD_API_PLAYER_NEXT) \
    X(MPD_API_PLAYER_PREV) \
    X(MPD_API_PLAYER_OUTPUT_LIST) \
    X(MPD_API_PLAYER_TOGGLE_OUTPUT) \
    X(MPD_API_PLAYER_CURRENT_SONG) \
    X(MPD_API_PLAYER_STATE) \
    X(MPD_API_SETTINGS_GET) \
    X(MPD_API_SETTINGS_SET) \
    X(MPD_API_WELCOME) \
    X(MPD_API_LIKE) \
    X(MPD_API_SYSCMD) \
    X(MPD_API_COLS_SAVE) \
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
    // Connection
    struct mpd_connection *conn;
    enum mpd_conn_states conn_state;
    int timeout;

    // Reponse Buffer
    char buf[MAX_SIZE];
    size_t buf_size;

    // States
    int song_id;
    int next_song_id;
    int last_song_id;
    unsigned queue_version;
    unsigned queue_length;
    int last_update_sticker_song_id;
    int last_last_played_id;
    
    // Features
    const unsigned* protocol;
    // Supported tags
    bool feat_sticker;
    bool feat_playlists;
    bool feat_tags;
    bool feat_library;
    bool feat_advsearch;
} mpd;

struct list mpd_tags;
struct list mympd_tags;
struct list mympd_searchtags;
struct list mympd_browsetags;
struct list last_played;
struct list syscmds;

typedef struct {
    long mpdport;
    const char *mpdhost;
    const char *mpdpass;
    const char *webport;
    bool ssl;
    const char *sslport;
    const char *sslcert;
    const char *sslkey;
    const char *user;
    bool coverimage;
    const char *coverimagename;
    bool stickers;
    bool mixramp;
    const char *taglist;
    const char *searchtaglist;
    const char *browsetaglist;
    bool smartpls;
    const char *varlibdir;
    const char *etcdir;
    unsigned long max_elements_per_page;
    bool syscmds;
    bool localplayer;
    long streamport;
    const char *streamurl;
    unsigned long last_played_count;
    long loglevel;
} t_config;

t_config config;

typedef struct {
    long playCount;
    long skipCount;
    long lastPlayed;
    long like;
} t_sticker;

typedef struct {
    bool notificationWeb;
    bool notificationPage;
    int jukeboxMode;
    const char *jukeboxPlaylist;
    int jukeboxQueueLength;
    char *colsQueueCurrent;
    char *colsSearch;
    char *colsBrowseDatabase;
    char *colsBrowsePlaylistsDetail;
    char *colsBrowseFilesystem;
    char *colsPlayback;
    char *colsQueueLastPlayed;
} t_mympd_state;

t_mympd_state mympd_state;

static int is_websocket(const struct mg_connection *nc) {
    return nc->flags & MG_F_IS_WEBSOCKET;
}

int randrange(int n);
void mympd_idle(struct mg_mgr *sm, int timeout);
void mympd_parse_idle(struct mg_mgr *s, int idle_bitmask);
void callback_mympd(struct mg_connection *nc, const struct mg_str msg);
void mympd_notify(struct mg_mgr *s);
void mympd_count_song_id(int song_id, char *name, int value);
void mympd_count_song_uri(const char *uri, char *name, int value);
void mympd_like_song_uri(const char *uri, int value);
void mympd_last_played_song_uri(const char *uri);
void mympd_last_played_song_id(int song_id);
void mympd_get_sticker(const char *uri, t_sticker *sticker);
void mympd_last_played_list(int song_id);
void mympd_jukebox();
bool mympd_state_get(char *name, char *value);
bool mympd_state_set(const char *name, const char *value);
int mympd_syscmd(char *buffer, char *cmd, int order);
int mympd_smartpls_save(char *smartpltype, char *playlist, char *tag, char *searchstr, int maxentries, int timerange);
int mympd_smartpls_put(char *buffer, char *playlist);
int mympd_smartpls_update_all();
int mympd_smartpls_clear(char *playlist);
int mympd_smartpls_update(char *sticker, char *playlist, int maxentries);
int mympd_smartpls_update_newest(char *playlist, int timerange);
int mympd_smartpls_update_search(char *playlist, char *tag, char *searchstr);
int mympd_get_updatedb_state(char *buffer);
void mympd_get_song_uri_from_song_id(int song_id, char *uri);
int mympd_put_state(char *buffer, int *current_song_id, int *next_song_id, int *last_song_id, unsigned *queue_version, unsigned *queue_length);
int mympd_put_outputs(char *buffer);
int mympd_put_current_song(char *buffer);
int mympd_put_queue(char *buffer, unsigned int offset, unsigned *queue_version, unsigned *queue_length);
int mympd_put_browse(char *buffer, char *path, unsigned int offset, char *filter);
int mympd_search(char *buffer, char *searchstr, char *filter, char *plist, unsigned int offset);
int mympd_search_adv(char *buffer, char *expression, char *sort, bool sortdesc, char *grouptag, char *plist, unsigned int offset);
int mympd_search_queue(char *buffer, char *mpdtagtype, unsigned int offset, char *searchstr);
int mympd_put_welcome(char *buffer);
int mympd_put_volume(char *buffer);
int mympd_put_stats(char *buffer);
int mympd_put_settings(char *buffer);
int mympd_put_db_tag(char *buffer, unsigned int offset, char *mpdtagtype, char *mpdsearchtagtype, char *searchstr, char *filter);
int mympd_put_songs_in_album(char *buffer, char *album, char *search, char *tag);
int mympd_put_playlists(char *buffer, unsigned int offset, char *filter);
int mympd_put_playlist_list(char *buffer, char *uri, unsigned int offset, char *filter);
int mympd_put_songdetails(char *buffer, char *uri);
int mympd_put_last_played_songs(char *buffer, unsigned int offset);
int mympd_queue_crop(char *buffer);
void mympd_disconnect();
#endif
