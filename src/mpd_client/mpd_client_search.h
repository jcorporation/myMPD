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

#ifndef __SEARCH_H__
#define __SEARCH_H__
int mpd_client_search(t_mpd_state *mpd_state, char *buffer, const char *searchstr, const char *filter, const char *plist, const unsigned int offset, const t_tags *tagcols);
int mpd_client_search_adv(t_mpd_state *mpd_state, char *buffer, const char *expression, const char *sort, const bool sortdesc, const char *grouptag, const char *plist, const unsigned int offset, const t_tags *tagcols);
#endif
