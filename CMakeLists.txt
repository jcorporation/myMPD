#
# SPDX-License-Identifier: GPL-2.0-or-later
# myMPD (c) 2018-2020 Juergen Mang <mail@jcgames.de>
# https://github.com/jcorporation/mympd
#

cmake_minimum_required(VERSION 2.6)

include(GNUInstallDirs)

project (mympd C)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "6")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")

if(EXISTS "/etc/arch-release")
  set(WEBAPPS "/webapps/${PROJECT_NAME}")
else()
  set(WEBAPPS "")
endif()

if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr")
  set(SUBDIR_ETC "${WEBAPPS}")
  set(SUBDIR "/${PROJECT_NAME}")
  set(SUBDIRLIB "/lib")
else() 
  set(SUBDIR_ETC "")
  set(SUBDIR "")
  set(SUBDIRLIB "")
endif()

if(${EMBEDDED_LIBMPDCLIENT} MATCHES "OFF")
   message("Compiling with libmpdclient from system")
else()
  set(EMBEDDED_LIBMPDCLIENT "ON")
  message("Compiling with embedded libmpdclient")
endif()

set(ETC_PATH "${CMAKE_INSTALL_FULL_SYSCONFDIR}${SUBDIR_ETC}")
message("ETC_PATH: ${ETC_PATH}")

message("BIN_PATH: ${CMAKE_INSTALL_FULL_BINDIR}")

set(VARLIB_PATH "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
if("${CMAKE_INSTALL_PREFIX}" MATCHES "/usr/local")
  set(VARLIB_PATH "/${CMAKE_INSTALL_LOCALSTATEDIR}${SUBDIRLIB}${SUBDIR}")
endif()
message("VARLIB_PATH: ${VARLIB_PATH}")

if(CMAKE_BUILD_TYPE MATCHES RELEASE)
  set(ASSETS_PATH "${VARLIB_PATH}/empty")
  set(DEBUG "OFF")
else()
  set(ASSETS_PATH "${PROJECT_SOURCE_DIR}/htdocs")
  set(DEBUG "ON")
endif()
message("ASSETS_PATH: ${ASSETS_PATH}")

find_package(Threads REQUIRED)

if(NOT "${EMBEDDED_LIBMPDCLIENT}" MATCHES "ON")
  message("Searching for libmpdclinet")
  find_package(LibMPDClient REQUIRED)
endif()
if(${EMBEDDED_LIBMPDCLIENT} MATCHES "ON")
  include_directories(dist/src/libmpdclient/include)
else()
  include_directories(${LIBMPDCLIENT_INCLUDE_DIR})
endif()

if(NOT "${ENABLE_SSL}" MATCHES "OFF")
  message("Searching for openssl")
  find_package(OpenSSL)
endif()
if(OPENSSL_FOUND)
  set(ENABLE_SSL "ON")
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

configure_file(src/config_defs.h.in ${PROJECT_BINARY_DIR}/config_defs.h)
configure_file(cmake/CopyConfig.cmake.in cmake/CopyConfig.cmake @ONLY)
configure_file(contrib/mympd.conf.in contrib/mympd.conf @ONLY)
configure_file(contrib/initscripts/mympd.service.in contrib/initscripts/mympd.service @ONLY)
configure_file(contrib/initscripts/mympd.sysVinit.in contrib/initscripts/mympd.sysVinit @ONLY)
configure_file(contrib/initscripts/mympd.openrc.in contrib/initscripts/mympd.openrc @ONLY)

include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})
include(CheckCSourceCompiles)

if(${MEMCHECK} MATCHES "TRUE")
  #for use with valgrind
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Werror")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -O1 -Wall -Werror -fstack-protector -D_FORTIFY_SOURCE=2 -pie -fPIE")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -fsanitize=address \
	-fsanitize=undefined -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=unreachable -fsanitize=vla-bound \
	-fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=bounds-strict \
	-fsanitize=alignment -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow \
	-fsanitize=nonnull-attribute -fsanitize=returns-nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -static-libasan")
endif()

