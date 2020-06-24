/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#define INCBIN_PREFIX 
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "../../dist/src/incbin/incbin.h"

INCBIN(json_lua, "../contrib/lualibs/json.lua");
