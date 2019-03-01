/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
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

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "mympd_coverextract.h"

int main(int argc, char **argv) {
    char buffer[1024];
    if (argc == 2) {
        int rc = coverextract(argv[1], buffer, 1024, true);
        printf("RC: %d, Buffer: %s\n", rc, buffer);
    }
    else {
        printf("Usage: %s mediafile\n", argv[0]);
        return 1;
    }
    return 0;
}
