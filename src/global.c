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
#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <mpd/client.h>

#include "tiny_queue.h"
#include "list.h"
#include "global.h"

bool testdir(const char *name, const char *dirname) {
    DIR* dir = opendir(dirname);
    if (dir) {
        closedir(dir);
        LOG_INFO("%s: \"%s\"", name, dirname);
        return true;
    }
    else {
        LOG_ERROR("%s: \"%s\" don't exists", name, dirname);
        return false;
    }
}

int randrange(int n) {
    return rand() / (RAND_MAX / (n + 1) + 1);
}

bool validate_string(const char *data) {
    if (strchr(data, '/') != NULL || strchr(data, '\n') != NULL || strchr(data, '\r') != NULL ||
        strchr(data, '"') != NULL || strchr(data, '\'') != NULL) {
        return false;
    }
    return true;
}

int replacechar(char *str, const char orig, const char rep) {
    char *ix = str;
    int n = 0;
    while ((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}

int copy_string(char * const dest, char const * const src, size_t const dst_len, size_t const src_len) {
    if (dst_len == 0 || src_len == 0)
        return 0;
    size_t const max = (src_len < dst_len) ? src_len : dst_len -1;
    memcpy(dest, src, max);
    dest[max] = '\0';
    return max;
}

enum mympd_cmd_ids get_cmd_id(const char *cmd) {
    const char * mympd_cmd_strs[] = { MYMPD_CMDS(GEN_STR) };

    for (unsigned i = 0; i < sizeof(mympd_cmd_strs) / sizeof(mympd_cmd_strs[0]); i++)
        if (!strncmp(cmd, mympd_cmd_strs[i], strlen(mympd_cmd_strs[i])))
            return i;

    return 0;
}


static const char *loglevel_names[] = {
  "ERROR", "WARN", "INFO", "VERBOSE", "DEBUG"
};


void set_loglevel(t_config *config) {
    #ifdef DEBUG
    config->loglevel = 4;
    #endif

    if (config->loglevel > 4) {
        config->loglevel = 4;
    }
    else if (config->loglevel < 0) {
        config->loglevel = 0;
    }
    LOG_INFO("Setting loglevel to %s", loglevel_names[config->loglevel]);
    loglevel = config->loglevel;
}

void mympd_log(int level, const char *file, int line, const char *fmt, ...) {
    if (level > loglevel) {
        return;
    }
    
    size_t max_out = 1024;
    char out[max_out];
    size_t len = 0;
    
    len = snprintf(out, max_out, "%-8s ", loglevel_names[level]);
    if (loglevel == 4) {
        len += snprintf(out + len, max_out - len, "%s:%d: ", file, line);
    }
    va_list args;
    va_start(args, fmt);
    if (len < max_out - 2) {
        len += vsnprintf(out + len, max_out - len, fmt, args);
    }
    va_end(args);
    if (len < max_out - 2) {
        snprintf(out + len, max_out -len, "\n");
    }
    else {
        snprintf(out + max_out - 5, 5, "...\n");
    }
    fprintf(stderr, "%s", out);
    fflush(stderr);
}
