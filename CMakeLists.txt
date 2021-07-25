#
# SPDX-License-Identifier: GPL-2.0-or-later
# myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

project (mympd C)
set(CPACK_PACKAGE_VERSION_MAJOR "8")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "2")

cmake_minimum_required(VERSION 3.4)
include(GNUInstallDirs)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")

if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr")
  set(SUBDIR "/${PROJECT_NAME}")
  set(SUBDIRLIB "/lib")
else() 
  set(SUBDIR "")
  set(SUBDIRLIB "")
endif()

message("BIN_PATH: ${CMAKE_INSTALL_FULL_BINDIR}")

set(VARLIB_PATH "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr/local")
  set(VARLIB_PATH "/${CMAKE_INSTALL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
endif()
message("VARLIB_PATH: ${VARLIB_PATH}")

if(CMAKE_BUILD_TYPE MATCHES RELEASE)
  set(ASSETS_PATH "${VARLIB_PATH}/empty")
  set(LUALIBS_PATH "")
  set(DEBUG "OFF")
else()
  set(ASSETS_PATH "${PROJECT_SOURCE_DIR}/htdocs")
  set(LUALIBS_PATH "${PROJECT_SOURCE_DIR}/contrib/lualibs")
  set(DEBUG "ON")
endif()
message("ASSETS_PATH: ${ASSETS_PATH}")

find_package(Threads REQUIRED)
find_package(PCRE REQUIRED)
include_directories(${PCRE_INCLUDE_DIRS})

if(NOT "${ENABLE_SSL}" MATCHES "OFF")
  message("Searching for openssl")
  find_package(OpenSSL)
endif()
if((OPENSSL_FOUND) AND ("${OPENSSL_VERSION}" VERSION_GREATER_EQUAL "1.1.0"))
  set(ENABLE_SSL "ON")
  set(MONGOOSE_SSL "-DMG_ENABLE_OPENSSL=1")
  include_directories(${OPENSSL_INCLUDE_DIR})
else()
  message("Openssl is disabled")
  set(ENABLE_SSL "OFF")
endif()

if(NOT "${ENABLE_LIBID3TAG}" MATCHES "OFF")
  message("Searching for libid3tag")
  find_package(LibId3Tag)
endif()
if(LIBID3TAG_FOUND)
  set(ENABLE_LIBID3TAG "ON")
  include_directories(${LIBID3TAG_INCLUDE_DIRS})
else()
  message("Libid3tag is disabled")
  set(ENABLE_LIBID3TAG "OFF")
endif()

if(NOT "${ENABLE_FLAC}" MATCHES "OFF")
  message("Searching for flac")
  find_package(FLAC)
endif()
if(FLAC_FOUND)
  set(ENABLE_FLAC "ON")
  include_directories(${FLAC_INCLUDE_DIRS})
else()
  message("Flac is disabled")
  set(ENABLE_FLAC "OFF")
endif()

if(NOT "${ENABLE_LUA}" MATCHES "OFF")
  IF(EXISTS "/etc/alpine-release")                                        
    set(ENV{LUA_DIR} "/usr/lib/lua5.4")                                 
  endif()     
  message("Searching for lua")
  find_package(Lua)
endif()
if(LUA_FOUND)
  if("${LUA_VERSION_STRING}" VERSION_GREATER "5.3.0")
    set(ENABLE_LUA "ON")
    include_directories(${LUA_INCLUDE_DIR})
  else()
    message("Lua is disabled because a version lower then 5.3.0 was found")
    set(ENABLE_LUA "OFF")
  endif()
elseif("${ENABLE_LUA}" MATCHES "OFF")
  message("Lua is disabled by user")
  set(ENABLE_LUA "OFF")
else()
  message("Lua is disabled because it was not found")
  set(ENABLE_LUA "OFF")
endif()

add_subdirectory("cli_tools")

configure_file(src/mympd_config_defs.h.in ${PROJECT_BINARY_DIR}/mympd_config_defs.h)
configure_file(cmake/CopyConfig.cmake.in cmake/CopyConfig.cmake @ONLY)
configure_file(contrib/initscripts/mympd.service.in contrib/initscripts/mympd.service @ONLY)
configure_file(contrib/initscripts/mympd.sysVinit.in contrib/initscripts/mympd.sysVinit @ONLY)
configure_file(contrib/initscripts/mympd.openrc.in contrib/initscripts/mympd.openrc @ONLY)

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR} dist/src/libmpdclient/include)
include(CheckCSourceCompiles)

