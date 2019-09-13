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

#ifndef __API_H__
#define __API_H__

//API cmds
#define MYMPD_CMDS(X) \
    X(MPD_API_UNKNOWN) \
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
    X(MPD_API_QUEUE_ADD_RANDOM) \
    X(MPD_API_PLAYLIST_CLEAR) \
    X(MPD_API_PLAYLIST_RENAME) \
    X(MPD_API_PLAYLIST_MOVE_TRACK) \
    X(MPD_API_PLAYLIST_ADD_TRACK) \
    X(MPD_API_PLAYLIST_RM_TRACK) \
    X(MPD_API_PLAYLIST_RM) \
    X(MPD_API_PLAYLIST_LIST) \
    X(MPD_API_PLAYLIST_CONTENT_LIST) \
    X(MPD_API_SMARTPLS_UPDATE_ALL) \
    X(MPD_API_SMARTPLS_UPDATE) \
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
    X(MPD_API_DATABASE_FINGERPRINT) \
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
    X(MPD_API_LIKE) \
    X(MPD_API_LOVE) \
    X(MYMPD_API_COLS_SAVE) \
    X(MYMPD_API_SYSCMD) \
    X(MYMPD_API_SETTINGS_GET) \
    X(MYMPD_API_SETTINGS_SET) \
    X(MYMPD_API_SETTINGS_RESET) \
    X(MYMPD_API_CONNECTION_SAVE) \
    X(MYMPD_API_BOOKMARK_LIST) \
    X(MYMPD_API_BOOKMARK_SAVE) \
    X(MYMPD_API_BOOKMARK_RM)

#define GEN_ENUM(X) X,
#define GEN_STR(X) #X,

//global enums
enum mympd_cmd_ids {
    MYMPD_CMDS(GEN_ENUM)
};

//global functions
enum mympd_cmd_ids get_cmd_id(const char *cmd);
#endif
