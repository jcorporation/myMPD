/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief myMPD API handling
 */

#include "compile_time.h"
#include "src/lib/api.h"

#include "src/lib/log.h"
#include "src/lib/mem.h"
#include "src/lib/msg_queue.h"
#include "src/lib/sds_extras.h"

#include <string.h>

/**
 * myMPD API methods as strings
 */
static const char *mympd_cmd_strs[] = { MYMPD_CMDS(GEN_STR) };

/**
 * ACL for myMPD API methods
 */
static enum mympd_cmd_acl_entity mympd_cmd_acl[] = {
    [GENERAL_API_UNKNOWN] = API_INTERNAL | API_INVALID,
    [GENERAL_API_NOT_READY] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_ALBUMART_BY_ALBUMID] = API_INTERNAL,
    [INTERNAL_API_ALBUMART_BY_URI] = API_INTERNAL,
    [INTERNAL_API_ALBUMCACHE_CREATED] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_ALBUMCACHE_ERROR] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_ALBUMCACHE_SKIPPED] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_FOLDERART] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_JUKEBOX_CREATED] = API_INTERNAL | API_SCRIPT | API_MYMPD_ONLY,
    [INTERNAL_API_JUKEBOX_ERROR] = API_INTERNAL | API_SCRIPT | API_MYMPD_ONLY,
    [INTERNAL_API_JUKEBOX_REFILL] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_JUKEBOX_REFILL_ADD] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_PLAYLISTART] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_RAW] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_SCRIPT_EXECUTE] = API_INTERNAL | API_SCRIPT_THREAD,
    [INTERNAL_API_SCRIPT_INIT] = API_INTERNAL | API_SCRIPT,
    [INTERNAL_API_SCRIPT_POST_EXECUTE] = API_INTERNAL | API_SCRIPT_THREAD,
    [INTERNAL_API_STATE_SAVE] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_STICKER_FEATURES] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_TAGART] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_TIMER_STARTPLAY] = API_INTERNAL,
    [INTERNAL_API_TRIGGER_EVENT_EMIT] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_WEBRADIODB_CREATED] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_WEBSERVER_NOTIFY] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_WEBSERVER_READY] = API_INTERNAL | API_MYMPD_ONLY,
    [INTERNAL_API_WEBSERVER_SETTINGS] = API_INTERNAL | API_MYMPD_ONLY,
    [MYMPD_API_CHANNEL_SUBSCRIBE] = API_PUBLIC,
    [MYMPD_API_CHANNEL_UNSUBSCRIBE] = API_PUBLIC,
    [MYMPD_API_CHANNEL_LIST] = API_PUBLIC,
    [MYMPD_API_CHANNEL_MESSAGE_SEND] = API_PUBLIC,
    [MYMPD_API_CHANNEL_MESSAGES_READ] = API_PUBLIC,
    [MYMPD_API_CONNECTION_SAVE] = API_PUBLIC | API_PROTECTED | API_MPD_DISCONNECTED,
    [MYMPD_API_CACHE_DISK_CLEAR] = API_PUBLIC | API_PROTECTED | API_MYMPD_ONLY | API_MYMPD_WORKER_ONLY,
    [MYMPD_API_CACHE_DISK_CROP] = API_PUBLIC | API_PROTECTED | API_MYMPD_ONLY | API_MYMPD_WORKER_ONLY,
    [MYMPD_API_CACHES_CREATE] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_DATABASE_ALBUM_DETAIL] = API_PUBLIC,
    [MYMPD_API_DATABASE_ALBUM_LIST] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_DATABASE_LIST_RANDOM] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_DATABASE_FILESYSTEM_LIST] = API_PUBLIC,
    [MYMPD_API_DATABASE_RESCAN] = API_PUBLIC,
    [MYMPD_API_DATABASE_SEARCH] = API_PUBLIC,
    [MYMPD_API_DATABASE_TAG_LIST] = API_PUBLIC,
    [MYMPD_API_DATABASE_UPDATE] = API_PUBLIC,
    [MYMPD_API_HOME_ICON_GET] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_HOME_ICON_LIST] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_HOME_ICON_MOVE] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_HOME_ICON_RM] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_HOME_ICON_SAVE] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_HOME_WIDGET_IFRAME_SAVE] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_HOME_WIDGET_SCRIPT_SAVE] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_JUKEBOX_APPEND_URIS] = API_SCRIPT | API_MYMPD_ONLY,
    [MYMPD_API_JUKEBOX_CLEAR] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_JUKEBOX_CLEARERROR] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_JUKEBOX_LENGTH] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_JUKEBOX_LIST] = API_PUBLIC,
    [MYMPD_API_JUKEBOX_RESTART] = API_PUBLIC,
    [MYMPD_API_JUKEBOX_RM] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_LAST_PLAYED_LIST] = API_PUBLIC,
    [MYMPD_API_LIKE] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_LOGLEVEL] = API_PUBLIC | API_PROTECTED | API_MYMPD_ONLY,
    [MYMPD_API_LYRICS_GET] = API_PUBLIC,
    [MYMPD_API_MOUNT_LIST] = API_PUBLIC,
    [MYMPD_API_MOUNT_MOUNT] = API_PUBLIC | API_PROTECTED,
    [MYMPD_API_MOUNT_NEIGHBOR_LIST] = API_PUBLIC,
    [MYMPD_API_MOUNT_URLHANDLER_LIST] = API_PUBLIC,
    [MYMPD_API_MOUNT_UNMOUNT] = API_PUBLIC | API_PROTECTED,
    [MYMPD_API_PARTITION_LIST] = API_PUBLIC,
    [MYMPD_API_PARTITION_NEW] = API_PUBLIC | API_PROTECTED,
    [MYMPD_API_PARTITION_OUTPUT_MOVE] = API_PUBLIC | API_PROTECTED,
    [MYMPD_API_PARTITION_RM] = API_PUBLIC | API_PROTECTED,
    [MYMPD_API_PARTITION_SAVE] = API_PUBLIC | API_PROTECTED,
    [MYMPD_API_PICTURE_LIST] = API_PUBLIC,
    [MYMPD_API_PLAYER_CLEARERROR] = API_PUBLIC,
    [MYMPD_API_PLAYER_CURRENT_SONG] = API_PUBLIC,
    [MYMPD_API_PLAYER_NEXT] = API_PUBLIC,
    [MYMPD_API_PLAYER_OPTIONS_SET] = API_PUBLIC,
    [MYMPD_API_PLAYER_OUTPUT_ATTRIBUTES_SET] = API_PUBLIC | API_PROTECTED,
    [MYMPD_API_PLAYER_OUTPUT_GET] = API_PUBLIC,
    [MYMPD_API_PLAYER_OUTPUT_LIST] = API_PUBLIC,
    [MYMPD_API_PLAYER_OUTPUT_TOGGLE] = API_PUBLIC,
    [MYMPD_API_PLAYER_PAUSE] = API_PUBLIC,
    [MYMPD_API_PLAYER_PLAY] = API_PUBLIC,
    [MYMPD_API_PLAYER_PLAY_SONG] = API_PUBLIC,
    [MYMPD_API_PLAYER_PREV] = API_PUBLIC,
    [MYMPD_API_PLAYER_RESUME] = API_PUBLIC,
    [MYMPD_API_PLAYER_SEEK_CURRENT] = API_PUBLIC,
    [MYMPD_API_PLAYER_STATE] = API_PUBLIC,
    [MYMPD_API_PLAYER_STOP] = API_PUBLIC,
    [MYMPD_API_PLAYER_VOLUME_GET] = API_PUBLIC,
    [MYMPD_API_PLAYER_VOLUME_SET] = API_PUBLIC,
    [MYMPD_API_PLAYER_VOLUME_CHANGE] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_APPEND_SEARCH] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_APPEND_URIS] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUMS] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_APPEND_ALBUM_TAG] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_CLEAR] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_DEDUP] = API_PUBLIC | API_MYMPD_ONLY,  // Handled in a worker thread
    [MYMPD_API_PLAYLIST_CONTENT_DEDUP_ALL] = API_PUBLIC | API_MYMPD_ONLY,  // Handled in a worker thread
    [MYMPD_API_PLAYLIST_CONTENT_ENUMERATE] = API_PUBLIC | API_MYMPD_ONLY,  // Handled in a worker thread
    [MYMPD_API_PLAYLIST_CONTENT_INSERT_SEARCH] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_INSERT_URIS] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUMS] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_INSERT_ALBUM_TAG] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_LIST] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_MOVE_POSITION] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_MOVE_TO_PLAYLIST] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_REPLACE_SEARCH] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_REPLACE_URIS] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUMS] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_REPLACE_ALBUM_TAG] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_RM_POSITIONS] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_RM_RANGE] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_CONTENT_SHUFFLE] = API_PUBLIC | API_MYMPD_ONLY,  // Handled in a worker thread
    [MYMPD_API_PLAYLIST_CONTENT_SORT] = API_PUBLIC | API_MYMPD_ONLY,  // Handled in a worker thread
    [MYMPD_API_PLAYLIST_CONTENT_VALIDATE] = API_PUBLIC | API_MYMPD_ONLY,  // Handled in a worker thread
    [MYMPD_API_PLAYLIST_CONTENT_VALIDATE_ALL] = API_PUBLIC | API_MYMPD_ONLY,  // Handled in a worker thread
    [MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP] = API_PUBLIC | API_MYMPD_ONLY,  // Handled in a worker thread
    [MYMPD_API_PLAYLIST_CONTENT_VALIDATE_DEDUP_ALL] = API_PUBLIC | API_MYMPD_ONLY,  // Handled in a worker thread
    [MYMPD_API_PLAYLIST_COPY] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_LIST] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_RENAME] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_RM] = API_PUBLIC,
    [MYMPD_API_PLAYLIST_RM_ALL] = API_PUBLIC | API_PROTECTED,
    [MYMPD_API_RATING] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_PRESET_RM] = API_PUBLIC,
    [MYMPD_API_PRESET_APPLY] = API_PUBLIC,
    [MYMPD_API_QUEUE_ADD_RANDOM] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_QUEUE_APPEND_PLAYLIST_RANGE] = API_PUBLIC,
    [MYMPD_API_QUEUE_APPEND_PLAYLISTS] = API_PUBLIC,
    [MYMPD_API_QUEUE_APPEND_SEARCH] = API_PUBLIC,
    [MYMPD_API_QUEUE_APPEND_URIS] = API_PUBLIC,
    [MYMPD_API_QUEUE_APPEND_URI_TAGS] = API_PUBLIC,
    [MYMPD_API_QUEUE_APPEND_URI_RESUME] = API_PUBLIC,
    [MYMPD_API_QUEUE_APPEND_ALBUMS] = API_PUBLIC,
    [MYMPD_API_QUEUE_APPEND_ALBUM_TAG] = API_PUBLIC,
    [MYMPD_API_QUEUE_APPEND_ALBUM_RANGE] = API_PUBLIC,
    [MYMPD_API_QUEUE_CLEAR] = API_PUBLIC,
    [MYMPD_API_QUEUE_CROP] = API_PUBLIC,
    [MYMPD_API_QUEUE_CROP_OR_CLEAR] = API_PUBLIC,
    [MYMPD_API_QUEUE_INSERT_PLAYLIST_RANGE] = API_PUBLIC,
    [MYMPD_API_QUEUE_INSERT_PLAYLISTS] = API_PUBLIC,
    [MYMPD_API_QUEUE_INSERT_SEARCH] = API_PUBLIC,
    [MYMPD_API_QUEUE_INSERT_URIS] = API_PUBLIC,
    [MYMPD_API_QUEUE_INSERT_URI_TAGS] = API_PUBLIC,
    [MYMPD_API_QUEUE_INSERT_URI_RESUME] = API_PUBLIC,
    [MYMPD_API_QUEUE_INSERT_ALBUMS] = API_PUBLIC,
    [MYMPD_API_QUEUE_INSERT_ALBUM_TAG] = API_PUBLIC,
    [MYMPD_API_QUEUE_INSERT_ALBUM_RANGE] = API_PUBLIC,
    [MYMPD_API_QUEUE_MOVE_POSITION] = API_PUBLIC,
    [MYMPD_API_QUEUE_MOVE_RELATIVE] = API_PUBLIC,
    [MYMPD_API_QUEUE_PRIO_SET] = API_PUBLIC,
    [MYMPD_API_QUEUE_PRIO_SET_HIGHEST] = API_PUBLIC,
    [MYMPD_API_QUEUE_REPLACE_PLAYLIST_RANGE] = API_PUBLIC,
    [MYMPD_API_QUEUE_REPLACE_PLAYLISTS] = API_PUBLIC,
    [MYMPD_API_QUEUE_REPLACE_SEARCH] = API_PUBLIC,
    [MYMPD_API_QUEUE_REPLACE_URIS] = API_PUBLIC,
    [MYMPD_API_QUEUE_REPLACE_URI_TAGS] = API_PUBLIC,
    [MYMPD_API_QUEUE_REPLACE_URI_RESUME] = API_PUBLIC,
    [MYMPD_API_QUEUE_REPLACE_ALBUMS] = API_PUBLIC,
    [MYMPD_API_QUEUE_REPLACE_ALBUM_TAG] = API_PUBLIC,
    [MYMPD_API_QUEUE_REPLACE_ALBUM_RANGE] = API_PUBLIC,
    [MYMPD_API_QUEUE_RM_RANGE] = API_PUBLIC,
    [MYMPD_API_QUEUE_RM_IDS] = API_PUBLIC,
    [MYMPD_API_QUEUE_SAVE] = API_PUBLIC,
    [MYMPD_API_QUEUE_SEARCH] = API_PUBLIC,
    [MYMPD_API_QUEUE_SHUFFLE] = API_PUBLIC,
    [MYMPD_API_SCRIPT_EXECUTE] = API_PUBLIC | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_GET] = API_PUBLIC | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_LIST] = API_PUBLIC | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_RELOAD] = API_PUBLIC | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_RM] = API_PUBLIC | API_PROTECTED | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_SAVE] = API_PUBLIC | API_PROTECTED | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_TMP_DELETE] = API_PUBLIC | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_TMP_GET] = API_PUBLIC | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_TMP_LIST] = API_PUBLIC | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_TMP_SET] = API_PUBLIC | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_VALIDATE] = API_PUBLIC | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_VAR_DELETE] = API_PUBLIC | API_PROTECTED | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_VAR_LIST] = API_PUBLIC | API_PROTECTED | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_VAR_SET] = API_PUBLIC | API_PROTECTED | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SCRIPT_VERIFY_SIG] = API_PUBLIC | API_SCRIPT_THREAD,  // This method is handled in the scripts thread
    [MYMPD_API_SESSION_LOGIN] = API_PUBLIC,  // This method is handled in the webserver thread
    [MYMPD_API_SESSION_LOGOUT] = API_PUBLIC | API_PROTECTED,  // This method is handled in the webserver thread
    [MYMPD_API_SESSION_VALIDATE] = API_PUBLIC | API_PROTECTED,  // This method is handled in the webserver thread
    [MYMPD_API_SETTINGS_GET] = API_PUBLIC | API_MPD_DISCONNECTED,
    [MYMPD_API_SETTINGS_SET] = API_PUBLIC | API_PROTECTED,
    [MYMPD_API_SMARTPLS_GET] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_SMARTPLS_NEWEST_SAVE] = API_PUBLIC | API_MYMPD_ONLY,  // Smartpls updates are handled in a worker thread
    [MYMPD_API_SMARTPLS_SEARCH_SAVE] = API_PUBLIC | API_MYMPD_ONLY,  // Smartpls updates are handled in a worker thread
    [MYMPD_API_SMARTPLS_STICKER_SAVE] = API_PUBLIC | API_MYMPD_ONLY,  // Smartpls updates are handled in a worker thread
    [MYMPD_API_SMARTPLS_UPDATE] = API_PUBLIC | API_MYMPD_ONLY,  // Smartpls updates are handled in a worker thread
    [MYMPD_API_SMARTPLS_UPDATE_ALL] = API_PUBLIC | API_MYMPD_ONLY,  // Smartpls updates are handled in a worker thread
    [MYMPD_API_SONG_COMMENTS] = API_PUBLIC,
    [MYMPD_API_SONG_DETAILS] = API_PUBLIC,
    [MYMPD_API_SONG_FINGERPRINT] = API_PUBLIC | API_MYMPD_ONLY,  // Handled in a worker thread
    [MYMPD_API_STATS] = API_PUBLIC,
    [MYMPD_API_STICKER_DELETE] = API_PUBLIC | API_MYMPD_ONLY,  // Stickers are handled by a different MPD connection
    [MYMPD_API_STICKER_GET] = API_PUBLIC | API_MYMPD_ONLY,  // Stickers are handled by a different MPD connection
    [MYMPD_API_STICKER_FIND] = API_PUBLIC | API_MYMPD_ONLY,  // Stickers are handled by a different MPD connection
    [MYMPD_API_STICKER_LIST] = API_PUBLIC | API_MYMPD_ONLY,  // Stickers are handled by a different MPD connection
    [MYMPD_API_STICKER_NAMES] = API_PUBLIC | API_MYMPD_ONLY,  // Stickers are handled by a different MPD connection
    [MYMPD_API_STICKER_SET] = API_PUBLIC | API_MYMPD_ONLY,  // Stickers are handled by a different MPD connection
    [MYMPD_API_STICKER_INC] = API_PUBLIC | API_MYMPD_ONLY,  // Stickers are handled by a different MPD connection
    [MYMPD_API_STICKER_DEC] = API_PUBLIC | API_MYMPD_ONLY,  // Stickers are handled by a different MPD connection
    [MYMPD_API_STICKER_PLAYCOUNT] = API_PUBLIC | API_MYMPD_ONLY,  // Stickers are handled by a different MPD connection
    [MYMPD_API_TIMER_GET] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_TIMER_LIST] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_TIMER_RM] = API_PUBLIC | API_MYMPD_ONLY | API_PROTECTED,
    [MYMPD_API_TIMER_SAVE] = API_PUBLIC | API_MYMPD_ONLY | API_PROTECTED,
    [MYMPD_API_TIMER_TOGGLE] = API_PUBLIC | API_MYMPD_ONLY | API_PROTECTED,
    [MYMPD_API_TRIGGER_GET] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_TRIGGER_LIST] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_TRIGGER_RM] = API_PUBLIC | API_MYMPD_ONLY | API_PROTECTED,
    [MYMPD_API_TRIGGER_SAVE] = API_PUBLIC | API_MYMPD_ONLY | API_PROTECTED,
    [MYMPD_API_VIEW_SAVE] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_WEBRADIO_FAVORITE_GET_BY_NAME] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_WEBRADIO_FAVORITE_GET_BY_URI] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_WEBRADIO_FAVORITE_RM] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_WEBRADIO_FAVORITE_SAVE] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_WEBRADIO_FAVORITE_SEARCH] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_WEBRADIODB_RADIO_GET_BY_NAME] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_WEBRADIODB_RADIO_GET_BY_URI] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_WEBRADIODB_SEARCH] = API_PUBLIC | API_MYMPD_ONLY,
    [MYMPD_API_WEBRADIODB_UPDATE] = API_PUBLIC | API_MYMPD_ONLY | API_MYMPD_WORKER_ONLY,
    [TOTAL_API_COUNT] = API_INTERNAL | API_INVALID,
};

