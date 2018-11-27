/* myMPD
   (c) 2018 Juergen Mang <mail@jcgames.de>
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
#include "validate.h"

void sanitize_string(const char *data) {
    static char ok_chars[] = "abcdefghijklmnopqrstuvwxyz"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "1234567890_-. ";
    char *cp = data;
    const char *end = data + strlen(data);
    for (cp += strspn(cp, ok_chars); cp != end; cp += strspn(cp, ok_chars))
        *cp = '_';
}

int validate_path(char *path, const char *basepath) {
    char *rpath = NULL;
    char *ptr;
    ptr = realpath(path, rpath);
    if (ptr == NULL)
        return 1;
    if (strncmp(basepath, ptr, strlen(basepath)) == 0) {
        free(rpath);
        return 0;
    }
    else {
        free(rpath);
        return 1;
    }
}
