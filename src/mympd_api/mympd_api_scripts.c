/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <stdio.h>
#include <stdlib.h>

#include "../../dist/src/sds/sds.h"
#include "../sds_extras.h"
#include "../../dist/src/frozen/frozen.h"
#include "../log.h"
#include "../list.h"
#include "config_defs.h"
#include "../utility.h"
#include "mympd_api_utility.h"
#include "mympd_api_scripts.h"

#ifdef ENABLE_LUA
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"  
#endif

#ifdef ENABLE_LUA
sds mympd_api_script_execute(t_config *config, sds buffer, sds method, int request_id, const char *script) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    sds script_file = sdscatfmt(sdsempty(), "%s/%s.lua", config->varlibdir, script);
    int rc = luaL_dofile(L, script_file);
    lua_close(L);
    sdsfree(script_file);
    if (rc == 0) {
        buffer = jsonrpc_respond_ok(buffer, method, request_id);
    }
    else {
        buffer = jsonrpc_respond_message(buffer, method, request_id, "Error executing script", true);
    }
    return buffer;
}
#else
sds mympd_api_script_execute(t_config *config, sds buffer, sds method, int_request_id, const char *script) {
    buffer = jsonrpc_respond_message(buffer, method, request_id, "Scripting is disabled", true);
    return buffer;
}
#endif
