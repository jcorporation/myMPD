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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>
#include <stdlib.h>

#define MAX_SIZE 2048 * 400
#define MAX_ELEMENTS_PER_PAGE 400

#define LOG_INFO() if (config.loglevel >= 1) 
#define LOG_VERBOSE() if (config.loglevel >= 2) 
#define LOG_DEBUG() if (config.loglevel == 3) 

typedef struct {
    long mpdport;
    const char *mpdhost;
    const char *mpdpass;
    const char *webport;
    bool ssl;
    const char *sslport;
    const char *sslcert;
    const char *sslkey;
    const char *user;
    bool coverimage;
    const char *coverimagename;
    long coverimagesize;
    bool stickers;
    bool mixramp;
    const char *taglist;
    const char *searchtaglist;
    const char *browsetaglist;
    bool smartpls;
    const char *varlibdir;
    const char *etcdir;
    unsigned long max_elements_per_page;
    bool syscmds;
    bool localplayer;
    long streamport;
    const char *streamurl;
    unsigned long last_played_count;
    long loglevel;
} t_config;

t_config config;

void sanitize_string(const char *data);
int copy_string(char * const dest, char const * const src, size_t const dst_len, size_t const src_len);
#endif