if(${MEMCHECK} MATCHES "TRUE")
  #for use with valgrind
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -Werror -Wvla -Winit-self -Wuninitialized -Werror=array-bounds")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -O3 -Wall -Werror -Wvla -Winit-self -Wuninitialized -Werror=array-bounds -fstack-protector-strong -D_FORTIFY_SOURCE=2 -pie -fPIE")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -fsanitize=address \
	-fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
	-fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
	-fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
	-fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -static-libasan")
endif()

set(MONGOOSE_FLAGS "${MONGOOSE_SSL} -DMG_ENABLE_DIRECTORY_LISTING=1")

if(CMAKE_BUILD_TYPE MATCHES RELEASE)
  set(MONGOOSE_FLAGS "${MONGOOSE_FLAGS} -DMG_ENABLE_LOG=0")
endif()

set(MYMPD_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings \
	-Wformat=2  -Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

#compiler flags for mympd src
file(GLOB MYMPD_SRC_FILES1 "src/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES1} PROPERTY COMPILE_FLAGS "${MYMPD_FLAGS} ${MONGOOSE_FLAGS}")

file(GLOB MYMPD_SRC_FILES2 "src/mpd_client/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES2} PROPERTY COMPILE_FLAGS "${MYMPD_FLAGS}")

file(GLOB MYMPD_SRC_FILES3 "src/mympd_api/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES3} PROPERTY COMPILE_FLAGS "${MYMPD_FLAGS}")

file(GLOB MYMPD_SRC_FILES4 "src/web_server/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES4} PROPERTY COMPILE_FLAGS "${MYMPD_FLAGS} ${MONGOOSE_FLAGS}")

file(GLOB MYMPD_SRC_FILES5 "src/mpd_worker/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES5} PROPERTY COMPILE_FLAGS "${MYMPD_FLAGS}")

file(GLOB MYMPD_SRC_FILES6 "src/mpd_shared/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES6} PROPERTY COMPILE_FLAGS "${MYMPD_FLAGS}")

#compiler flags for frozen
set_property(SOURCE dist/src/frozen/frozen.c PROPERTY COMPILE_FLAGS "-Wno-stringop-overflow")

#compiler flags for mongoose
set_property(SOURCE dist/src/mongoose/mongoose.c PROPERTY COMPILE_FLAGS "${MONGOOSE_FLAGS}")

#linker flags
set (CMAKE_EXE_LINKER_FLAGS "-Wl,-z,relro -Wl,-z,now")

set(SOURCES
  src/main.c
  src/api.c
  src/covercache.c
  src/global.c
  src/handle_options.c
  src/list.c
  src/tiny_queue.c
  src/log.c
  src/mympd_autoconf.c
  src/mympd_config.c
  src/mympd_migrate.c
  src/utility.c
  src/random.c
  src/sds_extras.c
  src/state_files.c
  src/http_client.c
  src/lua_mympd_state.c
  src/mpd_shared.c
  src/mpd_shared/mpd_shared_search.c
  src/mpd_shared/mpd_shared_tags.c
  src/mpd_shared/mpd_shared_playlists.c
  src/mpd_shared/mpd_shared_sticker.c
  src/mpd_client.c
  src/mpd_client/mpd_client_cover.c
  src/mpd_client/mpd_client_browse.c
  src/mpd_client/mpd_client_features.c
  src/mpd_client/mpd_client_jukebox.c
  src/mpd_client/mpd_client_utility.c
  src/mpd_client/mpd_client_playlists.c
  src/mpd_client/mpd_client_queue.c
  src/mpd_client/mpd_client_state.c
  src/mpd_client/mpd_client_stats.c
  src/mpd_client/mpd_client_sticker.c
  src/mpd_client/mpd_client_timer.c
  src/mpd_client/mpd_client_mounts.c
  src/mpd_client/mpd_client_partitions.c
  src/mpd_client/mpd_client_trigger.c
  src/mpd_client/mpd_client_lyrics.c
  src/mpd_worker.c
  src/mpd_worker/mpd_worker_api.c
  src/mpd_worker/mpd_worker_utility.c
  src/mpd_worker/mpd_worker_smartpls.c
  src/mpd_worker/mpd_worker_cache.c
  src/mympd_api.c
  src/mympd_api/mympd_api_handler.c
  src/mympd_api/mympd_api_home.c
  src/mympd_api/mympd_api_utility.c
  src/mympd_api/mympd_api_settings.c
  src/mympd_api/mympd_api_timer.c
  src/mympd_api/mympd_api_timer_handlers.c
  src/mympd_api/mympd_api_scripts.c
  src/web_server.c
  src/web_server/web_server_utility.c
  src/web_server/web_server_albumart.c
  src/web_server/web_server_tagart.c
  dist/src/mongoose/mongoose.c
  dist/src/frozen/frozen.c
  dist/src/sds/sds.c
  dist/src/rax/rax.c
  dist/src/tinymt/tinymt32.c
  dist/src/utf8decode/utf8decode.c
)

if("${ENABLE_SSL}" MATCHES "ON")
  set(CERT_SOURCES
      src/cert.c
  )
endif()

set(LIBMPDCLIENT_SOURCES
  dist/src/libmpdclient/src/albumart.c
  dist/src/libmpdclient/src/binary.c
  dist/src/libmpdclient/src/async.c
  dist/src/libmpdclient/src/audio_format.c
  dist/src/libmpdclient/src/capabilities.c
  dist/src/libmpdclient/src/cmessage.c
  dist/src/libmpdclient/src/cmount.c
  dist/src/libmpdclient/src/cneighbor.c
  dist/src/libmpdclient/src/connection.c
  dist/src/libmpdclient/src/coutput.c
  dist/src/libmpdclient/src/cpartition.c
  dist/src/libmpdclient/src/cplaylist.c
  dist/src/libmpdclient/src/cstats.c
  dist/src/libmpdclient/src/cstatus.c
  dist/src/libmpdclient/src/database.c
  dist/src/libmpdclient/src/directory.c
  dist/src/libmpdclient/src/entity.c
  dist/src/libmpdclient/src/error.c
  dist/src/libmpdclient/src/fd_util.c
  dist/src/libmpdclient/src/fingerprint.c
  dist/src/libmpdclient/src/idle.c
  dist/src/libmpdclient/src/ierror.c
  dist/src/libmpdclient/src/iso8601.c
  dist/src/libmpdclient/src/kvlist.c
  dist/src/libmpdclient/src/list.c
  dist/src/libmpdclient/src/message.c
  dist/src/libmpdclient/src/mixer.c
  dist/src/libmpdclient/src/mount.c
  dist/src/libmpdclient/src/neighbor.c
  dist/src/libmpdclient/src/output.c
  dist/src/libmpdclient/src/parser.c
  dist/src/libmpdclient/src/partition.c
  dist/src/libmpdclient/src/password.c
  dist/src/libmpdclient/src/player.c
  dist/src/libmpdclient/src/playlist.c
  dist/src/libmpdclient/src/queue.c
  dist/src/libmpdclient/src/quote.c
  dist/src/libmpdclient/src/replay_gain.c
  dist/src/libmpdclient/src/rdirectory.c
  dist/src/libmpdclient/src/readpicture.c
  dist/src/libmpdclient/src/recv.c
  dist/src/libmpdclient/src/replay_gain.c
  dist/src/libmpdclient/src/resolver.c
  dist/src/libmpdclient/src/response.c
  dist/src/libmpdclient/src/rplaylist.c
  dist/src/libmpdclient/src/run.c
  dist/src/libmpdclient/src/search.c
  dist/src/libmpdclient/src/send.c
  dist/src/libmpdclient/src/settings.c
  dist/src/libmpdclient/src/socket.c
  dist/src/libmpdclient/src/song.c
  dist/src/libmpdclient/src/stats.c
  dist/src/libmpdclient/src/status.c
  dist/src/libmpdclient/src/sticker.c
  dist/src/libmpdclient/src/sync.c
  dist/src/libmpdclient/src/tag.c
)

add_executable(mympd ${SOURCES} ${LIBMPDCLIENT_SOURCES} ${CERT_SOURCES})

target_link_libraries(mympd ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(mympd m)
target_link_libraries(mympd ${PCRE_LIBRARIES})
if (OPENSSL_FOUND)
  target_link_libraries(mympd ${OPENSSL_LIBRARIES})
endif()
if (LIBID3TAG_FOUND)
  target_link_libraries(mympd ${LIBID3TAG_LIBRARIES})
endif()
if (FLAC_FOUND)
  target_link_libraries(mympd ${FLAC_LIBRARIES})
endif()
if (LUA_FOUND)
  target_link_libraries(mympd ${LUA_LIBRARIES})
endif()

install(TARGETS mympd DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/cmake/CopyConfig.cmake)
