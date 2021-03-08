/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
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

sds mympd_api_syscmd(t_config *config, sds buffer, sds method, long request_id, 
                     const char *cmd)
{
    sds cmdline = list_get_value_p(&config->syscmd_list, cmd);
    if (cmdline == NULL) {
        MYMPD_LOG_ERROR("Syscmd not defined: %s", cmd);
        buffer = jsonrpc_respond_message(buffer, method, request_id, true,
            "script", "error", "System command not defined");
        return buffer;
    }
    MYMPD_LOG_DEBUG("Executing syscmd \"%s\"", cmdline);
    const int rc = system(cmdline); /* Flawfinder: ignore */
    if (rc == 0) {
        buffer = jsonrpc_respond_message_phrase(buffer, method, request_id, false,
            "script", "info", "Successfully execute cmd %{cmd}", 2, "cmd", cmd);
    }
    else {
        buffer = jsonrpc_respond_message_phrase(buffer, method, request_id, true,
            "script", "error", "Failed to execute cmd %{cmd}", 2, "cmd", cmd);
        MYMPD_LOG_ERROR("Executing syscmd \"%s\" failed", cmdline);
    }
    return buffer;
}
