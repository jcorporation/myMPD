/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_H
#define MYMPD_API_H

#include "../../dist/sds/sds.h"
#include "mympd_queue.h"

#include <signal.h>
#include <stdbool.h>

//global variables
extern _Atomic int worker_threads;

//signal handler
extern sig_atomic_t s_signal_received;

//message queue
extern struct t_mympd_queue *web_server_queue;
extern struct t_mympd_queue *mympd_api_queue;
extern struct t_mympd_queue *mympd_script_queue;

//API cmds
//all above INTERNAL_API_COUNT are internal
//TOTAL_API_COUNT must be the last
#define MYMPD_CMDS(X) \
    X(GENERAL_API_UNKNOWN) \
    X(INTERNAL_API_ALBUMART) \
    X(INTERNAL_API_ALBUMCACHE_CREATED) \
    X(INTERNAL_API_CACHES_CREATE) \
    X(INTERNAL_API_SCRIPT_INIT) \
    X(INTERNAL_API_SCRIPT_POST_EXECUTE) \
    X(INTERNAL_API_STATE_SAVE) \
    X(INTERNAL_API_STICKERCACHE_CREATED) \
    X(INTERNAL_API_TIMER_STARTPLAY) \
    X(INTERNAL_API_WEBSERVER_NOTIFY) \
    X(INTERNAL_API_WEBSERVER_SETTINGS) \
    X(INTERNAL_API_COUNT) \
    X(MYMPD_API_CLOUD_RADIOBROWSER_NEWEST) \
    X(MYMPD_API_CLOUD_RADIOBROWSER_SEARCH) \
    X(MYMPD_API_CLOUD_RADIOBROWSER_SERVERLIST) \
    X(MYMPD_API_CLOUD_RADIOBROWSER_STATION_DETAIL) \
    X(MYMPD_API_CLOUD_RADIOBROWSER_CLICK_COUNT) \
    X(MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET) \
    X(MYMPD_API_COLS_SAVE) \
    X(MYMPD_API_CONNECTION_SAVE) \
    X(MYMPD_API_COVERCACHE_CLEAR) \
    X(MYMPD_API_COVERCACHE_CROP) \
    X(MYMPD_API_DATABASE_ALBUMS_GET) \
    X(MYMPD_API_DATABASE_COMMENTS) \
    X(MYMPD_API_DATABASE_FILESYSTEM_LIST) \
    X(MYMPD_API_DATABASE_FINGERPRINT) \
    X(MYMPD_API_DATABASE_RESCAN) \
    X(MYMPD_API_DATABASE_SEARCH) \
    X(MYMPD_API_DATABASE_SEARCH_ADV) \
    X(MYMPD_API_DATABASE_SONGDETAILS) \
    X(MYMPD_API_DATABASE_STATS) \
    X(MYMPD_API_DATABASE_TAG_ALBUM_TITLE_LIST) \
    X(MYMPD_API_DATABASE_TAG_LIST) \
    X(MYMPD_API_DATABASE_UPDATE) \
    X(MYMPD_API_HOME_ICON_GET) \
    X(MYMPD_API_HOME_ICON_MOVE) \
    X(MYMPD_API_HOME_ICON_RM) \
    X(MYMPD_API_HOME_ICON_SAVE) \
    X(MYMPD_API_HOME_LIST) \
    X(MYMPD_API_JUKEBOX_CLEAR) \
    X(MYMPD_API_JUKEBOX_LIST) \
    X(MYMPD_API_JUKEBOX_RM) \
    X(MYMPD_API_LIKE) \
    X(MYMPD_API_LOGLEVEL) \
    X(MYMPD_API_LYRICS_GET) \
    X(MYMPD_API_MESSAGE_SEND) \
    X(MYMPD_API_MOUNT_LIST) \
    X(MYMPD_API_MOUNT_MOUNT) \
    X(MYMPD_API_MOUNT_NEIGHBOR_LIST) \
    X(MYMPD_API_MOUNT_UNMOUNT) \
    X(MYMPD_API_PARTITION_LIST) \
    X(MYMPD_API_PARTITION_NEW) \
    X(MYMPD_API_PARTITION_OUTPUT_MOVE) \
    X(MYMPD_API_PARTITION_RM) \
    X(MYMPD_API_PARTITION_SWITCH) \
    X(MYMPD_API_PICTURE_LIST) \
    X(MYMPD_API_PLAYER_CLEARERROR) \
    X(MYMPD_API_PLAYER_CURRENT_SONG) \
    X(MYMPD_API_PLAYER_NEXT) \
    X(MYMPD_API_PLAYER_OPTIONS_SET) \
    X(MYMPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET) \
    X(MYMPD_API_PLAYER_OUTPUT_LIST) \
    X(MYMPD_API_PLAYER_OUTPUT_TOGGLE) \
    X(MYMPD_API_PLAYER_PAUSE) \
    X(MYMPD_API_PLAYER_PLAY) \
    X(MYMPD_API_PLAYER_PLAY_SONG) \
    X(MYMPD_API_PLAYER_PREV) \
    X(MYMPD_API_PLAYER_RESUME) \
    X(MYMPD_API_PLAYER_SEEK_CURRENT) \
    X(MYMPD_API_PLAYER_STATE) \
    X(MYMPD_API_PLAYER_STOP) \
    X(MYMPD_API_PLAYER_VOLUME_GET) \
    X(MYMPD_API_PLAYER_VOLUME_SET) \
    X(MYMPD_API_PLAYLIST_CONTENT_APPEND_SEARCH) \
    X(MYMPD_API_PLAYLIST_CONTENT_APPEND_URI) \
    X(MYMPD_API_PLAYLIST_CONTENT_CLEAR) \
    X(MYMPD_API_PLAYLIST_CONTENT_INSERT_SEARCH) \
    X(MYMPD_API_PLAYLIST_CONTENT_INSERT_URI) \
    X(MYMPD_API_PLAYLIST_CONTENT_LIST) \
    X(MYMPD_API_PLAYLIST_CONTENT_MOVE_SONG) \
    X(MYMPD_API_PLAYLIST_CONTENT_REPLACE_SEARCH) \
    X(MYMPD_API_PLAYLIST_CONTENT_REPLACE_URI) \
    X(MYMPD_API_PLAYLIST_CONTENT_RM_RANGE) \
    X(MYMPD_API_PLAYLIST_CONTENT_SHUFFLE) \
    X(MYMPD_API_PLAYLIST_CONTENT_SORT) \
    X(MYMPD_API_PLAYLIST_CONTENT_RM_SONG) \
    X(MYMPD_API_PLAYLIST_LIST) \
    X(MYMPD_API_PLAYLIST_RENAME) \
    X(MYMPD_API_PLAYLIST_RM) \
    X(MYMPD_API_PLAYLIST_RM_ALL) \
    X(MYMPD_API_QUEUE_ADD_RANDOM) \
    X(MYMPD_API_QUEUE_APPEND_PLAYLIST) \
    X(MYMPD_API_QUEUE_APPEND_SEARCH) \
    X(MYMPD_API_QUEUE_APPEND_URI) \
    X(MYMPD_API_QUEUE_CLEAR) \
    X(MYMPD_API_QUEUE_CROP) \
    X(MYMPD_API_QUEUE_CROP_OR_CLEAR) \
    X(MYMPD_API_QUEUE_INSERT_PLAYLIST) \
    X(MYMPD_API_QUEUE_INSERT_SEARCH) \
    X(MYMPD_API_QUEUE_INSERT_URI) \
    X(MYMPD_API_QUEUE_LAST_PLAYED) \
    X(MYMPD_API_QUEUE_LIST) \
    X(MYMPD_API_QUEUE_MOVE_SONG) \
    X(MYMPD_API_QUEUE_PRIO_SET) \
    X(MYMPD_API_QUEUE_PRIO_SET_HIGHEST) \
    X(MYMPD_API_QUEUE_REPLACE_PLAYLIST) \
    X(MYMPD_API_QUEUE_REPLACE_SEARCH) \
    X(MYMPD_API_QUEUE_REPLACE_URI) \
    X(MYMPD_API_QUEUE_RM_RANGE) \
    X(MYMPD_API_QUEUE_RM_SONG) \
    X(MYMPD_API_QUEUE_SAVE) \
    X(MYMPD_API_QUEUE_SEARCH) \
    X(MYMPD_API_QUEUE_SEARCH_ADV) \
    X(MYMPD_API_QUEUE_SHUFFLE) \
    X(MYMPD_API_SCRIPT_EXECUTE) \
    X(MYMPD_API_SCRIPT_GET) \
    X(MYMPD_API_SCRIPT_LIST) \
    X(MYMPD_API_SCRIPT_RM) \
    X(MYMPD_API_SCRIPT_SAVE) \
    X(MYMPD_API_SESSION_LOGIN) \
    X(MYMPD_API_SESSION_LOGOUT) \
    X(MYMPD_API_SESSION_VALIDATE) \
    X(MYMPD_API_SETTINGS_GET) \
    X(MYMPD_API_SETTINGS_SET) \
    X(MYMPD_API_SMARTPLS_GET) \
    X(MYMPD_API_SMARTPLS_NEWEST_SAVE) \
    X(MYMPD_API_SMARTPLS_SEARCH_SAVE) \
    X(MYMPD_API_SMARTPLS_STICKER_SAVE) \
    X(MYMPD_API_SMARTPLS_UPDATE) \
    X(MYMPD_API_SMARTPLS_UPDATE_ALL) \
    X(MYMPD_API_TIMER_GET) \
    X(MYMPD_API_TIMER_LIST) \
    X(MYMPD_API_TIMER_RM) \
    X(MYMPD_API_TIMER_SAVE) \
    X(MYMPD_API_TIMER_TOGGLE) \
    X(MYMPD_API_TRIGGER_GET) \
    X(MYMPD_API_TRIGGER_LIST) \
    X(MYMPD_API_TRIGGER_RM) \
    X(MYMPD_API_TRIGGER_SAVE) \
    X(MYMPD_API_URLHANDLERS) \
    X(MYMPD_API_WEBRADIO_FAVORITE_GET) \
    X(MYMPD_API_WEBRADIO_FAVORITE_LIST) \
    X(MYMPD_API_WEBRADIO_FAVORITE_RM) \
    X(MYMPD_API_WEBRADIO_FAVORITE_SAVE) \
    X(TOTAL_API_COUNT)

