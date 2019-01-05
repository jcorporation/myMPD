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
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include "common.h"

void sanitize_string(const char *data) {
    static char ok_chars[] = "abcdefghijklmnopqrstuvwxyz"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "1234567890_-. ";
    char *cp = data;
    const char *end = data + strlen(data);
    for (cp += strspn(cp, ok_chars); cp != end; cp += strspn(cp, ok_chars))
        *cp = '_';
}

int copy_string(char * const dest, char const * const src, size_t const dst_len, size_t const src_len) {
    if (dst_len == 0 || src_len == 0)
        return 0;
    size_t const max = (src_len < dst_len) ? src_len : dst_len -1;
    memcpy(dest, src, max);
    dest[max] = '\0';
    return max;
}
