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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "../../dist/src/sds/sds.h"
#include "../utility.h"
#include "../log.h"
#include "../list.h"
#include "../config_defs.h"
#include "mympd_api_bookmarks.h"
#include "../dist/src/frozen/frozen.h"

bool mympd_api_bookmark_update(t_config *config, const int id, const char *name, const char *uri, const char *type) {
    int line_nr = 0;
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    bool inserted = false;
    int fd;
    sds tmp_file = sdscatprintf(sdsempty(), "%s/state/bookmarks.XXXXXX", config->varlibdir);
    
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sds_free(tmp_file);
        return false;
    }
    FILE *fo = fdopen(fd, "w");
    
    sds b_file = sdscatprintf(sdsempty(), "%s/state/bookmarks", config->varlibdir);
    FILE *fi = fopen(b_file, "r");
    if (fi != NULL) {
        while ((read = getline(&line, &n, fi)) > 0) {
            char *lname = NULL;
            char *luri = NULL;
            char *ltype = NULL;
            int lid;
            int je = json_scanf(line, read, "{id: %d, name: %Q, uri: %Q, type: %Q}", &lid, &lname, &luri, &ltype);
            if (je == 4) {
                if (name != NULL) {
                    if (strcmp(name, lname) < 0) {
                        line_nr++;
                        struct json_out out = JSON_OUT_FILE(fo);
                        json_printf(&out, "{id: %d, name: %Q, uri: %Q, type: %Q}\n", line_nr, name, uri, type);
                        inserted = true;
                    }
                }
                if (lid != id) {
                    line_nr++;
                    struct json_out out = JSON_OUT_FILE(fo);
                    json_printf(&out, "{id: %d, name: %Q, uri: %Q, type: %Q}\n", line_nr, lname, luri, ltype);
                }
                FREE_PTR(lname);
                FREE_PTR(luri);
                FREE_PTR(ltype);
            }
            else {
                LOG_ERROR("Can't read bookmarks line");
            }
        }
        FREE_PTR(line);
        fclose(fi);
    }
    if (inserted == false && name != NULL) {
        line_nr++;
        struct json_out out = JSON_OUT_FILE(fo);
        json_printf(&out, "{id: %d, name: %Q, uri: %Q, type: %Q}\n", line_nr, name, uri, type);
    }
    fclose(fo);
    
    if (rename(tmp_file, b_file) == -1) {
        LOG_ERROR("Rename file from %s to %s failed", tmp_file, b_file);
        sds_free(tmp_file);
        sds_free(b_file);
        return false;
    }
    sds_free(tmp_file);
    sds_free(b_file);
    return true;
}

sds mympd_api_bookmark_list(t_config *config, sds buffer, unsigned int offset) {
    size_t len = 0;
    char *line = NULL;
    char *crap = NULL;
    size_t n = 0;
    ssize_t read;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    
    sds b_file = sdscatprintf(sdsempty(), "%s/state/bookmarks", config->varlibdir);
    FILE *fi = fopen(b_file, "r");

    if (fi == NULL) {
        //create empty bookmarks file
        fi = fopen(b_file, "w");
        if (fi == NULL) {
            LOG_ERROR("Can't open %s for write", b_file);
            buffer = sdscat(sdsempty(), "{\"type\":\"error\",\"data\":\"Failed to open bookmarks file\"}");
        }
        else {
            fclose(fi);
            buffer = sdscat(sdsempty(), "{\"type\":\"bookmark\",\"data\":[],\"totalEntities\":0,\"offset\":0,\"returnedEntities\":0}");
        }
    } else {
        buffer = sdscat(buffer, "{\"type\":\"bookmark\",\"data\":[");
        while ((read = getline(&line, &n, fi)) > 0 && len < MAX_LIST_SIZE) {
            entity_count++;
            if (entity_count > offset && entity_count <= offset + config->max_elements_per_page) {
                if (entities_returned++) {
                    buffer = sdscat(buffer, ",");
                }
                strtok_r(line, "\n", &crap);
                buffer = sdscat(buffer, line);
            }
        }
        FREE_PTR(line);
        fclose(fi);
        buffer = sdscatprintf(buffer, "],\"totalEntities\":%d,\"offset\":%d,\"returnedEntities\":%d}",
            entity_count,
            offset,
            entities_returned
        );
    }
    sds_free(b_file);
    return buffer;
}
