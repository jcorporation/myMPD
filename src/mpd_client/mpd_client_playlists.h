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

#ifndef __MPD_CLIENT_PLAYLISTS_H__
#define __MPD_CLIENT_PLAYLISTS_H__
sds mpd_client_put_playlists(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                             const unsigned int offset, const char *filter);
sds mpd_client_put_playlist_list(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                                 const char *uri, const unsigned int offset, const char *filter, const t_tags *tagcols);
sds mpd_client_playlist_delete(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                               const char *playlist);
sds mpd_client_playlist_rename(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                                const char *old_playlist, const char *new_playlist);
sds mpd_client_smartpls_put(t_config *config, sds buffer, sds method, int request_id,
                            const char *playlist);
bool mpd_client_smartpls_save(t_config *config, t_mpd_state *mpd_state, const char *smartpltype, 
                              const char *playlist, const char *tag, const char *searchstr, const int maxentries, 
                              const int timerange);

bool mpd_client_smartpls_update_all(t_config *config, t_mpd_state *mpd_state);
bool mpd_client_smartpls_update(t_config *config, t_mpd_state *mpd_state, const char *playlist);
bool mpd_client_smartpls_clear(t_mpd_state *mpd_state, const char *playlist);
bool mpd_client_smartpls_update_search(t_mpd_state *mpd_state, const char *playlist, const char *tag, const char *searchstr);
bool mpd_client_smartpls_update_sticker(t_mpd_state *mpd_state, const char *playlist, const char *sticker, const int maxentries);
bool mpd_client_smartpls_update_newest(t_mpd_state *mpd_state, const char *playlist, const int timerange);
#endif
