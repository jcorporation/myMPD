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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../dist/src/sds/sds.h"
#include "log.h"

static const char *loglevel_names[] = {
  "ERROR", "WARN", "INFO", "VERBOSE", "DEBUG"
};

void set_loglevel(int level) {
    if (level > 4) {
        level = 4;
    }
    else if (level < 0) {
        level = 0;
    }
    LOG_INFO("Setting loglevel to %s", loglevel_names[level]);
    loglevel = level;
}

void mympd_log(int level, const char *file, int line, const char *fmt, ...) {
    if (level > loglevel) {
        return;
    }
    sds logline = sdscatprintf(sdsempty(), "%-8s ", loglevel_names[level]);

    if (loglevel == 4) {
        logline = sdscatprintf(logline, "%s:%d: ", file, line);
    }

    va_list args;
    va_start(args, fmt);
    logline = sdscatvprintf(logline, fmt, args);
    va_end(args);

    if (sdslen(logline) > 1023) {
        sdsrange(logline, 0, 1020);
        logline = sdscatlen(logline, "...\n", 4);
    }
    else {
        logline = sdscatlen(logline, "\n", 1);
    }
    
    fputs(logline, stderr);
    fflush(stderr);
    sdsfree(logline);
}
