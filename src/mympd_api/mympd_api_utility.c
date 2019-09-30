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

#include "../../dist/sds/sds.h"
#include "mympd_api_utility.h"
#include "../utility.h"

void free_mympd_state(t_mympd_state *mympd_state) {
    sds_free(mympd_state->mpd_host);
    sds_free(mympd_state->mpd_pass);
    sds_free(mympd_state->taglist);
    sds_free(mympd_state->searchtaglist);
    sds_free(mympd_state->browsetaglist);
    sds_free(mympd_state->love_channel);
    sds_free(mympd_state->love_message);
    sds_free(mympd_state->jukebox_playlist);
    sds_free(mympd_state->cols_queue_current);
    sds_free(mympd_state->cols_search);
    sds_free(mympd_state->cols_browse_database);
    sds_free(mympd_state->cols_browse_playlists_detail);
    sds_free(mympd_state->cols_browse_filesystem);
    sds_free(mympd_state->cols_playback);
    sds_free(mympd_state->cols_queue_last_played);
    sds_free(mympd_state->stream_url);
    sds_free(mympd_state->bg_color);
    sds_free(mympd_state->bg_css_filter);
    sds_free(mympd_state->coverimage_name);
    sds_free(mympd_state->locale);
    sds_free(mympd_state->music_directory);
    FREE_PTR(mympd_state);
}
