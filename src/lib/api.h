/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_API_H
#define MYMPD_API_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"

#include <stdbool.h>

/**
 * myMPD api methods
 * all above INTERNAL_API_COUNT are internal
 * TOTAL_API_COUNT must be the last
 */
#define MYMPD_CMDS(X) \
    X(GENERAL_API_UNKNOWN) \
    X(GENERAL_API_NOT_READY) \
    X(INTERNAL_API_ALBUMART_BY_ALBUMID) \
    X(INTERNAL_API_ALBUMART_BY_URI) \
    X(INTERNAL_API_ALBUMCACHE_CREATED) \
    X(INTERNAL_API_ALBUMCACHE_ERROR) \
    X(INTERNAL_API_ALBUMCACHE_SKIPPED) \
    X(INTERNAL_API_JUKEBOX_CREATED) \
    X(INTERNAL_API_JUKEBOX_ERROR) \
    X(INTERNAL_API_JUKEBOX_REFILL) \
    X(INTERNAL_API_JUKEBOX_REFILL_ADD) \
    X(INTERNAL_API_RAW) \
    X(INTERNAL_API_SCRIPT_EXECUTE) \
    X(INTERNAL_API_SCRIPT_INIT) \
    X(INTERNAL_API_SCRIPT_POST_EXECUTE) \
    X(INTERNAL_API_STATE_SAVE) \
    X(INTERNAL_API_STICKER_FEATURES) \
    X(INTERNAL_API_TAGART) \
    X(INTERNAL_API_TIMER_STARTPLAY) \
    X(INTERNAL_API_TRIGGER_EVENT_EMIT) \
    X(INTERNAL_API_WEBSERVER_NOTIFY) \
    X(INTERNAL_API_WEBSERVER_READY) \
    X(INTERNAL_API_WEBSERVER_SETTINGS) \
    X(INTERNAL_API_COUNT) \
    X(MYMPD_API_CACHES_CREATE) \
    X(MYMPD_API_CHANNEL_SUBSCRIBE) \
    X(MYMPD_API_CHANNEL_UNSUBSCRIBE) \
    X(MYMPD_API_CHANNEL_LIST) \
    X(MYMPD_API_CHANNEL_MESSAGE_SEND) \
    X(MYMPD_API_CHANNEL_MESSAGES_READ) \
    X(MYMPD_API_CLOUD_RADIOBROWSER_NEWEST) \
    X(MYMPD_API_CLOUD_RADIOBROWSER_SEARCH) \
    X(MYMPD_API_CLOUD_RADIOBROWSER_SERVERLIST) \
    X(MYMPD_API_CLOUD_RADIOBROWSER_STATION_DETAIL) \
    X(MYMPD_API_CLOUD_RADIOBROWSER_CLICK_COUNT) \
    X(MYMPD_API_CLOUD_WEBRADIODB_COMBINED_GET) \
    X(MYMPD_API_CONNECTION_SAVE) \
    X(MYMPD_API_CACHE_DISK_CLEAR) \
    X(MYMPD_API_CACHE_DISK_CROP) \
    X(MYMPD_API_DATABASE_ALBUM_DETAIL) \
    X(MYMPD_API_DATABASE_ALBUM_LIST) \
    X(MYMPD_API_DATABASE_FILESYSTEM_LIST) \
    X(MYMPD_API_DATABASE_RESCAN) \
    X(MYMPD_API_DATABASE_SEARCH) \
    X(MYMPD_API_DATABASE_TAG_LIST) \
    X(MYMPD_API_DATABASE_UPDATE) \
    X(MYMPD_API_HOME_ICON_GET) \
    X(MYMPD_API_HOME_ICON_MOVE) \
    X(MYMPD_API_HOME_ICON_RM) \
    X(MYMPD_API_HOME_ICON_SAVE) \
    X(MYMPD_API_HOME_ICON_LIST) \
    X(MYMPD_API_JUKEBOX_APPEND_URIS) \
    X(MYMPD_API_JUKEBOX_CLEAR) \
    X(MYMPD_API_JUKEBOX_CLEARERROR) \
    X(MYMPD_API_JUKEBOX_LENGTH) \
    X(MYMPD_API_JUKEBOX_LIST) \
    X(MYMPD_API_JUKEBOX_RESTART) \
    X(MYMPD_API_JUKEBOX_RM) \
    X(MYMPD_API_LAST_PLAYED_LIST) \
    X(MYMPD_API_LIKE) \
    X(MYMPD_API_LOGLEVEL) \
    X(MYMPD_API_LYRICS_GET) \
    X(MYMPD_API_MOUNT_LIST) \
    X(MYMPD_API_MOUNT_MOUNT) \
    X(MYMPD_API_MOUNT_NEIGHBOR_LIST) \
    X(MYMPD_API_MOUNT_URLHANDLER_LIST) \
    X(MYMPD_API_MOUNT_UNMOUNT) \
    X(MYMPD_API_PARTITION_LIST) \
    X(MYMPD_API_PARTITION_NEW) \
    X(MYMPD_API_PARTITION_OUTPUT_MOVE) \
    X(MYMPD_API_PARTITION_RM) \
    X(MYMPD_API_PARTITION_SAVE) \
    X(MYMPD_API_PICTURE_LIST) \
    X(MYMPD_API_PLAYER_CLEARERROR) \
    X(MYMPD_API_PLAYER_CURRENT_SONG) \
    X(MYMPD_API_PLAYER_NEXT) \
    X(MYMPD_API_PLAYER_OPTIONS_SET) \
    X(MYMPD_API_PLAYER_OUTPUT_ATTRIBUTES_SET) \
    X(MYMPD_API_PLAYER_OUTPUT_GET) \
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
    X(MYMPD_API_PLAYER_VOLUME_CHANGE) \
    X(MYMPD_API_PLAYLIST_CONTENT_APPEND_SEARCH) \
    X(MYMPD_API_PLAYLIST_CONTENT_APPEND_URIS) \
    X(MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUMS) \
    X(MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUM_DISC) \
    X(MYMPD_API_PLAYLIST_CONTENT_CLEAR) \
    X(MYMPD_API_PLAYLIST_CONTENT_DEDUP) \
    X(MYMPD_API_PLAYLIST_CONTENT_DEDUP_ALL) \
    X(MYMPD_API_PLAYLIST_CONTENT_ENUMERATE) \
    X(MYMPD_API_PLAYLIST_CONTENT_INSERT_SEARCH) \
    X(MYMPD_API_PLAYLIST_CONTENT_INSERT_URIS) \
    X(MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUMS) \
    X(MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUM_DISC) \
    X(MYMPD_API_PLAYLIST_CONTENT_LIST) \
    X(MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION) \
    X(MYMPD_API_PLAYLIST_CONTENT_MOVE_TO_PLAYLIST) \
    X(MYMPD_API_PLAYLIST_CONTENT_REPLACE_SEARCH) \
    X(MYMPD_API_PLAYLIST_CONTENT_REPLACE_URIS) \
    X(MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUMS) \
    X(MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUM_DISC) \
    X(MYMPD_API_PLAYLIST_CONTENT_RM_POSITIONS) \
    X(MYMPD_API_PLAYLIST_CONTENT_RM_RANGE) \
    X(MYMPD_API_PLAYLIST_CONTENT_SHUFFLE) \
    X(MYMPD_API_PLAYLIST_CONTENT_SORT) \
    X(MYMPD_API_PLAYLIST_CONTENT_VALIDATE) \
    X(MYMPD_API_PLAYLIST_CONTENT_VALIDATE_ALL) \
    X(MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP) \
    X(MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP_ALL) \
    X(MYMPD_API_PLAYLIST_COPY) \
    X(MYMPD_API_PLAYLIST_LIST) \
    X(MYMPD_API_PLAYLIST_RENAME) \
    X(MYMPD_API_PLAYLIST_RM) \
    X(MYMPD_API_PLAYLIST_RM_ALL) \
    X(MYMPD_API_RATING) \
    X(MYMPD_API_PRESET_RM) \
    X(MYMPD_API_PRESET_APPLY) \
    X(MYMPD_API_QUEUE_ADD_RANDOM) \
    X(MYMPD_API_QUEUE_APPEND_PLAYLISTS) \
    X(MYMPD_API_QUEUE_APPEND_SEARCH) \
    X(MYMPD_API_QUEUE_APPEND_URIS) \
    X(MYMPD_API_QUEUE_APPEND_URI_TAGS) \
    X(MYMPD_API_QUEUE_APPEND_ALBUMS) \
    X(MYMPD_API_QUEUE_APPEND_ALBUM_DISC) \
    X(MYMPD_API_QUEUE_CLEAR) \
    X(MYMPD_API_QUEUE_CROP) \
    X(MYMPD_API_QUEUE_CROP_OR_CLEAR) \
    X(MYMPD_API_QUEUE_INSERT_PLAYLISTS) \
    X(MYMPD_API_QUEUE_INSERT_SEARCH) \
    X(MYMPD_API_QUEUE_INSERT_URIS) \
    X(MYMPD_API_QUEUE_INSERT_URI_TAGS) \
    X(MYMPD_API_QUEUE_INSERT_ALBUMS) \
    X(MYMPD_API_QUEUE_INSERT_ALBUM_DISC) \
    X(MYMPD_API_QUEUE_MOVE_POSITION) \
    X(MYMPD_API_QUEUE_MOVE_RELATIVE) \
    X(MYMPD_API_QUEUE_PRIO_SET) \
    X(MYMPD_API_QUEUE_PRIO_SET_HIGHEST) \
    X(MYMPD_API_QUEUE_REPLACE_PLAYLISTS) \
    X(MYMPD_API_QUEUE_REPLACE_SEARCH) \
    X(MYMPD_API_QUEUE_REPLACE_URIS) \
    X(MYMPD_API_QUEUE_REPLACE_URI_TAGS) \
    X(MYMPD_API_QUEUE_REPLACE_ALBUMS) \
    X(MYMPD_API_QUEUE_REPLACE_ALBUM_DISC) \
    X(MYMPD_API_QUEUE_RM_RANGE) \
    X(MYMPD_API_QUEUE_RM_IDS) \
    X(MYMPD_API_QUEUE_SAVE) \
    X(MYMPD_API_QUEUE_SEARCH) \
    X(MYMPD_API_QUEUE_SHUFFLE) \
    X(MYMPD_API_SCRIPT_EXECUTE) \
    X(MYMPD_API_SCRIPT_GET) \
    X(MYMPD_API_SCRIPT_LIST) \
    X(MYMPD_API_SCRIPT_RELOAD) \
    X(MYMPD_API_SCRIPT_RM) \
    X(MYMPD_API_SCRIPT_SAVE) \
    X(MYMPD_API_SCRIPT_VALIDATE) \
    X(MYMPD_API_SCRIPT_VAR_DELETE) \
    X(MYMPD_API_SCRIPT_VAR_LIST) \
    X(MYMPD_API_SCRIPT_VAR_SET) \
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
    X(MYMPD_API_SONG_COMMENTS) \
    X(MYMPD_API_SONG_DETAILS) \
    X(MYMPD_API_SONG_FINGERPRINT) \
    X(MYMPD_API_STATS) \
    X(MYMPD_API_TIMER_GET) \
    X(MYMPD_API_TIMER_LIST) \
    X(MYMPD_API_TIMER_RM) \
    X(MYMPD_API_TIMER_SAVE) \
    X(MYMPD_API_TIMER_TOGGLE) \
    X(MYMPD_API_TRIGGER_GET) \
    X(MYMPD_API_TRIGGER_LIST) \
    X(MYMPD_API_TRIGGER_RM) \
    X(MYMPD_API_TRIGGER_SAVE) \
    X(MYMPD_API_VIEW_SAVE) \
    X(MYMPD_API_WEBRADIO_FAVORITE_GET) \
    X(MYMPD_API_WEBRADIO_FAVORITE_LIST) \
    X(MYMPD_API_WEBRADIO_FAVORITE_RM) \
    X(MYMPD_API_WEBRADIO_FAVORITE_SAVE) \
    X(TOTAL_API_COUNT)

