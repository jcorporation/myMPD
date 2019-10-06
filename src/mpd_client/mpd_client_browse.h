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

#ifndef __BROWSE_H__
#define __BROWSE_H__
sds mpd_client_put_fingerprint(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                               const char *uri);
sds mpd_client_put_songdetails(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                               const char *uri);
sds mpd_client_put_filesystem(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id, 
                              const char *path, const unsigned int offset, const char *filter, const t_tags *tagcols);
sds mpd_client_put_db_tag(t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                          const unsigned int offset, const char *mpdtagtype, const char *mpdsearchtagtype, const char *searchstr, const char *filter);
sds mpd_client_put_songs_in_album(t_config *config, t_mpd_state *mpd_state, sds buffer, sds method, int request_id,
                                  const char *album, const char *search, const char *tag, const t_tags *tagcols);
#endif
