/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <string.h>
#include <stdlib.h>

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
