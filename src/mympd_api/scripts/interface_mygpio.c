/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2024 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "src/mympd_api/scripts/interface_mygpio.h"

#include "src/lib/log.h"

#include <libmygpio/libmygpio.h>

// private definitions
static struct t_mygpio_connection *mygpio_connect(const char *mygpiod_socket);

// public functions

/**
 * Lua binding for mygpio_gpioblink
 * @param lua_vm lua instance
 * @return 1 on success, else luaL_error
 */
int lua_mygpio_gpio_blink(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 4) {
        MYMPD_LOG_ERROR(NULL, "Lua - mygpio_gpio_blink: invalid number of arguments");
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *mygpiod_socket = lua_tostring(lua_vm, 1);
    unsigned gpio = (unsigned)lua_tointeger(lua_vm, 2);
    int timeout_ms = (int)lua_tointeger(lua_vm, 3);
    int interval_ms = (int)lua_tointeger(lua_vm, 4);
    struct t_mygpio_connection *mygpio_conn = mygpio_connect(mygpiod_socket);
    if (mygpio_conn != NULL) {
        mygpio_gpioblink(mygpio_conn, gpio, timeout_ms, interval_ms);
        mygpio_connection_free(mygpio_conn);
        lua_pushinteger(lua_vm, 0);
        //return response count
        return 1;
    }
    return luaL_error(lua_vm, "Unable to connect to myGPIOd");
}

/**
 * Lua binding for mygpio_gpioget
 * @param lua_vm lua instance
 * @return 1 on success, else luaL_error
 */
int lua_mygpio_gpio_get(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 2) {
        MYMPD_LOG_ERROR(NULL, "Lua - mygpio_gpio_get: invalid number of arguments");
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *mygpiod_socket = lua_tostring(lua_vm, 1);
    unsigned gpio = (unsigned)lua_tointeger(lua_vm, 2);
    struct t_mygpio_connection *mygpio_conn = mygpio_connect(mygpiod_socket);
    if (mygpio_conn != NULL) {
        int value = mygpio_gpioget(mygpio_conn, gpio);
        mygpio_connection_free(mygpio_conn);
        lua_pushinteger(lua_vm, value);
        //return response count
        return 1;
    }
    return luaL_error(lua_vm, "Unable to connect to myGPIOd");
}

/**
 * Lua binding for mygpio_gpioset
 * @param lua_vm lua instance
 * @return 1 on success, else luaL_error
 */
int lua_mygpio_gpio_set(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 3) {
        MYMPD_LOG_ERROR(NULL, "Lua - mygpio_gpio_set: invalid number of arguments");
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *mygpiod_socket = lua_tostring(lua_vm, 1);
    unsigned gpio = (unsigned)lua_tointeger(lua_vm, 2);
    int value = (int)lua_tointeger(lua_vm, 3);
    struct t_mygpio_connection *mygpio_conn = mygpio_connect(mygpiod_socket);
    if (mygpio_conn != NULL) {
        mygpio_gpioset(mygpio_conn, gpio, value);
        mygpio_connection_free(mygpio_conn);
        lua_pushinteger(lua_vm, 0);
        //return response count
        return 1;
    }
    return luaL_error(lua_vm, "Unable to connect to myGPIOd");
}

/**
 * Lua binding for mygpio_gpiotoggle
 * @param lua_vm lua instance
 * @return 1 on success, else luaL_error
 */
int lua_mygpio_gpio_toggle(lua_State *lua_vm) {
    int n = lua_gettop(lua_vm);
    if (n != 2) {
        MYMPD_LOG_ERROR(NULL, "Lua - mygpio_gpio_toggle: invalid number of arguments");
        return luaL_error(lua_vm, "Invalid number of arguments");
    }
    const char *mygpiod_socket = lua_tostring(lua_vm, 1);
    unsigned gpio = (unsigned)lua_tointeger(lua_vm, 2);
    struct t_mygpio_connection *mygpio_conn = mygpio_connect(mygpiod_socket);
    if (mygpio_conn != NULL) {
        mygpio_gpiotoggle(mygpio_conn, gpio);
        mygpio_connection_free(mygpio_conn);
        lua_pushinteger(lua_vm, 0);
        //return response count
        return 1;
    }
    return luaL_error(lua_vm, "Unable to connect to myGPIOd");
}

/**
 * Connects to the myGPIOd socket /run/mygpiod/socket
 * @param mygpiod_socket path of the myGPIOd socket
 * @return myGPIOd connection or NULL on error
 */
static struct t_mygpio_connection *mygpio_connect(const char *mygpiod_socket) {
    struct t_mygpio_connection *conn = mygpio_connection_new(mygpiod_socket, 5000);
    if (conn == NULL) {
        MYMPD_LOG_ERROR(NULL, "libmygpiod: Out of memory");
        return NULL;
    }
    if (mygpio_connection_get_state(conn) != MYGPIO_STATE_OK) {
        MYMPD_LOG_ERROR(NULL, "libmygpiod: %s", mygpio_connection_get_error(conn));
        mygpio_connection_free(conn);
        return NULL;
    }
    MYMPD_LOG_DEBUG(NULL, "libmygpiod: Connected to %s", mygpiod_socket);
    return conn;
}
