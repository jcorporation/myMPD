/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "api.h"

enum mympd_cmd_ids get_cmd_id(const char *cmd) {
    const char * mympd_cmd_strs[] = { MYMPD_CMDS(GEN_STR) };

    for (unsigned i = 0; i < sizeof(mympd_cmd_strs) / sizeof(mympd_cmd_strs[0]); i++) {
        if (!strncmp(cmd, mympd_cmd_strs[i], strlen(mympd_cmd_strs[i]))) { /* Flawfinder: ignore */
            return i;
        }
    }
    return 0;
}

bool is_public_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MPD_API_UNKNOWN:
        case MPD_API_SCRIPT_INIT:
        case MPD_API_TIMER_STARTPLAY:
        case MPDWORKER_API_CACHES_CREATE:
        case MYMPD_API_TIMER_SET:
        case MYMPD_API_SCRIPT_INIT:
        case MYMPD_API_SCRIPT_POST_EXECUTE:
        case MYMPD_API_STATE_SAVE:
        case MPD_API_STATE_SAVE:
            return false;
        default:
            return true;
    }
}
