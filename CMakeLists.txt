# SPDX-License-Identifier: GPL-3.0-or-later
# myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd

#myMPD is written in C
#supported compilers: gcc, clang
project (mympd C)

#update myMPD version here and run ./build.sh setversion to update
#the source files
set(CPACK_PACKAGE_VERSION_MAJOR "9")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")

#minimal cmake version needed for correct GNUInstallDirs
cmake_minimum_required(VERSION 3.4)

#available options
option(ENABLE_SSL "Enables OpenSSL usage, default ON" ON)
option(ENABLE_LIBID3TAG "Enables libid3tag usage, default ON" ON)
option(ENABLE_FLAC "Enables flac usage, default ON" ON)
option(ENABLE_LUA "Enables lua usage, default ON" ON)
option(EMBEDDED_ASSETS "Embeds assets in binary, default ON" ON)
option(ENABLE_LIBASAN "Enables build with libasan, default OFF" OFF)

#cmake modules
include(GNUInstallDirs)
include(CheckCSourceCompiles)
include(CheckCCompilerFlag)

#custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")

#clang compatibility
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  message("Compiling with clang")
  add_compile_options("-Wno-unknown-warning-option")
endif()

message("Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")

#calculate paths
if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr")
  set(SUBDIR "/${PROJECT_NAME}")
  set(SUBDIRLIB "/lib")
  set(SUBDIRCACHE "/cache")
else()
  #for install in /opt
  set(SUBDIR "")
  set(SUBDIRLIB "")
  set(SUBDIRCACHE "")
endif()

message("BIN_PATH: ${CMAKE_INSTALL_FULL_BINDIR}")