/**
 * Compile time initialization check
 */
#define IFV_N (sizeof mympd_cmd_acl/sizeof mympd_cmd_acl[0])
_Static_assert(IFV_N == TOTAL_API_COUNT + 1, "Unexpected size");

/**
 * Converts a string to the mympd_cmd_ids enum
 * @param cmd string to convert
 * @return enum mympd_cmd_ids
 */
enum mympd_cmd_ids get_cmd_id(const char *cmd) {
    for (unsigned i = 0; i < TOTAL_API_COUNT; i++) {
        if (strcmp(cmd, mympd_cmd_strs[i]) == 0) {
            return i;
        }
    }
    return GENERAL_API_UNKNOWN;
}

/**
 * Converts the mympd_cmd_ids enum to the string
 * @param cmd_id myMPD API method
 * @return the API method as string
 */
const char *get_cmd_id_method_name(enum mympd_cmd_ids cmd_id) {
    if ((unsigned)cmd_id >= TOTAL_API_COUNT) {
        return NULL;
    }
    return mympd_cmd_strs[cmd_id];
}

/**
 * Checks the myMPD method ACL
 * @param cmd_id method to check
 * @param ace Access method
 * @return bool true on success, else false
 */
bool check_cmd_acl(enum mympd_cmd_ids cmd_id, enum mympd_cmd_acl_entity ace) {
    if ((unsigned)cmd_id > TOTAL_API_COUNT) {
        return false;
    }
    return mympd_cmd_acl[cmd_id] & ace;
}

