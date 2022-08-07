/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#ifndef MYMPD_EMBEDDED_LUALIBS_C
#define MYMPD_EMBEDDED_LUALIBS_C

#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE

#include "compile_time.h"
#include "../../dist/incbin/incbin.h"

INCBIN(json_lua, "../contrib/lualibs/json.lua");
INCBIN(mympd_lua, "../contrib/lualibs/mympd.lua");

#endif