#define GEN_ENUM(X) X,
#define GEN_STR(X) #X,

enum mympd_cmd_ids {
    MYMPD_CMDS(GEN_ENUM)
};

struct t_work_request {
    long long conn_id; // needed to identify the connection where to send the reply
    long id; //the jsonrpc id
    sds method; //the jsonrpc method
    enum mympd_cmd_ids cmd_id;
    sds data;
    void *extra;
};

struct t_work_response {
    long long conn_id; // needed to identify the connection where to send the reply
    long id; //the jsonrpc id
    sds method; //the jsonrpc method
    enum mympd_cmd_ids cmd_id;
    sds data;
    sds binary;
    void *extra;
};

//config data sent to webserver thread
struct set_mg_user_data_request {
    sds music_directory;
    sds playlist_directory;
    sds coverimage_names;
    sds thumbnail_names;
    bool feat_mpd_albumart;
    sds mpd_host;
    unsigned mpd_stream_port;
    bool covercache;
};

//public functions
enum mympd_cmd_ids get_cmd_id(const char *cmd);
const char *get_cmd_id_method_name(enum mympd_cmd_ids cmd_id);
bool is_protected_api_method(enum mympd_cmd_ids cmd_id);
bool is_public_api_method(enum mympd_cmd_ids cmd_id);
bool is_mympd_only_api_method(enum mympd_cmd_ids cmd_id);
void ws_notify(sds message);
struct t_work_response *create_response(struct t_work_request *request);
struct t_work_response *create_response_new(long long conn_id, long request_id, enum mympd_cmd_ids cmd_id);
struct t_work_request *create_request(long long conn_id, long request_id, enum mympd_cmd_ids cmd_id, const char *data);
void free_request(struct t_work_request *request);
void free_response(struct t_work_response *response);

#endif