/**
 * Sends a websocket message to all clients in a partition
 * @param message the message to send
 * @param partition mpd partition
 */
void ws_notify(sds message, const char *partition) {
    MYMPD_LOG_DEBUG(partition, "Push websocket notify to queue: \"%s\"", message);
    struct t_work_response *response = create_response_new(RESPONSE_TYPE_NOTIFY_PARTITION, 0, 0, INTERNAL_API_WEBSERVER_NOTIFY, partition);
    response->data = sds_replace(response->data, message);
    mympd_queue_push(webserver_queue, response, 0);
}

/**
 * Sends a websocket message to a client
 * @param message the message to send
 * @param request_id the jsonrpc id of the client
 */
void ws_notify_client(sds message, unsigned request_id) {
    MYMPD_LOG_DEBUG(NULL, "Push websocket notify to queue: \"%s\"", message);
    struct t_work_response *response = create_response_new(RESPONSE_TYPE_NOTIFY_CLIENT, 0, request_id, INTERNAL_API_WEBSERVER_NOTIFY, MPD_PARTITION_ALL);
    response->data = sds_replace(response->data, message);
    mympd_queue_push(webserver_queue, response, 0);
}

/**
 * Sends a websocket message to a client to display a dialog.
 * @param message The message to send
 * @param request_id the jsonrpc id of the client
 */