if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr/local")
  set(VARLIB_PATH "/${CMAKE_INSTALL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
else()
  set(VARLIB_PATH "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
endif()
message("VARLIB_PATH: ${VARLIB_PATH}")

if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr/local")
  set(VARCACHE_PATH "/${CMAKE_INSTALL_LOCALSTATEDIR}${SUBDIRCACHE}${SUBDIR}")
else()
  set(VARCACHE_PATH "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRCACHE}${SUBDIR}")
endif()
message("VARCACHE_PATH: ${VARCACHE_PATH}")

#default is to deliver embedded assets for release
#and assets in htdocs for debug
if("${EMBEDDED_ASSETS}" MATCHES "ON")
  set(ASSETS_PATH "${VARLIB_PATH}/empty")
  set(LUALIBS_PATH "")
  message("Emedding assets in binary")
else()
  set(ASSETS_PATH "${PROJECT_SOURCE_DIR}/htdocs")
  set(LUALIBS_PATH "${PROJECT_SOURCE_DIR}/contrib/lualibs")
  message("Serving assets from filesystem")
endif()
message("ASSETS_PATH: ${ASSETS_PATH}")

#set debug define for source
if(CMAKE_BUILD_TYPE MATCHES RELEASE)
  set(DEBUG "OFF")
else()
  set(DEBUG "ON")
endif()

#required dependencies
find_package(Threads REQUIRED)
find_package(PCRE2 REQUIRED)

#optional dependencies
if(NOT "${ENABLE_SSL}" MATCHES "OFF")
  message("Searching for openssl")
  find_package(OpenSSL)
  if(OPENSSL_FOUND)
    if ("${OPENSSL_VERSION}" VERSION_GREATER_EQUAL "1.1.0")
      set(ENABLE_SSL "ON")
      set(MONGOOSE_SSL "-DMG_ENABLE_OPENSSL=1")
      include_directories(${OPENSSL_INCLUDE_DIR})
    else()
      message("OpenSSL is disabled because a version lower than 1.1.0 was found")
      set(ENABLE_SSL "OFF")
    endif()
  else()
    message("OpenSSL is disabled because it was not found")
    set(ENABLE_SSL "OFF")
  endif()
else()
  message("OpenSSL is disabled by user")
endif()

if(NOT "${ENABLE_LIBID3TAG}" MATCHES "OFF")
  message("Searching for libid3tag")
  find_package(LIBID3TAG)
  if(LIBID3TAG_FOUND)
    set(ENABLE_LIBID3TAG "ON")
    include_directories(${LIBID3TAG_INCLUDE_DIRS})
  else()
    message("Libid3tag is disabled because it was not found")
    set(ENABLE_LIBID3TAG "OFF")
  endif()
else("${ENABLE_LIBID3TAG}" MATCHES "OFF")
  message("Libid3tag is disabled by user")
endif()

if(NOT "${ENABLE_FLAC}" MATCHES "OFF")
  message("Searching for flac")
  find_package(FLAC)
  if(FLAC_FOUND)
    set(ENABLE_FLAC "ON")
    include_directories(${FLAC_INCLUDE_DIRS})
  else()
    message("Flac is disabled because it was not found")
    set(ENABLE_FLAC "OFF")
  endif()
else()
  message("Flac is disabled by user")
endif()

if(NOT "${ENABLE_LUA}" MATCHES "OFF")
  IF(EXISTS "/etc/alpine-release")
    set(ENV{LUA_DIR} "/usr/lib/lua5.4")
  endif()
  message("Searching for lua")
  find_package(Lua)
  if(LUA_FOUND)
    if("${LUA_VERSION_STRING}" VERSION_GREATER_EQUAL "5.3.0")
      set(ENABLE_LUA "ON")
      include_directories(${LUA_INCLUDE_DIR})
    else()
      message("Lua is disabled because a version lower than 5.3.0 was found")
      set(ENABLE_LUA "OFF")
    endif()
  else()
    message("Lua is disabled because it was not found")
    set(ENABLE_LUA "OFF")
  endif()
else()
  message("Lua is disabled by user")
endif()

#configure some files - version and path information
configure_file(src/mympd_config_defs.h.in ${PROJECT_BINARY_DIR}/mympd_config_defs.h)
configure_file(cmake/CopyConfig.cmake.in cmake/CopyConfig.cmake @ONLY)
configure_file(contrib/initscripts/mympd.service.in contrib/initscripts/mympd.service @ONLY)
configure_file(contrib/initscripts/mympd.sysVinit.in contrib/initscripts/mympd.sysVinit @ONLY)
configure_file(contrib/initscripts/mympd.openrc.in contrib/initscripts/mympd.openrc @ONLY)

#set include dirs
include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})
include_directories(${PCRE2_INCLUDE_DIRS})

#we use the integrated libmpdclient, distribution versions are often outdated
include_directories("dist/libmpdclient/include")