/**
 * Helper macros
 */
#define GEN_ENUM(X) X,
#define GEN_STR(X) #X,

/**
 * Enum of myMPD jsonrpc api methods
 */
enum mympd_cmd_ids {
    MYMPD_CMDS(GEN_ENUM)
};

/**
 * Response types
 */
enum work_response_types {
    RESPONSE_TYPE_DEFAULT = 0,       //!< Send message back to the mongoose connection
    RESPONSE_TYPE_NOTIFY_CLIENT,     //!< Send message to client identified by jsonrpc id
    RESPONSE_TYPE_NOTIFY_PARTITION,  //!< Send message to all clients in a specific partition
    RESPONSE_TYPE_PUSH_CONFIG,       //!< Internal message from myMPD API thread to webserver thread to push the configuration
    RESPONSE_TYPE_SCRIPT,            //!< Respond is for the script thread
    RESPONSE_TYPE_DISCARD,           //!< Response will be discarded
    RESPONSE_TYPE_RAW,               //!< Raw http message
    RESPONSE_TYPE_SCRIPT_DIALOG      //!< Script dialog
};

/**
 * Request types
 */
enum work_request_types {
    REQUEST_TYPE_DEFAULT = 0,       //!< Request is from a mongoose connection
    REQUEST_TYPE_SCRIPT,            //!< Request is from th script thread
    REQUEST_TYPE_NOTIFY_PARTITION,  //!< Send message to all clients in a specific partition
    REQUEST_TYPE_DISCARD            //!< Response will be discarded
};