void ws_script_dialog(sds message, unsigned request_id) {
    MYMPD_LOG_DEBUG(NULL, "Push websocket notify to queue: \"%s\"", message);
    struct t_work_response *response = create_response_new(RESPONSE_TYPE_SCRIPT_DIALOG, 0, request_id, INTERNAL_API_WEBSERVER_NOTIFY, MPD_PARTITION_ALL);
    response->data = sds_replace(response->data, message);
    mympd_queue_push(webserver_queue, response, 0);
}

/**
 * Mallocs and initializes a t_work_response struct, as reply of the provided request
 * @param request the request the ids are copied
 * @return the initialized t_work_response struct
 */
struct t_work_response *create_response(struct t_work_request *request) {
    enum work_response_types type = RESPONSE_TYPE_DEFAULT;
    switch(request->type) {
        case REQUEST_TYPE_DEFAULT: type = RESPONSE_TYPE_DEFAULT; break;
        case REQUEST_TYPE_SCRIPT:  type = RESPONSE_TYPE_SCRIPT; break;
        case REQUEST_TYPE_NOTIFY_PARTITION: type = RESPONSE_TYPE_NOTIFY_PARTITION; break;
        case REQUEST_TYPE_DISCARD: type = RESPONSE_TYPE_DISCARD; break;
    }
    struct t_work_response *response = create_response_new(type, request->conn_id, request->id, request->cmd_id, request->partition);
    return response;
}

