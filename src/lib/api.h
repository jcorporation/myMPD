/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD API handling
 */

#ifndef MYMPD_API_H
#define MYMPD_API_H

#include "dist/sds/sds.h"
#include "src/lib/list.h"
#include "src/lib/webradio.h"

#include <stdbool.h>

/**
 * myMPD api methods
 * Must be added to mympd_cmd_acl_entity also!
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
    X(INTERNAL_API_FOLDERART) \
    X(INTERNAL_API_JUKEBOX_CREATED) \
    X(INTERNAL_API_JUKEBOX_ERROR) \
    X(INTERNAL_API_JUKEBOX_REFILL) \
    X(INTERNAL_API_JUKEBOX_REFILL_ADD) \
    X(INTERNAL_API_PLAYLISTART) \
    X(INTERNAL_API_RAW) \
    X(INTERNAL_API_SCRIPT_EXECUTE) \
    X(INTERNAL_API_SCRIPT_INIT) \
    X(INTERNAL_API_SCRIPT_POST_EXECUTE) \
    X(INTERNAL_API_STATE_SAVE) \
    X(INTERNAL_API_STICKER_FEATURES) \
    X(INTERNAL_API_TAGART) \
    X(INTERNAL_API_TIMER_STARTPLAY) \
    X(INTERNAL_API_TRIGGER_EVENT_EMIT) \
    X(INTERNAL_API_WEBRADIODB_CREATED) \
    X(INTERNAL_API_WEBSERVER_NOTIFY) \
    X(INTERNAL_API_WEBSERVER_READY) \
    X(INTERNAL_API_WEBSERVER_SETTINGS) \
    X(MYMPD_API_CHANNEL_SUBSCRIBE) \
    X(MYMPD_API_CHANNEL_UNSUBSCRIBE) \
    X(MYMPD_API_CHANNEL_LIST) \
    X(MYMPD_API_CHANNEL_MESSAGE_SEND) \
    X(MYMPD_API_CHANNEL_MESSAGES_READ) \
    X(MYMPD_API_CONNECTION_SAVE) \
    X(MYMPD_API_CACHE_DISK_CLEAR) \
    X(MYMPD_API_CACHE_DISK_CROP) \
    X(MYMPD_API_CACHES_CREATE) \
    X(MYMPD_API_DATABASE_ALBUM_DETAIL) \
    X(MYMPD_API_DATABASE_ALBUM_LIST) \
    X(MYMPD_API_DATABASE_LIST_RANDOM) \
    X(MYMPD_API_DATABASE_FILESYSTEM_LIST) \
    X(MYMPD_API_DATABASE_RESCAN) \
    X(MYMPD_API_DATABASE_SEARCH) \
    X(MYMPD_API_DATABASE_TAG_LIST) \
    X(MYMPD_API_DATABASE_UPDATE) \
    X(MYMPD_API_HOME_ICON_GET) \
    X(MYMPD_API_HOME_ICON_LIST) \
    X(MYMPD_API_HOME_ICON_MOVE) \
    X(MYMPD_API_HOME_ICON_RM) \
    X(MYMPD_API_HOME_ICON_SAVE) \
    X(MYMPD_API_HOME_WIDGET_IFRAME_SAVE) \
    X(MYMPD_API_HOME_WIDGET_SCRIPT_SAVE) \
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
    X(MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUM_TAG) \
    X(MYMPD_API_PLAYLIST_CONTENT_CLEAR) \
    X(MYMPD_API_PLAYLIST_CONTENT_DEDUP) \
    X(MYMPD_API_PLAYLIST_CONTENT_DEDUP_ALL) \
    X(MYMPD_API_PLAYLIST_CONTENT_ENUMERATE) \
    X(MYMPD_API_PLAYLIST_CONTENT_INSERT_SEARCH) \
    X(MYMPD_API_PLAYLIST_CONTENT_INSERT_URIS) \
    X(MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUMS) \
    X(MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUM_TAG) \
    X(MYMPD_API_PLAYLIST_CONTENT_LIST) \
    X(MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION) \
    X(MYMPD_API_PLAYLIST_CONTENT_MOVE_TO_PLAYLIST) \
    X(MYMPD_API_PLAYLIST_CONTENT_REPLACE_SEARCH) \
    X(MYMPD_API_PLAYLIST_CONTENT_REPLACE_URIS) \
    X(MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUMS) \
    X(MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUM_TAG) \
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
    X(MYMPD_API_QUEUE_APPEND_PLAYLIST_RANGE) \
    X(MYMPD_API_QUEUE_APPEND_PLAYLISTS) \
    X(MYMPD_API_QUEUE_APPEND_SEARCH) \
    X(MYMPD_API_QUEUE_APPEND_URIS) \
    X(MYMPD_API_QUEUE_APPEND_URI_TAGS) \
    X(MYMPD_API_QUEUE_APPEND_URI_RESUME) \
    X(MYMPD_API_QUEUE_APPEND_ALBUMS) \
    X(MYMPD_API_QUEUE_APPEND_ALBUM_TAG) \
    X(MYMPD_API_QUEUE_APPEND_ALBUM_RANGE) \
    X(MYMPD_API_QUEUE_CLEAR) \
    X(MYMPD_API_QUEUE_CROP) \
    X(MYMPD_API_QUEUE_CROP_OR_CLEAR) \
    X(MYMPD_API_QUEUE_INSERT_PLAYLIST_RANGE) \
    X(MYMPD_API_QUEUE_INSERT_PLAYLISTS) \
    X(MYMPD_API_QUEUE_INSERT_SEARCH) \
    X(MYMPD_API_QUEUE_INSERT_URIS) \
    X(MYMPD_API_QUEUE_INSERT_URI_TAGS) \
    X(MYMPD_API_QUEUE_INSERT_URI_RESUME) \
    X(MYMPD_API_QUEUE_INSERT_ALBUMS) \
    X(MYMPD_API_QUEUE_INSERT_ALBUM_TAG) \
    X(MYMPD_API_QUEUE_INSERT_ALBUM_RANGE) \
    X(MYMPD_API_QUEUE_MOVE_POSITION) \
    X(MYMPD_API_QUEUE_MOVE_RELATIVE) \
    X(MYMPD_API_QUEUE_PRIO_SET) \
    X(MYMPD_API_QUEUE_PRIO_SET_HIGHEST) \
    X(MYMPD_API_QUEUE_REPLACE_PLAYLIST_RANGE) \
    X(MYMPD_API_QUEUE_REPLACE_PLAYLISTS) \
    X(MYMPD_API_QUEUE_REPLACE_SEARCH) \
    X(MYMPD_API_QUEUE_REPLACE_URIS) \
    X(MYMPD_API_QUEUE_REPLACE_URI_TAGS) \
    X(MYMPD_API_QUEUE_REPLACE_URI_RESUME) \
    X(MYMPD_API_QUEUE_REPLACE_ALBUMS) \
    X(MYMPD_API_QUEUE_REPLACE_ALBUM_TAG) \
    X(MYMPD_API_QUEUE_REPLACE_ALBUM_RANGE) \
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
    X(MYMPD_API_SCRIPT_TMP_DELETE) \
    X(MYMPD_API_SCRIPT_TMP_GET) \
    X(MYMPD_API_SCRIPT_TMP_LIST) \
    X(MYMPD_API_SCRIPT_TMP_SET) \
    X(MYMPD_API_SCRIPT_VALIDATE) \
    X(MYMPD_API_SCRIPT_VAR_DELETE) \
    X(MYMPD_API_SCRIPT_VAR_LIST) \
    X(MYMPD_API_SCRIPT_VAR_SET) \
    X(MYMPD_API_SCRIPT_VERIFY_SIG) \
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
    X(MYMPD_API_STICKER_DELETE) \
    X(MYMPD_API_STICKER_GET) \
    X(MYMPD_API_STICKER_FIND) \
    X(MYMPD_API_STICKER_LIST) \
    X(MYMPD_API_STICKER_NAMES) \
    X(MYMPD_API_STICKER_SET) \
    X(MYMPD_API_STICKER_INC) \
    X(MYMPD_API_STICKER_DEC) \
    X(MYMPD_API_STICKER_PLAYCOUNT) \
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
    X(MYMPD_API_WEBRADIO_FAVORITE_GET_BY_NAME) \
    X(MYMPD_API_WEBRADIO_FAVORITE_GET_BY_URI) \
    X(MYMPD_API_WEBRADIO_FAVORITE_RM) \
    X(MYMPD_API_WEBRADIO_FAVORITE_SAVE) \
    X(MYMPD_API_WEBRADIO_FAVORITE_SEARCH) \
    X(MYMPD_API_WEBRADIODB_RADIO_GET_BY_NAME) \
    X(MYMPD_API_WEBRADIODB_RADIO_GET_BY_URI) \
    X(MYMPD_API_WEBRADIODB_SEARCH) \
    X(MYMPD_API_WEBRADIODB_UPDATE) \
    X(TOTAL_API_COUNT)

/**
 * Macro to generate enum for the API methods
 */
