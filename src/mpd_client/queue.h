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

#ifndef __QUEUE_H__
#define __QUEUE_H__
int mpd_client_get_queue_state(t_mpd_state *mpd_state, char *buffer);
int mpd_client_put_queue_state(struct mpd_status *status, char *buffer);
int mpd_client_put_queue(t_mpd_state *mpd_state, char *buffer, const unsigned int offset, const t_tags *tagcols);
int mpd_client_search_queue(t_mpd_state *mpd_state, char *buffer, const char *mpdtagtype, const unsigned int offset, const char *searchstr, const t_tags *tagcols);
int mpd_client_queue_crop(t_mpd_state *mpd_state, char *buffer);
#endif