/**
 * Mallocs and initializes a t_work_response struct
 * @param type work response type
 * @param conn_id connection id (from webserver)
 * @param request_id id for the request
 * @param cmd_id myMPD API method
 * @param partition mpd partition
 * @return the initialized t_work_response struct
 */
struct t_work_response *create_response_new(enum work_response_types type, unsigned long conn_id,
        unsigned request_id, enum mympd_cmd_ids cmd_id, const char *partition)
{
    struct t_work_response *response = malloc_assert(sizeof(struct t_work_response));
    response->type = type;
    response->conn_id = conn_id;
    response->id = request_id;
    response->cmd_id = cmd_id;
    response->data = sdsempty();
    response->partition = sdsnew(partition);
    response->extra = NULL;
    response->extra_free = NULL;
    return response;
}

/**
 * Mallocs and initializes a t_work_request struct
 * @param type work request type
 * @param conn_id connection id (from webserver)
 * @param request_id id for the request
 * @param cmd_id myMPD API method
 * @param data jsonrpc request, if NULL the start of a jsonrpc request is added,
 *             use a empty string to assign NULL, else a sds string is malloced from data
 * @param partition mpd partition
 * @return the initialized t_work_request struct
 */
struct t_work_request *create_request(enum work_request_types type, unsigned long conn_id,
        unsigned request_id, enum mympd_cmd_ids cmd_id, const char *data, const char *partition)
{
    struct t_work_request *request = malloc_assert(sizeof(struct t_work_request));
    request->type = type;
    request->conn_id = conn_id;
    request->cmd_id = cmd_id;
    request->id = request_id;
    if (data == NULL) {
        request->data = sdscatfmt(sdsempty(), "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"%s\",\"params\":{", get_cmd_id_method_name(cmd_id));
    }
    else if (data[0] == '\0') {
        request->data = NULL;
    }
    else {
        request->data = sdsnew(data);
    }
    request->partition = sdsnew(partition);
    request->extra = NULL;
    request->extra_free = NULL;
    return request;
}

