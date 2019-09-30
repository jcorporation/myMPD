/* myMPD (c) 2018-2019 Juergen Mang <mail@jcgames.de> This project's
   homepage is: https://github.com/jcorporation/mympd
   
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
#ifndef __MPD_CLIENT_STATE_H__
#define __MPD_CLIENT_STATE_H__
sds mpd_client_get_updatedb_state(t_mpd_state *mpd_state, sds buffer);
sds mpd_client_put_state(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
sds mpd_client_put_volume(t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
sds mpd_client_put_outputs(t_mpd_state *mpd_state, sds buffer, sds method, int request_id)
sds mpd_client_put_current_song(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id);
#endif
