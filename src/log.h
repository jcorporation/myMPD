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

#ifndef __LOG_H__
#define __LOG_H__

enum { LOGLEVEL_ERROR, LOGLEVEL_WARN, LOGLEVEL_INFO, LOGLEVEL_VERBOSE, LOGLEVEL_DEBUG };

#define LOG_ERROR(...) mympd_log(LOGLEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) mympd_log(LOGLEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) mympd_log(LOGLEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_VERBOSE(...) mympd_log(LOGLEVEL_VERBOSE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) mympd_log(LOGLEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

int loglevel;

void mympd_log(int level, const char *file, int line, const char *fmt, ...);
void set_loglevel(int level);

#endif
