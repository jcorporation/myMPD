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
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <assert.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../api.h"
#include "../tiny_queue.h"
#include "../global.h"
#include "../utility.h"
#include "mympd_api_utility.h"

void mympd_api_push_to_mpd_client(t_mympd_state *mympd_state) {
    t_work_request *mpd_client_request = (t_work_request *)malloc(sizeof(t_work_request));
    assert(mpd_client_request);
    mpd_client_request->conn_id = -1;
    mpd_client_request->id = 0;
    mpd_client_request->cmd_id = MYMPD_API_SETTINGS_SET;
    mpd_client_request->method = sdsnew("MYMPD_API_SETTINGS_SET");
    sds data = sdsempty();

    data = sdscat(data, "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"MYMPD_API_SETTINGS_SET\",\"params\":{");
    data = tojson_long(data, "jukeboxMode", mympd_state->jukebox_mode, true);
    data = tojson_char(data, "jukeboxPlaylist", mympd_state->jukebox_playlist, true);
    data = tojson_long(data, "jukeboxQueueLength", mympd_state->jukebox_queue_length, true);
    data = tojson_bool(data, "autoPlay", mympd_state->auto_play, true);
    data = tojson_bool(data, "coverimage", mympd_state->coverimage, true);
    data = tojson_char(data, "coverimageName", mympd_state->coverimage_name, true);
    data = tojson_bool(data, "love", mympd_state->love, true);
    data = tojson_char(data, "loveChannel", mympd_state->love_channel, true);
    data = tojson_char(data, "loveMessage", mympd_state->love_message, true);
    data = tojson_char(data, "taglist", mympd_state->taglist, true);
    data = tojson_char(data, "searchtaglist", mympd_state->searchtaglist, true);
    data = tojson_char(data, "browsetaglist", mympd_state->browsetaglist, true);
    data = tojson_bool(data, "stickers", mympd_state->stickers, true);
    data = tojson_bool(data, "smartpls", mympd_state->smartpls, true);
    data = tojson_char(data, "mpdHost", mympd_state->mpd_host, true);
    data = tojson_char(data, "mpdPass", mympd_state->mpd_pass, true);
    data = tojson_long(data, "mpdPort", mympd_state->mpd_port, true);
    data = tojson_long(data, "lastPlayedCount", mympd_state->last_played_count, true);
    data = tojson_long(data, "maxElementsPerPage", mympd_state->max_elements_per_page, true);
    data = tojson_char(data, "musicDirectory", mympd_state->music_directory, false);
    data = sdscat(data, "}}");

    mpd_client_request->data = data;
    tiny_queue_push(mpd_client_queue, mpd_client_request);
}

void free_mympd_state(t_mympd_state *mympd_state) {
    sdsfree(mympd_state->mpd_host);
    sdsfree(mympd_state->mpd_pass);
    sdsfree(mympd_state->taglist);
    sdsfree(mympd_state->searchtaglist);
    sdsfree(mympd_state->browsetaglist);
    sdsfree(mympd_state->love_channel);
    sdsfree(mympd_state->love_message);
    sdsfree(mympd_state->jukebox_playlist);
    sdsfree(mympd_state->cols_queue_current);
    sdsfree(mympd_state->cols_search);
    sdsfree(mympd_state->cols_browse_database);
    sdsfree(mympd_state->cols_browse_playlists_detail);
    sdsfree(mympd_state->cols_browse_filesystem);
    sdsfree(mympd_state->cols_playback);
    sdsfree(mympd_state->cols_queue_last_played);
    sdsfree(mympd_state->stream_url);
    sdsfree(mympd_state->bg_color);
    sdsfree(mympd_state->bg_css_filter);
    sdsfree(mympd_state->coverimage_name);
    sdsfree(mympd_state->locale);
    sdsfree(mympd_state->music_directory);
    FREE_PTR(mympd_state);
}
