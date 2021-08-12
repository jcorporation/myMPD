/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "api.h"

#include <string.h>

static const char *mympd_cmd_strs[] = { MYMPD_CMDS(GEN_STR) };

enum mympd_cmd_ids get_cmd_id(const char *cmd) {
    const size_t cmd_len = strlen(cmd);
    for (unsigned i = 0; i < TOTAL_API_COUNT; i++) {
        const size_t len = strlen(mympd_cmd_strs[i]);
        if (cmd_len == len && strncmp(cmd, mympd_cmd_strs[i], len) == 0) {
            return i;
        }
    }
    return GENERAL_API_UNKNOWN;
}

const char *get_cmd_id_method_name(enum mympd_cmd_ids cmd_id) {
    if (cmd_id >= TOTAL_API_COUNT) {
        return NULL;
    }
    return mympd_cmd_strs[cmd_id];
}

//defines methods that need authentication if a pin is set
bool is_protected_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_CONNECTION_SAVE:
        case MYMPD_API_COVERCACHE_CLEAR:
        case MYMPD_API_COVERCACHE_CROP:
        case MYMPD_API_MOUNT_MOUNT:
        case MYMPD_API_MOUNT_UNMOUNT:
        case MYMPD_API_PARTITION_NEW:
        case MYMPD_API_PARTITION_RM:
        case MYMPD_API_PARTITION_OUTPUT_MOVE:
        case MYMPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET:
        case MYMPD_API_PLAYLIST_RM_ALL:
        case MYMPD_API_SESSION_LOGOUT:
        case MYMPD_API_SESSION_VALIDATE:
        case MYMPD_API_SETTINGS_SET:
        case MYMPD_API_SCRIPT_RM:
        case MYMPD_API_SCRIPT_SAVE:
        case MYMPD_API_TIMER_RM:
        case MYMPD_API_TIMER_SAVE:
        case MYMPD_API_TIMER_TOGGLE:
        case MYMPD_API_TRIGGER_RM:
        case MYMPD_API_TRIGGER_SAVE:
            return true;
        default:
            return false;
    }
}

//defines methods that are internal
bool is_public_api_method(enum mympd_cmd_ids cmd_id) {
    if (cmd_id <= INTERNAL_API_COUNT ||
        cmd_id >= TOTAL_API_COUNT)
    {
        return false;
    }
    return true;
}

//defines methods that should work with no mpd connection
//this is necessary for correct startup and changing mpd connection settings
bool is_mympd_only_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_CONNECTION_SAVE:
        case MYMPD_API_HOME_LIST:
        case MYMPD_API_SCRIPT_LIST:
        case MYMPD_API_SETTINGS_GET:
            return true;
        default:
            return false;
    }
}