/**
 * Frees the request struct
 * @param request request struct to free
 */
void free_request(struct t_work_request *request) {
    if (request == NULL) {
        return;
    }
    FREE_SDS(request->data);
    FREE_SDS(request->partition);
    if (request->extra != NULL) {
        if (request->extra_free != NULL) {
            request->extra_free(request->extra);
        }
        else {
            MYMPD_LOG_WARN(NULL, "Extra data for request %s not freed", get_cmd_id_method_name(request->cmd_id));
        }
    }
    FREE_PTR(request);
}

/**
 * Frees the response struct
 * @param response response struct to free
 */
void free_response(struct t_work_response *response) {
    if (response == NULL) {
        return;
    }
    FREE_SDS(response->data);
    FREE_SDS(response->partition);
    if (response->extra != NULL) {
        if (response->extra_free != NULL) {
            response->extra_free(response->extra);
        }
        else {
            MYMPD_LOG_WARN(NULL, "Extra data for response %s not freed", get_cmd_id_method_name(response->cmd_id));
        }
    }
    FREE_PTR(response);
}

/**
 * Pushes the response to a queue or frees it
 * @param response pointer to response struct to push
 * @return true on success, else false
 */
bool push_response(struct t_work_response *response) {
    switch(response->type) {
        case RESPONSE_TYPE_DEFAULT:
        case RESPONSE_TYPE_NOTIFY_CLIENT:
        case RESPONSE_TYPE_NOTIFY_PARTITION:
        case RESPONSE_TYPE_PUSH_CONFIG:
        case RESPONSE_TYPE_SCRIPT_DIALOG:
        case RESPONSE_TYPE_REDIRECT:
            MYMPD_LOG_DEBUG(NULL, "Push response to webserver queue for connection %lu: %s", response->conn_id, response->data);
            return mympd_queue_push(webserver_queue, response, 0);
        case RESPONSE_TYPE_RAW:
            MYMPD_LOG_DEBUG(NULL, "Push raw response to webserver queue for connection %lu with %lu bytes", response->conn_id, (unsigned long)sdslen(response->data));
            return mympd_queue_push(webserver_queue, response, 0);
        case RESPONSE_TYPE_SCRIPT:
            #ifdef MYMPD_ENABLE_LUA
                MYMPD_LOG_DEBUG(NULL, "Push response to script_worker_queue for thread %u: %s", response->id, response->data);
                return mympd_queue_push(script_worker_queue, response, response->id);
            #endif
        case RESPONSE_TYPE_DISCARD:
            // discard response
            free_response(response);
            return true;
    }
    // this should not appear
    MYMPD_LOG_ERROR(NULL, "Invalid response type for connection %lu: %s", response->conn_id, response->data);
    free_response(response);
    return false;
}

/**
 * Pushes the request to a queue
 * @param request pointer to request struct to push
 * @param id request id
 * @return true on success, else false
 */
bool push_request(struct t_work_request *request, unsigned id) {
    if (check_cmd_acl(request->cmd_id, API_SCRIPT_THREAD) == true) {
        #ifdef MYMPD_ENABLE_LUA
            //forward API request to script thread
            return mympd_queue_push(script_queue, request, id);
        #else
            return false;
        #endif
    }
    else {
        //forward API request to mympd_api thread
        return mympd_queue_push(mympd_api_queue, request, id);
    }
}
