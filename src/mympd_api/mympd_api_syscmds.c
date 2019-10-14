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
#include <stdbool.h>

#include "../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../utility.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "mympd_api_utility.h"
#include "mympd_api_syscmds.h"

sds mympd_api_syscmd(t_config *config, sds buffer, sds method, int request_id, 
                     const char *cmd)
{
    sds cmdline = list_get_extra(&config->syscmd_list, cmd);
    if (cmdline == NULL) {
        LOG_ERROR("Syscmd not defined: %s", cmd);
        buffer = jsonrpc_respond_message(buffer, method, request_id, "System command not defined", true);
        return buffer;
    }
    LOG_DEBUG("Executing syscmd \"%s\"", cmdline);
    const int rc = system(cmdline); /* Flawfinder: ignore */
    if (rc == 0) {
        buffer = jsonrpc_start_phrase(buffer, method, request_id, "Successfully execute cmd %{cmd}", false);
        buffer = tojson_char(buffer, "cmd", cmd, false);
        buffer = jsonrpc_end_phrase(buffer);
    }
    else {
        buffer = jsonrpc_start_phrase(buffer, method, request_id, "Failed to execute cmd %{cmd}", true);
        buffer = tojson_char(buffer, "cmd", cmd, false);
        buffer = jsonrpc_end_phrase(buffer);
        LOG_ERROR("Executing syscmd \"%s\" failed", cmdline);
    }
    return buffer;
}
