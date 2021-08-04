/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "api.h"

#include <string.h>

enum mympd_cmd_ids get_cmd_id(const char *cmd) {
    const char * mympd_cmd_strs[] = { MYMPD_CMDS(GEN_STR) };

    for (unsigned i = 0; i < sizeof(mympd_cmd_strs) / sizeof(mympd_cmd_strs[0]); i++) {
        if (!strncmp(cmd, mympd_cmd_strs[i], strlen(mympd_cmd_strs[i]))) { /* Flawfinder: ignore */
            return i;
        }
    }
    return 0;
}

bool is_protected_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_MOUNT_MOUNT:
        case MYMPD_API_MOUNT_UNMOUNT:
        case MYMPD_API_PARTITION_NEW:
        case MYMPD_API_PARTITION_RM:
        case MYMPD_API_PARTITION_OUTPUT_MOVE:
        case MYMPD_API_TRIGGER_SAVE:
        case MYMPD_API_TRIGGER_DELETE:
        case MYMPD_API_CONNECTION_SAVE:
        case MYMPD_API_COVERCACHE_CROP:
        case MYMPD_API_COVERCACHE_CLEAR:
        case MYMPD_API_TIMER_SAVE:
        case MYMPD_API_TIMER_RM:
        case MYMPD_API_TIMER_TOGGLE:
        case MYMPD_API_SCRIPT_SAVE:
        case MYMPD_API_SCRIPT_DELETE:
        case MYMPD_API_SETTINGS_SET:
            return true;
        default:
            return false;
    }
}

bool is_public_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_UNKNOWN:
        case MYMPD_API_TIMER_STARTPLAY:
        case MYMPD_API_CACHES_CREATE:
        case MYMPD_API_TIMER_SET:
        case MYMPD_API_SCRIPT_INIT:
        case MYMPD_API_SCRIPT_POST_EXECUTE:
        case MYMPD_API_STATE_SAVE:
            return false;
        default:
            return true;
    }
}

bool is_mympd_only_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_SETTINGS_GET:
        case MYMPD_API_CONNECTION_SAVE:
        case MYMPD_API_HOME_LIST:
        case MYMPD_API_SCRIPT_LIST:
            return true;
        default:
            return false;
    }
}
