/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
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

#define RETURN_ERROR_AND_RECOVER(X) do { \
    printf("MPD %s: %s\n", X, mpd_connection_get_error_message(mpd.conn)); \
    len = json_printf(&out, "{type: error, data: %Q}", mpd_connection_get_error_message(mpd.conn)); \
    if (!mpd_connection_clear_error(mpd.conn)) \
        mpd.conn_state = MPD_FAILURE; \
    return len; \
} while (0)

#define LOG_ERROR_AND_RECOVER(X) do { \
    printf("MPD %s: %s\n", X, mpd_connection_get_error_message(mpd.conn)); \
    if (!mpd_connection_clear_error(mpd.conn)) \
        mpd.conn_state = MPD_FAILURE; \
} while (0)

#define PUT_SONG_TAGS() do { \
    struct node *current = mympd_tags.list; \
    int tagnr = 0; \
    while (current != NULL) { \
        if (tagnr ++) \
            len += json_printf(&out, ","); \
        len += json_printf(&out, "%Q: %Q", current->data, mpd_client_get_tag(song, mpd_tag_name_parse(current->data))); \
        current = current->next; \
    } \
    len += json_printf(&out, ", Duration: %d, uri: %Q", mpd_song_get_duration(song), mpd_song_get_uri(song)); \
} while (0)

#define PUT_MIN_SONG_TAGS() do { \
    len += json_printf(&out, "Title: %Q, Duration: %d, uri: %Q", mpd_client_get_tag(song, MPD_TAG_TITLE), mpd_song_get_duration(song), mpd_song_get_uri(song)); \
} while (0)


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

void mpd_client_idle(int timeout);
void mpd_client_parse_idle(int idle_bitmask);
void mpd_client_api(void *arg_request);
void mpd_client_notify(size_t n);
bool mpd_client_count_song_id(int song_id, char *name, int value);
bool mpd_client_count_song_uri(const char *uri, char *name, int value);
bool mpd_client_like_song_uri(const char *uri, int value);
bool mpd_client_last_played_song_uri(const char *uri);
bool mpd_client_last_played_song_id(int song_id);
bool mpd_client_get_sticker(const char *uri, t_sticker *sticker);
bool mpd_client_last_played_list(int song_id);
bool mpd_client_jukebox();
bool mpd_client_state_get(char *name, char *value);
bool mpd_client_state_set(const char *name, const char *value);
int mpd_client_smartpls_save(char *smartpltype, char *playlist, char *tag, char *searchstr, int maxentries, int timerange);
int mpd_client_smartpls_put(char *buffer, char *playlist);
int mpd_client_smartpls_update_all();
int mpd_client_smartpls_clear(char *playlist);
int mpd_client_smartpls_update_sticker(char *playlist, char *sticker, int maxentries);
int mpd_client_smartpls_update_newest(char *playlist, int timerange);
int mpd_client_smartpls_update_search(char *playlist, char *tag, char *searchstr);
int mpd_client_get_updatedb_state(char *buffer);
int mpd_client_put_state(char *buffer, int *current_song_id, int *next_song_id, int *last_song_id, unsigned *queue_version, unsigned *queue_length);
int mpd_client_put_outputs(char *buffer);
int mpd_client_put_current_song(char *buffer);
int mpd_client_put_queue(char *buffer, unsigned int offset, unsigned *queue_version, unsigned *queue_length);
int mpd_client_put_browse(char *buffer, char *path, unsigned int offset, char *filter);
int mpd_client_search(char *buffer, char *searchstr, char *filter, char *plist, unsigned int offset);
int mpd_client_search_adv(char *buffer, char *expression, char *sort, bool sortdesc, char *grouptag, char *plist, unsigned int offset);
int mpd_client_search_queue(char *buffer, char *mpdtagtype, unsigned int offset, char *searchstr);
int mpd_client_put_welcome(char *buffer);
int mpd_client_put_volume(char *buffer);
int mpd_client_put_stats(char *buffer);
int mpd_client_put_settings(char *buffer, void *arg_config);
int mpd_client_put_db_tag(char *buffer, unsigned int offset, char *mpdtagtype, char *mpdsearchtagtype, char *searchstr, char *filter);
int mpd_client_put_songs_in_album(char *buffer, char *album, char *search, char *tag);
int mpd_client_put_playlists(char *buffer, unsigned int offset, char *filter);
int mpd_client_put_playlist_list(char *buffer, char *uri, unsigned int offset, char *filter);
int mpd_client_put_songdetails(char *buffer, char *uri);
int mpd_client_put_last_played_songs(char *buffer, unsigned int offset);
int mpd_client_queue_crop(char *buffer);
void mpd_client_disconnect();
#endif
