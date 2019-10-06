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
#include "mympd_api_utility.h"
#include "mympd_api_bookmarks.h"
#include "../dist/src/frozen/frozen.h"

//private definitions
static bool write_bookmarks_line(FILE *fp, int id, const char *name,
                                 const char *uri, const char *type);

//public functions
bool mympd_api_bookmark_update(t_config *config, const int id, const char *name, 
                               const char *uri, const char *type)
{
    int line_nr = 0;
    char *line = NULL;
    size_t n = 0;
    ssize_t read;
    bool inserted = false;
    int fd;
    sds tmp_file = sdscatfmt(sdsempty(), "%s/state/bookmarks.XXXXXX", config->varlibdir);
    
    if ((fd = mkstemp(tmp_file)) < 0 ) {
        LOG_ERROR("Can't open %s for write", tmp_file);
        sds_free(tmp_file);
        return false;
    }
    FILE *fo = fdopen(fd, "w");
    
    sds b_file = sdscatfmt(sdsempty(), "%s/state/bookmarks", config->varlibdir);
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
                        bool rc = write_bookmarks_line(fo, line_nr, name, uri, type);
                        if (rc == true) {
                            inserted = true;
                        }
                    }
                }
                if (lid != id) {
                    line_nr++;
                    write_bookmarks_line(fo, line_nr, lname, luri, ltype);
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
        write_bookmarks_line(fo, line_nr, name, uri, type);
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

sds mympd_api_bookmark_list(t_config *config, sds buffer, sds method, int request_id,
                            unsigned int offset)
{
    char *line = NULL;
    char *crap = NULL;
    size_t n = 0;
    ssize_t read;
    unsigned entity_count = 0;
    unsigned entities_returned = 0;
    
    sds b_file = sdscatfmt(sdsempty(), "%s/state/bookmarks", config->varlibdir);
    FILE *fi = fopen(b_file, "r");

    buffer = jsonrpc_start_result(buffer, method, request_id);
    buffer = sdscat(buffer, "[");

    if (fi == NULL) {
        //create empty bookmarks file
        fi = fopen(b_file, "w");
        if (fi == NULL) {
            LOG_ERROR("Can't open %s for write", b_file);
            buffer = jsonrpc_respond_message(sdsempty(), method, request_id, "Failed to open bookmarks file", true);
        }
        else {
            fclose(fi);
        }
    }
    else {
        while ((read = getline(&line, &n, fi)) > 0) {
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

    }
    sds_free(b_file);
    buffer = sdscat(buffer, "],");
    buffer = tojson_long(buffer, "totalEntities", entity_count, true);
    buffer = tojson_long(buffer, "offset", offset, true);
    buffer = tojson_long(buffer, "returnedEntities", entities_returned, false);
    buffer = jsonrpc_end_result(buffer);
    return buffer;
}

//private functions
static bool write_bookmarks_line(FILE *fp, int id, const char *name, 
                                 const char *uri, const char *type)
{
    sds line = sdscatfmt(sdsempty(), "{\"id\": %d,\"name\":%Q,\"uri\":\"%Q\",\"type\":\"%Q\"}\n", line_nr, name, uri, type);
    int rc = fputs(line, fo);
    sds_free(line);
    if (rc > 0) {
        return true;
    }
    LOG_ERROR("Can't write bookmarks line to file");
    return false;
}