#define GEN_ENUM(X) X,

/**
 * Macro to generate strings for the API methods
 */
#define GEN_STR(X) #X,

/**
 * Enum of myMPD jsonrpc api methods
 */
enum mympd_cmd_ids {
    MYMPD_CMDS(GEN_ENUM)
};

/**
 * myMPD ACL values
 */
enum mympd_cmd_acl_entity {
    API_INTERNAL = 0x1,            //!< Defines internal methods.
    API_PUBLIC = 0x2,              //!< Defines methods that are public.
    API_PROTECTED = 0x4,           //!< Defines methods that need authentication if a pin is set.
    API_SCRIPT = 0x8,              //!< Defines internal methods that are accessible by scripts.
    API_MPD_DISCONNECTED = 0x10,   //!< Defines methods that should be handled if MPD is disconnected.
    API_MYMPD_ONLY = 0x20,         //!< Defines methods that do not require to leave the mpd idle mode in the mympd_api thread.
    API_MYMPD_WORKER_ONLY = 0x40,  //!< Defines methods that do not require to create a mpd connection in the mympd_worker thread.
    API_INVALID = 0x80,            //!< API methods that should not be called.
    API_SCRIPT_THREAD = 0x100,     //!< API methods that are handled by the script thread.
};

/**
 * Response types
 */
