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
#include <libgen.h>
#include <ctype.h>
#include <libgen.h>
#include <dirent.h>
#include <stdbool.h>

#include "list.h"
#include "tiny_queue.h"
#include "global.h"
#include "mympd_api.h"
#include "../dist/src/frozen/frozen.h"

//private definitions
//static void mympd_api(void *arg_request, void *arg_config);
static void mympd_api(t_work_request *request, t_config *config);
static int mympd_api_syscmd(char *buffer, const char *cmd, t_config *config);

//public functions
void *mympd_api_loop(void *arg_config) {
    while (s_signal_received == 0) {
        struct t_work_request *request = tiny_queue_shift(mympd_api_queue);
        mympd_api(request, arg_config);
    }
    return NULL;
}

//private functions
static void mympd_api(t_work_request *request, t_config *config) {
    //t_work_request *request = (t_work_request *) arg_request;
    size_t len = 0;
    char buffer[MAX_SIZE];
    int je;
    char *p_charbuf1;
    LOG_VERBOSE() printf("MYMPD API request: %.*s\n", request->length, request->data);
    
    if (request->cmd_id == MYMPD_API_SYSCMD) {
        if (config->syscmds == true) {
            je = json_scanf(request->data, request->length, "{data: {cmd: %Q}}", &p_charbuf1);
            if (je == 1) {
                len = mympd_api_syscmd(buffer, p_charbuf1, config);
                free(p_charbuf1);
            }
        } 
        else {
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"System commands are disabled.\"}");
        }
    }
    else {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Unknown cmd_id %u.\"}", request->cmd_id);
        printf("ERROR: Unknown cmd_id %u\n", request->cmd_id);    
    }

    if (len == 0) {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"No response for cmd_id %u.\"}", request->cmd_id);
        printf("ERROR: No response for cmd_id %u\n", request->cmd_id);
    }
    LOG_DEBUG() fprintf(stderr, "DEBUG: Send http response to connection %lu (first 800 chars):\n%*.*s\n", request->conn_id, 0, 800, buffer);

    t_work_result *response = (t_work_result *)malloc(sizeof(t_work_result));
    response->conn_id = request->conn_id;
    response->length = copy_string(response->data, buffer, MAX_SIZE, len);
    tiny_queue_push(web_server_queue, response);

    free(request);
}

static int mympd_api_syscmd(char *buffer, const char *cmd, t_config *config) {
    int len;
    char filename[400];
    char *line;
    char *crap;
    size_t n = 0;
    ssize_t read;
    
    const int order = list_get_value(&config->syscmd_list, cmd);
    if (order == -1) {
        printf("ERROR: Syscmd not defined: %s\n", cmd);
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"System command not defined\"}");
        return len;
    }
    
    snprintf(filename, 400, "%s/syscmds/%d%s", config->etcdir, order, cmd);
    FILE *fp = fopen(filename, "r");    
    if (fp == NULL) {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't execute cmd %s.\"}", cmd);
        printf("ERROR: Can't execute syscmd \"%s\"\n", cmd);
        return len;
    }
    read = getline(&line, &n, fp);
    fclose(fp);
    if (read > 0) {
        strtok_r(line, "\n", &crap);
        const int rc = system(line);
        if ( rc == 0) {
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"result\", \"data\": \"Executed cmd %s.\"}", cmd);
            LOG_VERBOSE() printf("Executed syscmd: \"%s\"\n", line);
        }
        else {
            len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Executing cmd %s failed.\"}", cmd);
            printf("ERROR: Executing syscmd \"%s\" failed.\n", cmd);
        }
    } else {
        len = snprintf(buffer, MAX_SIZE, "{\"type\": \"error\", \"data\": \"Can't execute cmd %s.\"}", cmd);
        printf("ERROR: Can't execute syscmd \"%s\"\n", cmd);
    }
    CHECK_RETURN_LEN();    
}

