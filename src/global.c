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

#include "tiny_queue.h"
#include "list.h"
#include "global.h"

bool state_file_read(t_config *config, const char *name, char *value) {
    char cfg_file[400];
    char *line;
    size_t n = 0;
    ssize_t read;
    
    if (!validate_string(name))
        return false;
    snprintf(cfg_file, 400, "%s/state/%s", config->varlibdir, name);
    FILE *fp = fopen(cfg_file, "r");
    if (fp == NULL) {
        printf("Error opening %s\n", cfg_file);
        return false;
    }
    read = getline(&line, &n, fp);
    snprintf(value, 400, "%s", line);
    LOG_DEBUG() fprintf(stderr, "DEBUG: State %s: %s\n", name, value);
    fclose(fp);
    if (read > 0)
        return true;
    else
        return false;
}

bool state_file_write(t_config *config, const char *name, const char *value) {
    char tmp_file[400];
    char cfg_file[400];
    
    if (!validate_string(name))
        return false;
    snprintf(cfg_file, 400, "%s/state/%s", config->varlibdir, name);
    snprintf(tmp_file, 400, "%s/tmp/%s", config->varlibdir, name);
        
    FILE *fp = fopen(tmp_file, "w");
    if (fp == NULL) {
        printf("Error opening %s\n", tmp_file);
        return false;
    }
    fprintf(fp, "%s", value);
    fclose(fp);
    if (rename(tmp_file, cfg_file) == -1) {
        printf("Error renaming file from %s to %s\n", tmp_file, cfg_file);
        return false;
    }
    return true;
}


bool testdir(const char *name, const char *dirname) {
    DIR* dir = opendir(dirname);
    if (dir) {
        closedir(dir);
        LOG_INFO() printf("%s: \"%s\"\n", name, dirname);
        return true;
    }
    else {
        printf("%s: \"%s\" don't exists\n", name, dirname);
        return false;
    }
}

int randrange(int n) {
    return rand() / (RAND_MAX / (n + 1) + 1);
}

bool validate_string(const char *data) {
    static char ok_chars[] = "abcdefghijklmnopqrstuvwxyz"
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                             "1234567890_-. ";
    const char *cp = data;
    const char *end = data + strlen(data);
    for (cp += strspn(cp, ok_chars); cp != end; cp += strspn(cp, ok_chars)) {
        printf("ERROR: Invalid character in string\n");
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