/**
 * Struct for work request in the queue
 */
struct t_work_request {
    enum work_request_types type;  //!< request type
    unsigned long conn_id;         //!< mongoose connection id
    unsigned id;                   //!< the jsonrpc id
    enum mympd_cmd_ids cmd_id;     //!< the jsonrpc method as enum
    sds data;                      //!< full jsonrpc request
    void *extra;                   //!< extra data for the request
    sds partition;                 //!< mpd partition
};

/**
 * Struct for work responses in the queue
 */
struct t_work_response {
    enum work_response_types type;  //!< response type
    unsigned long conn_id;          //!< mongoose connection id
    unsigned id;                    //!< the jsonrpc id
    enum mympd_cmd_ids cmd_id;      //!< the jsonrpc method as enum
    sds data;                       //!< full jsonrpc response
    sds binary;                     //!< binary data for the response
    void *extra;                    //!< extra data for the response
    sds partition;                  //!< mpd partition
};

/**
 * Config data sent to webserver thread
 */
struct set_mg_user_data_request {
    sds music_directory;      //!< detected mpd music directory
    sds playlist_directory;   //!< configured mpd playlist directory
    sds coverimage_names;     //!< comma separated list of coverimage names
    sds thumbnail_names;      //!< comma separated list of coverimage thumbnail names
    bool feat_albumart;       //!< true if mpd supports the albumart protocol command
    sds mpd_host;             //!< configured mpd host
    struct t_list partitions; //!< partition specific settings
};

/**
 * Public functions
 */
enum mympd_cmd_ids get_cmd_id(const char *cmd);
const char *get_cmd_id_method_name(enum mympd_cmd_ids cmd_id);
bool is_protected_api_method(enum mympd_cmd_ids cmd_id);
bool is_public_api_method(enum mympd_cmd_ids cmd_id);
bool is_script_api_method(enum mympd_cmd_ids cmd_id);
bool is_mympd_only_api_method(enum mympd_cmd_ids cmd_id);
void ws_notify(sds message, const char *partition);
void ws_notify_client(sds message, unsigned request_id);
void ws_script_dialog(sds message, unsigned request_id);
struct t_work_response *create_response(struct t_work_request *request);
struct t_work_response *create_response_new(enum work_response_types type, unsigned long conn_id,
        unsigned request_id, enum mympd_cmd_ids cmd_id, const char *partition);
struct t_work_request *create_request(enum work_request_types type, unsigned long conn_id,
        unsigned request_id, enum mympd_cmd_ids cmd_id, const char *data, const char *partition);
void free_request(struct t_work_request *request);
void free_response(struct t_work_response *response);
bool push_response(struct t_work_response *response);
bool push_request(struct t_work_request *request, unsigned id);

#endif
