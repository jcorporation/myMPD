# - Try to find Lua
# Once done, this will define
#
#   LUA_FOUND - System has lua
#   LUA_INCLUDE_DIR - The lua include directories
#   LUA_LIBRARIES - The libraries needed to use lua
#   LUA_DEFINITIONS - Compiler witches required for using lua

find_package(PkgConfig)
pkg_check_modules(PC_LUA QUIET lua5.3 lua5.4)
set(LUA_DEFINITIONS ${PC_LUA_CFLAGS_OTHER})

find_path(LUA_INCLUDE_DIR
    NAMES lua5.3/lua.h lua5.4/lua.h lua/5.3/lua.h lua/lua5.4/lua.h
    HINTS ${PC_LUA_INCLUDEDIR} ${PC_LUA_INCLUDE_DIRS}
)

find_library(LUA_LIBRARY
    NAMES lua5.3/liblua.so lua5.4/liblua.so lua/5.3/liblua.so lua/5.4/liblua.so liblua5.3.so.0 liblua5.4.so.0
    HINTS ${PC_LUA_LIBDIR} ${PC_LUA_LIBRARY_DIRS}
)

set(LUA_LIBRARIES ${LUA_LIBRARY})
set(LUA_INCLUDE_DIRS ${LUA_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Lua DEFAULT_MSG
    LUA_LIBRARY LUA_INCLUDE_DIR
)

mark_as_advanced(LUA_LIBRARY LUA_INCLUDE_DIR)
