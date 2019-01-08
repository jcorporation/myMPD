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

#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "tiny_queue.h"
#include "global.h"

int randrange(int n) {
    return rand() / (RAND_MAX / (n + 1) + 1);
}

void sanitize_string(const char *data) {
    static char ok_chars[] = "abcdefghijklmnopqrstuvwxyz"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "1234567890_-. ";
    char *cp = data;
    const char *end = data + strlen(data);
    for (cp += strspn(cp, ok_chars); cp != end; cp += strspn(cp, ok_chars))
        *cp = '_';
}

bool validate_string(const char *data) {
    static char ok_chars[] = "abcdefghijklmnopqrstuvwxyz"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "1234567890_-. ";
    const char *cp = data;
    const char *end = data + strlen(data);
    for (cp += strspn(cp, ok_chars); cp != end; cp += strspn(cp, ok_chars))
        return false;
    return true;
}


int copy_string(char * const dest, char const * const src, size_t const dst_len, size_t const src_len) {
    if (dst_len == 0 || src_len == 0)
        return 0;
    size_t const max = (src_len < dst_len) ? src_len : dst_len -1;
    memcpy(dest, src, max);
    dest[max] = '\0';
    return max;
}

enum mypd_cmd_ids get_cmd_id(const char *cmd) {
    const char * mympd_cmd_strs[] = { MYMPD_CMDS(GEN_STR) };

    for (unsigned i = 0; i < sizeof(mympd_cmd_strs) / sizeof(mympd_cmd_strs[0]); i++)
        if (!strncmp(cmd, mympd_cmd_strs[i], strlen(mympd_cmd_strs[i])))
            return i;

    return 0;
}
