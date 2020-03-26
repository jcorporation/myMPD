/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mympd_api_utility.h"
#include "mympd_api_syscmds.h"

sds mympd_api_syscmd(t_config *config, sds buffer, sds method, int request_id, 
                     const char *cmd)
{
    sds cmdline = list_get_value_p(&config->syscmd_list, cmd);
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