#check for enabling libasan
if("${ENABLE_LIBASAN}" MATCHES "ON")
  message("Compiling with libasan")
  set(LIBASAN_FLAGS "-fsanitize=address \
    -fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
    -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
    -fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
    -fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr \
    -static-libasan")
else()
  set(LIBASAN_FLAGS "")
endif()

#set mongoose functions
set(MONGOOSE_FLAGS "${MONGOOSE_SSL} -DMG_ENABLE_DIRECTORY_LISTING=1 -DMG_ENABLE_SSI=0")
#set mjson functions
set(MJSON_FLAGS "-D MJSON_ENABLE_PRINT=0 -D MJSON_ENABLE_BASE64=0 -D MJSON_ENABLE_RPC=0 \
  -D MJSON_ENABLE_PRETTY=0 -D MJSON_ENABLE_MERGE=0")

#set strict global compile flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong -D_FORTIFY_SOURCE=2 \
  -ffunction-sections -fdata-sections -Wall -Werror -Wvla -Winit-self -Wuninitialized \
	-Werror=array-bounds -Wextra -pedantic -Wunused-parameter -Wshadow -Wwrite-strings \
	-Wformat=2 -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs \
	-Wmissing-include-dirs -Wundef -Wformat-nonliteral -Wsign-compare \
	${LIBASAN_FLAGS} ${MONGOOSE_FLAGS} ${MJSON_FLAGS}")

#check for supported compiler flags
foreach(FLAG IN ITEMS "-std=gnu17" "-fstack-clash-protection" "-fcf-protection" "-fno-plt")
  message("Checking for compiler flag ${FLAG}")
  unset(COMPILER_SUPPORTS_FLAG CACHE)
  unset(COMPILER_SUPPORTS_FLAG)
  CHECK_C_COMPILER_FLAG("${FLAG}" COMPILER_SUPPORTS_FLAG)
  if(COMPILER_SUPPORTS_FLAG MATCHES "1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
  endif()
endforeach()

#set debug/release flags
if(CMAKE_BUILD_TYPE MATCHES RELEASE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fPIE -DMG_ENABLE_LOG=0")
  set (CMAKE_EXE_LINKER_FLAGS "-Wl,-z,relro,-z,now,--gc-sections -pie -s")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -Og")
  set (CMAKE_EXE_LINKER_FLAGS "")
endif()

#relax some compiler settings for dist sources
#compiler flags for mongoose
set_property(SOURCE dist/mongoose/mongoose.c PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral")
#compiler flags for rax
set_property(SOURCE dist/rax/rax.c PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral \
  -Wno-shadow -Wno-discarded-qualifiers -Wno-incompatible-pointer-types-discards-qualifiers")
#compiler flags for sds
set_property(SOURCE dist/sds/sds.c PROPERTY COMPILE_FLAGS "-Wno-format-nonliteral")

#set source files
set(MYMPD_SOURCES
  dist/mjson/mjson.c
  dist/mongoose/mongoose.c
  dist/rax/rax.c
  dist/sds/sds.c
  dist/tinymt/tinymt32.c
  src/handle_options.c
  src/lib/api.c
  src/lib/covercache.c
  src/lib/http_client.c
  src/lib/jsonrpc.c
  src/lib/list.c
  src/lib/log.c
  src/lib/lua_mympd_state.c
  src/lib/m3u.c
  src/lib/mimetype.c
  src/lib/mympd_pin.c
  src/lib/mympd_queue.c
  src/lib/random.c
  src/lib/sds_extras.c
  src/lib/state_files.c
  src/lib/utility.c
  src/lib/validate.c
  src/main.c
  src/mpd_client/mpd_client_autoconf.c
  src/mpd_client/mpd_client_features.c
  src/mpd_client/mpd_client_idle.c
  src/mpd_client/mpd_client_jukebox.c
  src/mpd_shared.c
  src/mpd_shared/mpd_shared_playlists.c
  src/mpd_shared/mpd_shared_search.c
  src/mpd_shared/mpd_shared_sticker.c
  src/mpd_shared/mpd_shared_tags.c
  src/mpd_worker.c
  src/mpd_worker/mpd_worker_api.c
  src/mpd_worker/mpd_worker_cache.c
  src/mpd_worker/mpd_worker_smartpls.c
  src/mpd_worker/mpd_worker_utility.c
  src/mympd_api.c
  src/mympd_api/mympd_api_albumart.c
  src/mympd_api/mympd_api_browse.c
  src/mympd_api/mympd_api_handler.c
  src/mympd_api/mympd_api_home.c
  src/mympd_api/mympd_api_lyrics.c
  src/mympd_api/mympd_api_mounts.c
  src/mympd_api/mympd_api_partitions.c
  src/mympd_api/mympd_api_playlists.c
  src/mympd_api/mympd_api_queue.c
  src/mympd_api/mympd_api_scripts.c
  src/mympd_api/mympd_api_settings.c
  src/mympd_api/mympd_api_stats.c
  src/mympd_api/mympd_api_status.c
  src/mympd_api/mympd_api_sticker.c
  src/mympd_api/mympd_api_timer.c
  src/mympd_api/mympd_api_timer_handlers.c
  src/mympd_api/mympd_api_trigger.c
  src/mympd_api/mympd_api_utility.c
  src/mympd_api/mympd_api_webradios.c
  src/mympd_config.c
  src/web_server.c
  src/web_server/web_server_albumart.c
  src/web_server/web_server_proxy.c
  src/web_server/web_server_radiobrowser.c
  src/web_server/web_server_sessions.c
  src/web_server/web_server_tagart.c
  src/web_server/web_server_utility.c
  src/web_server/web_server_webradiodb.c
)

if("${ENABLE_SSL}" MATCHES "ON")
  set(CERT_SOURCES
      src/lib/cert.c
  )
endif()

set(LIBMPDCLIENT_SOURCES
  dist/libmpdclient/src/albumart.c
  dist/libmpdclient/src/async.c
  dist/libmpdclient/src/audio_format.c
  dist/libmpdclient/src/binary.c
  dist/libmpdclient/src/capabilities.c
  dist/libmpdclient/src/cmessage.c
  dist/libmpdclient/src/cmount.c
  dist/libmpdclient/src/cneighbor.c
  dist/libmpdclient/src/connection.c
  dist/libmpdclient/src/coutput.c
  dist/libmpdclient/src/cpartition.c
  dist/libmpdclient/src/cplaylist.c
  dist/libmpdclient/src/cstats.c
  dist/libmpdclient/src/cstatus.c
  dist/libmpdclient/src/database.c
  dist/libmpdclient/src/directory.c
  dist/libmpdclient/src/entity.c
  dist/libmpdclient/src/error.c
  dist/libmpdclient/src/fd_util.c
  dist/libmpdclient/src/fingerprint.c
  dist/libmpdclient/src/idle.c
  dist/libmpdclient/src/ierror.c
  dist/libmpdclient/src/iso8601.c
  dist/libmpdclient/src/kvlist.c
  dist/libmpdclient/src/list.c
  dist/libmpdclient/src/message.c
  dist/libmpdclient/src/mixer.c
  dist/libmpdclient/src/mount.c
  dist/libmpdclient/src/neighbor.c
  dist/libmpdclient/src/output.c
  dist/libmpdclient/src/parser.c
  dist/libmpdclient/src/partition.c
  dist/libmpdclient/src/password.c
  dist/libmpdclient/src/player.c
  dist/libmpdclient/src/playlist.c
  dist/libmpdclient/src/position.c
  dist/libmpdclient/src/queue.c
  dist/libmpdclient/src/quote.c
  dist/libmpdclient/src/replay_gain.c
  dist/libmpdclient/src/rdirectory.c
  dist/libmpdclient/src/readpicture.c
  dist/libmpdclient/src/recv.c
  dist/libmpdclient/src/replay_gain.c
  dist/libmpdclient/src/resolver.c
  dist/libmpdclient/src/response.c
  dist/libmpdclient/src/rplaylist.c
  dist/libmpdclient/src/run.c
  dist/libmpdclient/src/search.c
  dist/libmpdclient/src/send.c
  dist/libmpdclient/src/settings.c
  dist/libmpdclient/src/socket.c
  dist/libmpdclient/src/song.c
  dist/libmpdclient/src/stats.c
  dist/libmpdclient/src/status.c
  dist/libmpdclient/src/sticker.c
  dist/libmpdclient/src/sync.c
  dist/libmpdclient/src/tag.c
)

#compile the executeable
add_executable(mympd ${MYMPD_SOURCES} ${LIBMPDCLIENT_SOURCES} ${CERT_SOURCES})

#link required dependencies
target_link_libraries(mympd ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(mympd m)
target_link_libraries(mympd ${PCRE2_LIBRARIES})
#link optional dependencies
if(OPENSSL_FOUND)
  target_link_libraries(mympd ${OPENSSL_LIBRARIES})
endif()
if(LIBID3TAG_FOUND)
  target_link_libraries(mympd ${LIBID3TAG_LIBRARIES})
endif()
if(FLAC_FOUND)
  target_link_libraries(mympd ${FLAC_LIBRARIES})
endif()
if(LUA_FOUND)
  target_link_libraries(mympd ${LUA_LIBRARIES})
endif()

#install
install(TARGETS mympd DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/cmake/CopyConfig.cmake)

#sub projects
add_subdirectory("cli_tools")
