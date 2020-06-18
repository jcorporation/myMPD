/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdlib.h>

#include <mpd/client.h>

#include "../dist/src/sds/sds.h"
#include "lua_mympd_state.h"

void free_t_lua_mympd_state(t_lua_mympd_state *lua_mympd_state) {
    sdsfree(lua_mympd_state->music_directory);
    sdsfree(lua_mympd_state->varlibdir);
    free(lua_mympd_state);
}