enum work_response_types {
    RESPONSE_TYPE_DEFAULT = 0,       //!< Send message back to the mongoose connection
    RESPONSE_TYPE_NOTIFY_CLIENT,     //!< Send message to client identified by jsonrpc id
    RESPONSE_TYPE_NOTIFY_PARTITION,  //!< Send message to all clients in a specific partition
    RESPONSE_TYPE_PUSH_CONFIG,       //!< Internal message from myMPD API thread to webserver thread to push the configuration
    RESPONSE_TYPE_SCRIPT,            //!< Response is for the script thread
    RESPONSE_TYPE_DISCARD,           //!< Response will be discarded
    RESPONSE_TYPE_RAW,               //!< Raw http message
    RESPONSE_TYPE_SCRIPT_DIALOG,     //!< Script dialog
    RESPONSE_TYPE_REDIRECT           //!< Send a redirect
};

/**
 * Request types
 */
enum work_request_types {
    REQUEST_TYPE_DEFAULT = 0,       //!< Request is from a mongoose connection
    REQUEST_TYPE_SCRIPT,            //!< Request is from the script thread
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
    sds partition;                 //!< mpd partition
    void *extra;                   //!< extra data for the request
    void (*extra_free)(void *);    //!< Function pointer to free extra data
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
    sds partition;                  //!< mpd partition
    void *extra;                    //!< extra data for the response
    void (*extra_free)(void *);     //!< Function pointer to free extra data
};

/**
 * Config data sent to webserver thread
 */
struct set_mg_user_data_request {
    sds music_directory;                     //!< detected mpd music directory
    sds playlist_directory;                  //!< configured mpd playlist directory
    sds coverimage_names;                    //!< comma separated list of coverimage names
    sds thumbnail_names;                     //!< comma separated list of coverimage thumbnail names
    sds mpd_host;                            //!< configured mpd host
    struct t_list partitions;                //!< partition specific settings
    struct t_webradios *webradiodb;          //!< Pointer to webradiodb
    struct t_webradios *webradio_favorites;  //!< Pointer to webradio favorites
};

/**
 * Public functions
 */
enum mympd_cmd_ids get_cmd_id(const char *cmd);
const char *get_cmd_id_method_name(enum mympd_cmd_ids cmd_id);
bool check_cmd_acl(enum mympd_cmd_ids cmd_id, enum mympd_cmd_acl_entity ace);
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