if(OPENSSL_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMG_ENABLE_SSL")
endif()

#compiler flags for mympd src
file(GLOB MYMPD_SRC_FILES1 "src/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES1} PROPERTY COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings \
	-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

file(GLOB MYMPD_SRC_FILES2 "src/mpd_client/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES2} PROPERTY COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings \
	-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

file(GLOB MYMPD_SRC_FILES3 "src/mympd_api/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES3} PROPERTY COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings \
	-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

file(GLOB MYMPD_SRC_FILES4 "src/web_server/*.c")
set_property(SOURCE ${MYMPD_SRC_FILES4} PROPERTY COMPILE_FLAGS "-Wextra -pedantic -Wformat=2 -Wunused-parameter -Wshadow -Wwrite-strings \
	-Wstrict-prototypes -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs")

#compiler flags for inih, issue#188
set_property(SOURCE dist/src/inih/ini.c PROPERTY COMPILE_FLAGS "-Wno-stringop-truncation")

#compiler flags for frozen
set_property(SOURCE dist/src/frozen/frozen.c PROPERTY COMPILE_FLAGS "-Wno-stringop-overflow")

#compiler flags for mongoose.c
set_property(SOURCE dist/src/mongoose/mongoose.c PROPERTY COMPILE_FLAGS "-Wno-format-truncation")

set (CMAKE_EXE_LINKER_FLAGS "-Wl,-z,relro -Wl,-z,now")

set(SOURCES
  src/main.c
  src/api.c
  src/global.c
  src/list.c
  src/tiny_queue.c
  src/log.c
  src/config.c
  src/handle_options.c
  src/maintenance.c
  src/utility.c
  src/sds_extras.c
  src/mpd_client.c
  src/mpd_client/mpd_client_api.c
  src/mpd_client/mpd_client_cover.c
  src/mpd_client/mpd_client_browse.c
  src/mpd_client/mpd_client_features.c
  src/mpd_client/mpd_client_jukebox.c
  src/mpd_client/mpd_client_utility.c
  src/mpd_client/mpd_client_playlists.c
  src/mpd_client/mpd_client_queue.c
  src/mpd_client/mpd_client_search.c
  src/mpd_client/mpd_client_settings.c
  src/mpd_client/mpd_client_state.c
  src/mpd_client/mpd_client_stats.c
  src/mpd_client/mpd_client_timer.c
  src/mympd_api.c
  src/mympd_api/mympd_api_bookmarks.c
  src/mympd_api/mympd_api_utility.c
  src/mympd_api/mympd_api_settings.c
  src/mympd_api/mympd_api_syscmds.c
  src/mympd_api/mympd_api_timer.c
  src/mympd_api/mympd_api_timer_handlers.c
  src/web_server.c
  src/web_server/web_server_utility.c
  src/web_server/web_server_albumart.c
  dist/src/mongoose/mongoose.c
  dist/src/frozen/frozen.c
  dist/src/inih/ini.c
  dist/src/sds/sds.c
)

if(OPENSSL_FOUND)
  set(CERT_SOURCES
      src/cert.c
  )
endif()

if(${EMBEDDED_LIBMPDCLIENT} MATCHES "ON")
  set(LIBMPDCLIENT_SOURCES
    dist/src/libmpdclient/src/albumart.c
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
    dist/src/libmpdclient/src/rdirectory.c
    dist/src/libmpdclient/src/readpicture.c
    dist/src/libmpdclient/src/recv.c
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
endif()

add_executable(mympd ${SOURCES} ${LIBMPDCLIENT_SOURCES} ${CERT_SOURCES})

target_link_libraries(mympd ${CMAKE_THREAD_LIBS_INIT})
if (OPENSSL_FOUND)
  target_link_libraries(mympd ${OPENSSL_LIBRARIES})
endif()
if (LIBID3TAG_FOUND)
  target_link_libraries(mympd ${LIBID3TAG_LIBRARIES})
endif()
if (FLAC_FOUND)
  target_link_libraries(mympd ${FLAC_LIBRARIES})
endif()
if(NOT ${EMBEDDED_LIBMPDCLIENT} MATCHES "ON")
  target_link_libraries(mympd ${LIBMPDCLIENT_LIBRARY})
endif()

install(TARGETS mympd DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/cmake/CopyConfig.cmake)
